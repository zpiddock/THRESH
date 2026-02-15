#pragma once

#include "render_data.hpp"
#include "material_system.hpp"
#include "flux/device.hpp"
#include "flux/mesh.hpp"
#include "flux/shader_program.hpp"
#include "flux/graphics_state.hpp"
#include "flux/sampler.hpp"
#include "renderer/frame_context.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace thresh {
    class RenderGraph;
}

namespace flux {
    struct PassExecutionContext;
}

namespace thresh {

    /**
     * Data needed by the ForwardPass each frame.
     * Returned by the FrameDataProvider callback.
     */
    struct ForwardPassFrameData {
        const FrameRenderData *render_data = nullptr;
        const std::vector<std::unique_ptr<flux::Mesh>> *meshes = nullptr;
    };

    /**
     * Callback that provides per-frame render data to the ForwardPass.
     * Called at the start of each pass execution on the render thread.
     * The callback should return pointers to the current frame's data
     * (e.g., by reading from shared state under a lock).
     */
    using FrameDataProvider = std::function<ForwardPassFrameData()>;

    /**
     * Forward rendering pass with PBR shading.
     *
     * Manages:
     * - Per-frame UBO (view/proj/camera/light)
     * - Per-frame SSBO (object transforms + material indices)
     * - Descriptor set 0 layout + pool + per-frame descriptor sets
     * - ShaderProgram (forward.vert + forward.frag)
     * - GraphicsState for standard mesh rendering
     *
     * Usage:
     *   1. Construct with Device, MaterialSystem, and shader directory
     *   2. Call setup_graph() during graph build phase with a FrameDataProvider
     *   3. The pass automatically uploads GPU data and renders each frame
     */
    class THRESH_API ForwardPass {
    public:
        struct Config {
            flux::Device *device = nullptr;
            MaterialSystem *material_system = nullptr;
            std::filesystem::path shader_dir;
        };

        explicit ForwardPass(const Config &config);
        ~ForwardPass();

        ForwardPass(const ForwardPass &) = delete;
        auto operator=(const ForwardPass &) -> ForwardPass & = delete;

        /**
         * Set up the render graph pass. Call during graph build phase.
         * Creates transient depth image and registers the forward pass.
         *
         * The frame_data_provider is called at the start of each pass execution
         * (on the render thread) to get the current frame's render data and meshes.
         * This callback is stored and invoked each frame.
         *
         * @param graph The render graph to set up in
         * @param swapchain_extent Current swapchain dimensions
         * @param frame_data_provider Callback that returns per-frame data
         */
        auto setup_graph(
            RenderGraph &graph,
            VkExtent2D swapchain_extent,
            FrameDataProvider frame_data_provider
        ) -> void;

        /**
         * Upload frame data (UBO + SSBO) for the current frame.
         * Called internally by the pass callback, or externally if needed.
         * @param frame_index Current frame-in-flight index
         * @param render_data Scene render data for this frame
         * @param meshes Mesh array (indexed by RenderObject::mesh_index)
         */
        auto update_frame_data(
            std::uint32_t frame_index,
            const FrameRenderData &render_data,
            const std::vector<std::unique_ptr<flux::Mesh>> &meshes
        ) -> void;

    private:
        auto create_descriptor_resources() -> void;
        auto create_ubo_buffers() -> void;
        auto create_object_ssbo_buffers() -> void;
        auto grow_object_ssbo_if_needed(std::uint32_t object_count) -> void;
        auto update_descriptors(std::uint32_t frame_index) -> void;

        auto destroy_buffer(VkBuffer &buffer, VkDeviceMemory &memory) -> void;

        flux::Device *m_device = nullptr;
        MaterialSystem *m_material_system = nullptr;

        // Shader program + graphics state
        std::unique_ptr<flux::ShaderProgram> m_shader_program;
        flux::GraphicsState m_graphics_state;

        // Descriptor set 0: UBO (binding 0) + Object SSBO (binding 1) + Material SSBO (binding 2)
        VkDescriptorSetLayout m_set0_layout = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptor_pool = VK_NULL_HANDLE;
        std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_descriptor_sets{};

        // Per-frame UBO (view/proj/camera/light)
        struct alignas(16) FrameUBO {
            glm::mat4 view;
            glm::mat4 proj;
            glm::vec3 camera_pos;
            float _pad0 = 0.0f;
            glm::vec3 sun_direction;
            float sun_intensity = 1.5f;
            glm::vec3 sun_color;
            float _pad1 = 0.0f;
        };

        struct UboBuffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void *mapped = nullptr;
        };
        std::array<UboBuffer, MAX_FRAMES_IN_FLIGHT> m_ubo_buffers{};

        // Per-frame object SSBO (transforms + material indices)
        struct alignas(16) GpuObjectData {
            glm::mat4 model;
            glm::mat4 normal_matrix;
            std::uint32_t material_index;
            std::uint32_t _pad0 = 0;
            std::uint32_t _pad1 = 0;
            std::uint32_t _pad2 = 0;
        };

        struct ObjectSsboBuffer {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void *mapped = nullptr;
            std::uint32_t capacity = 0; // Max number of objects
        };
        std::array<ObjectSsboBuffer, MAX_FRAMES_IN_FLIGHT> m_object_ssbo_buffers{};

        // Cached data for execute callback
        struct FrameCache {
            std::uint32_t object_count = 0;
            std::vector<RenderObject> objects;
        };
        std::array<FrameCache, MAX_FRAMES_IN_FLIGHT> m_frame_caches{};

        // Pointers for the execute callback
        const std::vector<std::unique_ptr<flux::Mesh>> *m_meshes_ptr = nullptr;
    };

} // namespace thresh
