#pragma once

#include "flux/render_graph_resource.hpp"
#include "flux/render_graph_pass.hpp"
#include "flux/barrier_batcher.hpp"
#include "flux/transient_allocator.hpp"
#include "renderer/frame_context.hpp"  // For MAX_FRAMES_IN_FLIGHT

#include <array>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

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
    class Renderer;

    // Forward declarations
    class RenderGraphBuilder;
    class CompiledRenderGraph;

    /**
 * External resource descriptor for swapchain, persistent textures, etc.
 * These are resources not managed by the render graph but used by it.
 */
    struct THRESH_API ExternalResource {
        flux::ResourceHandle handle = flux::INVALID_RESOURCE;
        flux::ResourceType type = flux::ResourceType::Image;

        // For images
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent = {0, 0};

        // For buffers
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize size = 0;

        // Initial/final states for barrier generation
        flux::ResourceState initial_state;
        flux::ResourceState final_state;
    };

    /**
 * High-level render graph API.
 *
 * Usage:
 * 1. Create a RenderGraph
 * 2. Call begin_build() to get a builder
 * 3. Define resources and passes using the builder
 * 4. Call compile() to optimize and prepare for execution
 * 5. Call execute() each frame
 */
    class THRESH_API RenderGraph {
    public:
        explicit RenderGraph(Renderer &renderer);

        ~RenderGraph();

        RenderGraph(const RenderGraph &) = delete;

        RenderGraph &operator=(const RenderGraph &) = delete;

        /**
     * Begin building a new graph. Clears any existing graph.
     * @return Reference to the builder for chaining
     */
        auto begin_build() -> RenderGraphBuilder &;

        /**
     * Compile the graph for execution.
     * Performs topological sort, allocates resources, computes barriers.
     * @return true if compilation succeeded
     */
        auto compile() -> bool;

        /**
     * Execute the render graph.
     * @param cmd Command buffer to record into
     * @param frame_index Current frame index
     * @param delta_time Time since last frame
     */
        auto execute(VkCommandBuffer cmd, std::uint32_t frame_index, float delta_time = 0.0f) -> void;

        /**
     * Set the backbuffer (swapchain image) for this frame.
     * Must be called before execute() each frame.
     */
        auto set_backbuffer(
            VkImage image,
            VkImageView view,
            VkFormat format,
            VkExtent2D extent
        ) -> void;

        /**
     * Get the handle for the backbuffer resource.
     */
        [[nodiscard]] auto get_backbuffer_handle() const -> flux::ResourceHandle;

        /**
     * Check if the graph has been compiled.
     */
        [[nodiscard]] auto is_compiled() const -> bool;

        /**
     * Get the number of passes in the compiled graph.
     */
        [[nodiscard]] auto get_pass_count() const -> std::size_t;

        /**
     * Mark the graph as needing recompilation.
     * Call this when swapchain is recreated.
     */
        auto invalidate() -> void;

        /**
     * Get the render extent (from backbuffer).
     */
        [[nodiscard]] auto get_render_extent() const -> VkExtent2D;

        /**
     * Get an image view for a resource handle (if compiled).
     * @param handle Resource handle
     * @return Image view, or VK_NULL_HANDLE if not compiled or invalid handle
     */
        [[nodiscard]] auto get_image_view(flux::ResourceHandle handle) const -> VkImageView;

        /**
     * Get an image for a resource handle (if compiled).
     * @param handle Resource handle
     * @return VkImage, or VK_NULL_HANDLE if not compiled or invalid handle
     */
        [[nodiscard]] auto get_image(flux::ResourceHandle handle) const -> VkImage;

        /**
     * Get a buffer for a resource handle (if compiled).
     * @param handle Resource handle
     * @return VkBuffer, or VK_NULL_HANDLE if not compiled or invalid handle
     */
        [[nodiscard]] auto get_buffer(flux::ResourceHandle handle) const -> VkBuffer;

        /**
     * Get the buffer size for a resource handle (if compiled).
     * @param handle Resource handle
     * @return Buffer size in bytes, or 0 if not compiled or invalid handle
     */
        [[nodiscard]] auto get_buffer_size(flux::ResourceHandle handle) const -> VkDeviceSize;

    private:
        Renderer &m_renderer;
        std::unique_ptr<RenderGraphBuilder> m_builder;
        std::unique_ptr<CompiledRenderGraph> m_compiled;

        // External resources
        std::unordered_map<std::string, ExternalResource> m_external_resources;
        flux::ResourceHandle m_backbuffer_handle = flux::INVALID_RESOURCE;
        ExternalResource m_backbuffer;

        bool m_needs_recompile = false;
    };

    /**
 * Builder interface for declarative graph construction.
 * Uses fluent API for ease of use.
 */
    class THRESH_API RenderGraphBuilder {
    public:
        explicit RenderGraphBuilder(flux::Device &device);

        ~RenderGraphBuilder();

        RenderGraphBuilder(const RenderGraphBuilder &) = delete;

        RenderGraphBuilder &operator=(const RenderGraphBuilder &) = delete;

        /**
     * Create a transient image resource.
     */
        auto create_image(
            const std::string &name,
            const flux::ImageResourceDesc &desc
        ) -> flux::ResourceHandle;

        /**
     * Create a transient buffer resource.
     */
        auto create_buffer(
            const std::string &name,
            const flux::BufferResourceDesc &desc
        ) -> flux::ResourceHandle;

        /**
     * Import an external resource (e.g., swapchain, persistent texture).
     */
        auto import_external(
            const std::string &name,
            const ExternalResource &external
        ) -> flux::ResourceHandle;

        /**
     * Add a graphics pass.
     */
        auto add_graphics_pass(
            const std::string &name,
            flux::PassExecuteCallback execute
        ) -> RenderGraphBuilder &;

        /**
     * Add a compute pass.
     */
        auto add_compute_pass(
            const std::string &name,
            flux::PassExecuteCallback execute
        ) -> RenderGraphBuilder &;

        /**
     * Add a transfer pass.
     */
        auto add_transfer_pass(
            const std::string &name,
            flux::PassExecuteCallback execute
        ) -> RenderGraphBuilder &;

        // Pass configuration (fluent API - operates on current pass)

        /**
     * Declare a read dependency on a resource.
     * This version is for barrier generation only - no descriptor binding.
     */
        auto read(
            flux::ResourceHandle handle,
            flux::ResourceUsage usage
        ) -> RenderGraphBuilder &;

        /**
     * Declare a read dependency with descriptor binding.
     * This enables auto-generated descriptor sets for the pass.
     * @param handle Resource handle
     * @param usage How the resource will be used
     * @param binding Descriptor binding index
     * @param sampler Sampler for SampledImage usage (required)
     */
        auto read(
            flux::ResourceHandle handle,
            flux::ResourceUsage usage,
            uint32_t binding,
            VkSampler sampler = VK_NULL_HANDLE
        ) -> RenderGraphBuilder &;

        /**
     * Declare a write dependency on a resource.
     * This version is for barrier generation only - no descriptor binding.
     */
        auto write(
            flux::ResourceHandle handle,
            flux::ResourceUsage usage
        ) -> RenderGraphBuilder &;

        /**
     * Declare a write dependency with descriptor binding.
     * This enables auto-generated descriptor sets for the pass.
     * @param handle Resource handle
     * @param usage How the resource will be used
     * @param binding Descriptor binding index
     */
        auto write(
            flux::ResourceHandle handle,
            flux::ResourceUsage usage,
            uint32_t binding
        ) -> RenderGraphBuilder &;

        /**
     * Set a color attachment for the current graphics pass.
     * @param index Attachment index (0-7)
     * @param handle Resource handle
     * @param load_op How to initialize the attachment
     * @param clear_value Clear color (if load_op is CLEAR)
     */
        auto set_color_attachment(
            std::uint32_t index,
            flux::ResourceHandle handle,
            VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            VkClearColorValue clear_value = {{0.0f, 0.0f, 0.0f, 1.0f}}
        ) -> RenderGraphBuilder &;

        /**
     * Set the depth attachment for the current graphics pass.
     */
        auto set_depth_attachment(
            flux::ResourceHandle handle,
            VkAttachmentLoadOp load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
            VkClearDepthStencilValue clear_value = {1.0f, 0}
        ) -> RenderGraphBuilder &;

        /**
     * Set the queue for the current pass (for async compute).
     */
        auto set_queue(flux::QueueType queue) -> RenderGraphBuilder &;

        /**
     * Clear the builder for reuse.
     */
        auto clear() -> void;

        // Internal access for compilation
        [[nodiscard]] auto get_resources() const -> const std::vector<flux::ResourceDesc> &;

        [[nodiscard]] auto get_passes() -> std::vector<flux::PassDefinition> &;

        [[nodiscard]] auto
        get_external_resources() const -> const std::unordered_map<flux::ResourceHandle, ExternalResource> &;

    private:
        flux::Device &m_device;
        std::vector<flux::ResourceDesc> m_resources;
        std::vector<flux::PassDefinition> m_passes;
        std::unordered_map<flux::ResourceHandle, ExternalResource> m_externals;
        flux::PassDefinition *m_current_pass = nullptr;
        flux::ResourceHandle m_next_handle = 0;
    };

    /**
 * Compiled render graph optimized for execution.
 * Contains pre-computed barriers and resource allocations.
 */
    class THRESH_API CompiledRenderGraph {
    public:
        CompiledRenderGraph(
            flux::Device &device,
            RenderGraphBuilder &builder,
            VkInstance instance,
            VkPhysicalDevice physical_device
        );

        ~CompiledRenderGraph();

        CompiledRenderGraph(const CompiledRenderGraph &) = delete;

        CompiledRenderGraph &operator=(const CompiledRenderGraph &) = delete;

        /**
     * Execute the compiled graph.
     */
        auto execute(
            VkCommandBuffer cmd,
            std::uint32_t frame_index,
            float delta_time,
            VkExtent2D extent
        ) -> void;

        /**
     * Update an external resource (e.g., swapchain image changed).
     */
        auto update_external(
            flux::ResourceHandle handle,
            const ExternalResource &external
        ) -> void;

        /**
     * Get number of passes.
     */
        [[nodiscard]] auto get_pass_count() const -> std::size_t;

        // Resource access for pass callbacks
        [[nodiscard]] auto get_image(flux::ResourceHandle handle) const -> VkImage;

        [[nodiscard]] auto get_image_view(flux::ResourceHandle handle) const -> VkImageView;

        [[nodiscard]] auto get_buffer(flux::ResourceHandle handle) const -> VkBuffer;

        [[nodiscard]] auto get_buffer_size(flux::ResourceHandle handle) const -> VkDeviceSize;

        [[nodiscard]] auto get_image_format(flux::ResourceHandle handle) const -> VkFormat;

        [[nodiscard]] auto get_image_extent(flux::ResourceHandle handle) const -> VkExtent3D;

        /**
         * Get descriptor layout for a pass (if auto_descriptors enabled).
         */
        [[nodiscard]] auto get_pass_descriptor_layout(std::size_t pass_index) const -> VkDescriptorSetLayout;

    private:
        auto topological_sort() -> void;

        auto compute_lifetimes() -> void;

        auto allocate_resources() -> void;

        auto compute_barriers() -> void;

        auto create_pass_descriptors() -> void;

        auto update_pass_descriptors(std::uint32_t frame_index) -> void;

        auto begin_graphics_pass(VkCommandBuffer cmd, const flux::PassDefinition &pass, VkExtent2D extent) -> void;

        auto end_graphics_pass(VkCommandBuffer cmd) -> void;

        flux::Device &m_device;
        std::unique_ptr<flux::TransientAllocator> m_allocator;
        std::unique_ptr<flux::BarrierBatcher> m_barrier_batcher;

        // Resource data
        std::vector<flux::ResourceDesc> m_resources;
        std::vector<flux::PhysicalResource> m_physical_resources;
        std::unordered_map<flux::ResourceHandle, flux::ResourceLifetime> m_lifetimes;

        // Pass data (in topological order)
        std::vector<flux::PassDefinition> m_passes;

        // Pre-computed barriers per pass
        std::vector<std::vector<flux::PassBarrier> > m_pre_pass_barriers;
        std::vector<flux::PassBarrier> m_final_barriers;

        // External resource mapping
        std::unordered_map<flux::ResourceHandle, ExternalResource> m_externals;

        // Resource state tracking
        std::unordered_map<flux::ResourceHandle, flux::ResourceState> m_resource_states;

        // Auto-generated descriptors per pass
        // Layout and pool per pass (index matches m_passes)
        std::vector<VkDescriptorSetLayout> m_pass_descriptor_layouts;
        std::vector<VkDescriptorPool> m_pass_descriptor_pools;
        // Descriptor sets: [pass_index][frame_index]
        std::vector<std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT>> m_pass_descriptor_sets;
    };
} // namespace thresh