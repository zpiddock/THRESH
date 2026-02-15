#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <cstdint>
#include <variant>

#ifdef _WIN32
#ifdef FLUX_EXPORTS
#define FLUX_API __declspec(dllexport)
#else
#define FLUX_API __declspec(dllimport)
#endif
#else
#define FLUX_API
#endif

// Forward declare VMA types (defined in global namespace by VMA)
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

namespace flux {
    // Unique identifier for resources within the graph
    using ResourceHandle = std::uint32_t;
    constexpr ResourceHandle INVALID_RESOURCE = ~0u;

    // Resource type enumeration
    enum class ResourceType {
        Image,
        Buffer
    };

    // Queue type for multi-queue support
    enum class QueueType {
        Graphics,
        Compute,
        Transfer
    };

    // Image resource description
    struct FLUX_API ImageResourceDesc {
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent3D extent = {0, 0, 1};
        VkImageUsageFlags usage = 0;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        std::uint32_t mip_levels = 1;
        std::uint32_t array_layers = 1;
        bool is_transient = true; // Can be aliased with other transients

        // Helper for common 2D image creation
        static auto create_2d(
            VkFormat format,
            std::uint32_t width,
            std::uint32_t height,
            VkImageUsageFlags usage
        ) -> ImageResourceDesc {
            return {
                .format = format,
                .extent = {width, height, 1},
                .usage = usage,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .mip_levels = 1,
                .array_layers = 1,
                .is_transient = true
            };
        }
    };

    // Buffer resource description
    struct FLUX_API BufferResourceDesc {
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = 0;
        bool is_transient = true;
    };

    // Unified resource description
    struct FLUX_API ResourceDesc {
        std::string name;
        ResourceType type = ResourceType::Image;
        std::variant<ImageResourceDesc, BufferResourceDesc> desc;

        auto is_image() const -> bool { return type == ResourceType::Image; }
        auto is_buffer() const -> bool { return type == ResourceType::Buffer; }

        auto get_image_desc() const -> const ImageResourceDesc & {
            return std::get<ImageResourceDesc>(desc);
        }

        auto get_buffer_desc() const -> const BufferResourceDesc & {
            return std::get<BufferResourceDesc>(desc);
        }
    };

    // Resource state tracking for barrier generation
    struct FLUX_API ResourceState {
        VkPipelineStageFlags2 stage_mask = VK_PIPELINE_STAGE_2_NONE;
        VkAccessFlags2 access_mask = VK_ACCESS_2_NONE;
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED; // For images only
        std::uint32_t queue_family = VK_QUEUE_FAMILY_IGNORED;

        auto operator==(const ResourceState &other) const -> bool {
            return stage_mask == other.stage_mask &&
                   access_mask == other.access_mask &&
                   layout == other.layout &&
                   queue_family == other.queue_family;
        }

        auto operator!=(const ResourceState &other) const -> bool {
            return !(*this == other);
        }
    };

    // Physical resource backing (allocated at compile time)
    struct FLUX_API PhysicalImage {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent3D extent = {0, 0, 0};
    };

    struct FLUX_API PhysicalBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = nullptr;
        VkDeviceSize size = 0;
    };

    struct FLUX_API PhysicalResource {
        ResourceType type = ResourceType::Image;
        std::variant<PhysicalImage, PhysicalBuffer> resource;

        auto is_image() const -> bool { return type == ResourceType::Image; }
        auto is_buffer() const -> bool { return type == ResourceType::Buffer; }

        auto get_image() const -> const PhysicalImage & {
            return std::get<PhysicalImage>(resource);
        }

        auto get_image() -> PhysicalImage & {
            return std::get<PhysicalImage>(resource);
        }

        auto get_buffer() const -> const PhysicalBuffer & {
            return std::get<PhysicalBuffer>(resource);
        }

        auto get_buffer() -> PhysicalBuffer & {
            return std::get<PhysicalBuffer>(resource);
        }
    };

    // Resource usage within a pass
    enum class ResourceUsage {
        // Read operations
        SampledImage,
        StorageImageRead,
        UniformBuffer,
        StorageBufferRead,
        VertexBuffer,
        IndexBuffer,
        IndirectBuffer,
        TransferSource,
        DepthStencilRead,
        InputAttachment,

        // Write operations
        ColorAttachment,
        DepthStencilWrite,
        StorageImageWrite,
        StorageBufferWrite,
        TransferDestination,

        // Read-write operations
        StorageImageReadWrite,
        StorageBufferReadWrite,
        DepthStencilReadWrite
    };

    // Convert ResourceUsage to Vulkan pipeline stage and access flags
    FLUX_API auto usage_to_stage_mask(ResourceUsage usage) -> VkPipelineStageFlags2;

    FLUX_API auto usage_to_access_mask(ResourceUsage usage) -> VkAccessFlags2;

    FLUX_API auto usage_to_image_layout(ResourceUsage usage) -> VkImageLayout;

    FLUX_API auto is_write_usage(ResourceUsage usage) -> bool;

    FLUX_API auto is_read_usage(ResourceUsage usage) -> bool;

    // Create a ResourceState from a ResourceUsage
    FLUX_API auto usage_to_state(ResourceUsage usage,
                                    std::uint32_t queue_family = VK_QUEUE_FAMILY_IGNORED) -> ResourceState;

    // Get image aspect flags from format
    FLUX_API auto format_to_aspect_mask(VkFormat format) -> VkImageAspectFlags;

    // Convert ResourceUsage to VkDescriptorType for auto-generated descriptors
    FLUX_API auto usage_to_descriptor_type(ResourceUsage usage) -> VkDescriptorType;

    // Get shader stage flags for a ResourceUsage (for descriptor set layout)
    FLUX_API auto usage_to_shader_stages(ResourceUsage usage) -> VkShaderStageFlags;
} // namespace flux