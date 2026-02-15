#include "flux/texture_loader.hpp"
#include "flux/image.hpp"
#include "flux/image_utils.hpp"
#include "substratum/log.hpp"

#include <vk_mem_alloc.h>
#include "stb_image.h"
#include <ktx.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace flux {

    TextureLoader::TextureLoader(Device &device)
        : m_device{device} {
    }

    // ── Auto-detect by extension ────────────────────────────────────────────────

    auto TextureLoader::load(
        const std::vector<std::uint8_t> &data,
        const std::string &virtual_path,
        TextureType type
    ) -> std::unique_ptr<Texture> {
        // Extract extension (lowercase)
        auto dot_pos = virtual_path.rfind('.');
        if (dot_pos != std::string::npos) {
            auto ext = virtual_path.substr(dot_pos);
            // Lowercase the extension
            std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });

            if (ext == ".ktx2" || ext == ".ktx") {
                return load_ktx2(data, virtual_path, type);
            }
        }

        // Default: try stb_image path
        return load_stb(data, virtual_path, type);
    }

    // ── stb_image path (PNG/JPG/TGA/BMP/HDR) ───────────────────────────────────

    auto TextureLoader::load_stb(
        const std::vector<std::uint8_t> &data,
        const std::string &virtual_path,
        TextureType type
    ) -> std::unique_ptr<Texture> {
        int width = 0;
        int height = 0;
        int channels = 0;

        // Decode to RGBA8
        auto *pixels = stbi_load_from_memory(
            data.data(),
            static_cast<int>(data.size()),
            &width, &height, &channels,
            STBI_rgb_alpha  // Always request 4 channels
        );

        if (!pixels) {
            SUB_ERROR("stb_image failed to decode '{}': {}", virtual_path, stbi_failure_reason());
            return nullptr;
        }

        SUB_DEBUG("Decoded '{}': {}x{} ({} channels -> RGBA8)", virtual_path, width, height, channels);

        // Normal maps use UNORM (linear data), everything else uses SRGB
        auto format = (type == TextureType::Normal)
            ? VK_FORMAT_R8G8B8A8_UNORM
            : VK_FORMAT_R8G8B8A8_SRGB;

        auto image = upload_pixels(
            pixels,
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height),
            format,
            true  // generate mipmaps
        );

        stbi_image_free(pixels);

        if (!image) {
            SUB_ERROR("Failed to upload pixels for '{}'", virtual_path);
            return nullptr;
        }

        return std::make_unique<Texture>(std::move(image), type, virtual_path);
    }

    // ── KTX2 path (GPU-compressed + Basis Universal) ────────────────────────────

    auto TextureLoader::load_ktx2(
        const std::vector<std::uint8_t> &data,
        const std::string &virtual_path,
        TextureType type
    ) -> std::unique_ptr<Texture> {
        ktxTexture2 *ktx_texture = nullptr;

        auto result = ktxTexture2_CreateFromMemory(
            data.data(),
            data.size(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &ktx_texture
        );

        if (result != KTX_SUCCESS) {
            SUB_ERROR("Failed to load KTX2 '{}': error code {}", virtual_path, static_cast<int>(result));
            return nullptr;
        }

        // If Basis Universal supercompressed, transcode to best GPU format
        if (ktxTexture2_NeedsTranscoding(ktx_texture)) {
            // Choose transcode target based on device support
            ktx_transcode_fmt_e target_format = KTX_TTF_RGBA32;

            // Prefer BC7 on desktop (widely supported)
            if (is_format_supported(VK_FORMAT_BC7_SRGB_BLOCK)) {
                target_format = KTX_TTF_BC7_RGBA;
                SUB_DEBUG("Transcoding '{}' to BC7", virtual_path);
            } else if (is_format_supported(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)) {
                target_format = KTX_TTF_ASTC_4x4_RGBA;
                SUB_DEBUG("Transcoding '{}' to ASTC 4x4", virtual_path);
            } else if (is_format_supported(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)) {
                target_format = KTX_TTF_ETC2_RGBA;
                SUB_DEBUG("Transcoding '{}' to ETC2", virtual_path);
            } else {
                SUB_WARN("No compressed format supported, transcoding '{}' to RGBA32", virtual_path);
            }

            result = ktxTexture2_TranscodeBasis(ktx_texture, target_format, 0);
            if (result != KTX_SUCCESS) {
                SUB_ERROR("Failed to transcode KTX2 '{}': error code {}", virtual_path, static_cast<int>(result));
                ktxTexture_Destroy(ktxTexture(ktx_texture));
                return nullptr;
            }
        }

        auto vk_format = static_cast<VkFormat>(ktx_texture->vkFormat);
        auto tex_width = ktx_texture->baseWidth;
        auto tex_height = ktx_texture->baseHeight;
        auto mip_levels = ktx_texture->numLevels;

        SUB_DEBUG("KTX2 '{}': {}x{}, {} mip levels, VkFormat {}",
                  virtual_path, tex_width, tex_height, mip_levels, static_cast<int>(vk_format));

        // Create device-local image
        Image::Config image_config{};
        image_config.device = m_device.get_logical_device();
        image_config.allocator = m_device.get_allocator();
        image_config.width = tex_width;
        image_config.height = tex_height;
        image_config.mip_levels = mip_levels;
        image_config.format = vk_format;
        image_config.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        auto image = std::make_unique<Image>(image_config);

        // Calculate total upload size for staging buffer
        VkDeviceSize total_size = 0;
        for (std::uint32_t level = 0; level < mip_levels; ++level) {
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset(ktxTexture(ktx_texture), level, 0, 0, &offset);

            ktx_size_t image_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), level);
            total_size += image_size;
        }

        // Create staging buffer via VMA
        VkBufferCreateInfo staging_buf_info{};
        staging_buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_buf_info.size = total_size;
        staging_buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VmaAllocation staging_alloc = nullptr;
        VmaAllocationInfo staging_info{};

        auto vma_result = ::vmaCreateBuffer(
            m_device.get_allocator(),
            &staging_buf_info, &staging_alloc_info,
            &staging_buffer, &staging_alloc, &staging_info
        );

        if (vma_result != VK_SUCCESS) {
            SUB_ERROR("Failed to create KTX2 staging buffer for '{}'", virtual_path);
            ktxTexture_Destroy(ktxTexture(ktx_texture));
            return nullptr;
        }

        // Copy all mip levels into staging buffer
        auto *staging_ptr = static_cast<std::uint8_t *>(staging_info.pMappedData);
        VkDeviceSize staging_offset = 0;

        struct MipCopyInfo {
            VkDeviceSize buffer_offset;
            std::uint32_t width;
            std::uint32_t height;
        };
        std::vector<MipCopyInfo> mip_copies(mip_levels);

        for (std::uint32_t level = 0; level < mip_levels; ++level) {
            ktx_size_t offset = 0;
            ktxTexture_GetImageOffset(ktxTexture(ktx_texture), level, 0, 0, &offset);

            ktx_size_t image_size = ktxTexture_GetImageSize(ktxTexture(ktx_texture), level);
            auto *src = ktx_texture->pData + offset;

            std::memcpy(staging_ptr + staging_offset, src, image_size);

            mip_copies[level].buffer_offset = staging_offset;
            mip_copies[level].width = std::max(1u, tex_width >> level);
            mip_copies[level].height = std::max(1u, tex_height >> level);

            staging_offset += image_size;
        }

        // Record upload commands
        auto cmd = m_device.begin_single_time_commands();

        // Transition all mip levels to TRANSFER_DST
        image->transition_layout(cmd,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mip_levels);

        // Copy each mip level from staging buffer
        for (std::uint32_t level = 0; level < mip_levels; ++level) {
            VkBufferImageCopy region{};
            region.bufferOffset = mip_copies[level].buffer_offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = level;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {mip_copies[level].width, mip_copies[level].height, 1};

            ::vkCmdCopyBufferToImage(
                cmd, staging_buffer, image->get_image(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region
            );
        }

        // Transition all mip levels to SHADER_READ_ONLY
        image->transition_layout(cmd,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            0, mip_levels);

        m_device.end_single_time_commands(cmd);

        // Clean up staging + KTX
        ::vmaDestroyBuffer(m_device.get_allocator(), staging_buffer, staging_alloc);
        ktxTexture_Destroy(ktxTexture(ktx_texture));

        return std::make_unique<Texture>(std::move(image), type, virtual_path);
    }

    // ── Solid color textures ────────────────────────────────────────────────────

    auto TextureLoader::create_solid_color(
        std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a,
        TextureType type
    ) -> std::unique_ptr<Texture> {
        std::uint8_t pixel[4] = {r, g, b, a};

        auto format = (type == TextureType::Normal)
            ? VK_FORMAT_R8G8B8A8_UNORM
            : VK_FORMAT_R8G8B8A8_SRGB;

        auto image = upload_pixels(pixel, 1, 1, format, false);
        if (!image) {
            SUB_ERROR("Failed to create solid color texture");
            return nullptr;
        }

        return std::make_unique<Texture>(std::move(image), type, "<solid_color>");
    }

    auto TextureLoader::create_default_white() -> std::unique_ptr<Texture> {
        return create_solid_color(255, 255, 255, 255, TextureType::Albedo);
    }

    auto TextureLoader::create_default_normal() -> std::unique_ptr<Texture> {
        // Tangent-space "up" normal: (0, 0, 1) encoded as (128, 128, 255)
        return create_solid_color(128, 128, 255, 255, TextureType::Normal);
    }

    // ── Internal: upload raw pixels via staging buffer ──────────────────────────

    auto TextureLoader::upload_pixels(
        const void *pixels,
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        bool generate_mips
    ) -> std::unique_ptr<Image> {
        auto mip_levels = generate_mips ? calculate_mip_levels(width, height) : 1u;
        VkDeviceSize image_size = static_cast<VkDeviceSize>(width) * height * 4; // RGBA8

        // Create staging buffer via VMA (host-visible, mapped)
        VkBufferCreateInfo staging_buf_info{};
        staging_buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        staging_buf_info.size = image_size;
        staging_buf_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        staging_buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo staging_alloc_info{};
        staging_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
        staging_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                   VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer staging_buffer = VK_NULL_HANDLE;
        VmaAllocation staging_alloc = nullptr;
        VmaAllocationInfo staging_info{};

        auto vma_result = ::vmaCreateBuffer(
            m_device.get_allocator(),
            &staging_buf_info, &staging_alloc_info,
            &staging_buffer, &staging_alloc, &staging_info
        );

        if (vma_result != VK_SUCCESS) {
            SUB_ERROR("Failed to create staging buffer for pixel upload");
            return nullptr;
        }

        // Copy pixel data into staging buffer
        std::memcpy(staging_info.pMappedData, pixels, static_cast<std::size_t>(image_size));

        // Create device-local image
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (generate_mips) {
            // Need TRANSFER_SRC for mipmap blit chain
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        Image::Config image_config{};
        image_config.device = m_device.get_logical_device();
        image_config.allocator = m_device.get_allocator();
        image_config.width = width;
        image_config.height = height;
        image_config.mip_levels = mip_levels;
        image_config.format = format;
        image_config.usage = usage;

        auto image = std::make_unique<Image>(image_config);

        // Record upload commands
        auto cmd = m_device.begin_single_time_commands();

        // Transition to TRANSFER_DST
        image->transition_layout(cmd,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mip_levels);

        // Copy staging buffer -> image (mip level 0)
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        ::vkCmdCopyBufferToImage(
            cmd, staging_buffer, image->get_image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region
        );

        if (generate_mips && mip_levels > 1) {
            // generate_mipmaps expects all mip levels in TRANSFER_DST layout.
            // Mip 0 is already TRANSFER_DST, other mips are also TRANSFER_DST from the
            // full-range transition above. generate_mipmaps will handle all transitions
            // and leave the image in SHADER_READ_ONLY_OPTIMAL.
            generate_mipmaps(m_device, cmd, image->get_image(), format, width, height, mip_levels);
        } else {
            // No mipmaps — transition directly to SHADER_READ_ONLY
            image->transition_layout(cmd,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        m_device.end_single_time_commands(cmd);

        // Clean up staging buffer
        ::vmaDestroyBuffer(m_device.get_allocator(), staging_buffer, staging_alloc);

        return image;
    }

    // ── Format support query ────────────────────────────────────────────────────

    auto TextureLoader::is_format_supported(VkFormat format) const -> bool {
        VkFormatProperties properties;
        ::vkGetPhysicalDeviceFormatProperties(m_device.get_physical_device(), format, &properties);

        // We need optimal tiling with sampled image support
        return (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
    }

} // namespace flux
