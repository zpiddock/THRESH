#pragma once

#include "texture.hpp"
#include "device.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#ifdef FLUX_EXPORTS
#define FLUX_API __declspec(dllexport)
#else
#define FLUX_API __declspec(dllimport)
#endif
#else
#define FLUX_API
#endif

namespace flux {

    /**
     * Loads textures from raw pixel data (stb_image) or GPU-compressed KTX2 files (libktx).
     *
     * Supports:
     * - PNG, JPG, TGA, BMP, HDR via stb_image -> RGBA8 staging -> device-local Image + mipmaps
     * - KTX2 via libktx -> Basis Universal transcode to best GPU format (BC7/ASTC/ETC2)
     *
     * All load methods use Device::begin_single_time_commands() for staging upload.
     */
    class FLUX_API TextureLoader {
    public:
        explicit TextureLoader(Device &device);
        ~TextureLoader() = default;

        TextureLoader(const TextureLoader &) = delete;
        auto operator=(const TextureLoader &) -> TextureLoader & = delete;

        /**
         * Auto-detect format by file extension and load accordingly.
         * .ktx2 / .ktx -> KTX2 path, everything else -> stb_image path.
         * @param data Raw file bytes (e.g. from VFS::read_file())
         * @param virtual_path Virtual path for metadata + extension detection
         * @param type Semantic texture type (Albedo, Normal, etc.)
         * @return Loaded texture or nullptr on failure
         */
        [[nodiscard]] auto load(
            const std::vector<std::uint8_t> &data,
            const std::string &virtual_path,
            TextureType type = TextureType::Albedo
        ) -> std::unique_ptr<Texture>;

        /**
         * Load from raw pixels decoded by stb_image (PNG/JPG/TGA/BMP).
         * Creates RGBA8_SRGB (or RGBA8_UNORM for Normal maps) device-local image with mipmaps.
         * @param data Raw file bytes
         * @param virtual_path Path for metadata
         * @param type Semantic texture type
         * @return Loaded texture or nullptr on failure
         */
        [[nodiscard]] auto load_stb(
            const std::vector<std::uint8_t> &data,
            const std::string &virtual_path,
            TextureType type = TextureType::Albedo
        ) -> std::unique_ptr<Texture>;

        /**
         * Load a KTX2 file (optionally Basis Universal supercompressed).
         * Transcodes to best GPU format supported by the device.
         * Mips are already baked into KTX2 — no generation needed.
         * @param data Raw KTX2 file bytes
         * @param virtual_path Path for metadata
         * @param type Semantic texture type
         * @return Loaded texture or nullptr on failure
         */
        [[nodiscard]] auto load_ktx2(
            const std::vector<std::uint8_t> &data,
            const std::string &virtual_path,
            TextureType type = TextureType::Albedo
        ) -> std::unique_ptr<Texture>;

        /**
         * Create a 1x1 solid-color texture.
         * Useful for default/fallback textures (white albedo, flat normal, etc.)
         * @param r Red component (0-255)
         * @param g Green component (0-255)
         * @param b Blue component (0-255)
         * @param a Alpha component (0-255)
         * @param type Semantic texture type
         * @return 1x1 texture with the specified color
         */
        [[nodiscard]] auto create_solid_color(
            std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a,
            TextureType type = TextureType::Albedo
        ) -> std::unique_ptr<Texture>;

        /**
         * Create default white albedo texture (1x1 white).
         */
        [[nodiscard]] auto create_default_white() -> std::unique_ptr<Texture>;

        /**
         * Create default flat normal map (1x1, RGB = 128,128,255 = tangent-space up).
         */
        [[nodiscard]] auto create_default_normal() -> std::unique_ptr<Texture>;

    private:
        /**
         * Upload raw RGBA pixel data to a device-local Image with mipmaps.
         * @param pixels RGBA8 pixel data
         * @param width Image width
         * @param height Image height
         * @param format Vulkan format
         * @param generate_mips Whether to generate mip chain
         * @return Image ready for shader sampling
         */
        [[nodiscard]] auto upload_pixels(
            const void *pixels,
            std::uint32_t width,
            std::uint32_t height,
            VkFormat format,
            bool generate_mips
        ) -> std::unique_ptr<Image>;

        /**
         * Check if device supports a specific VkFormat for sampled images.
         */
        [[nodiscard]] auto is_format_supported(VkFormat format) const -> bool;

        Device &m_device;
    };

} // namespace flux
