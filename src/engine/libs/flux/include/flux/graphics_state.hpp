#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

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

    class Device;

    /**
     * Encapsulates all dynamic graphics pipeline state required by VK_EXT_shader_object.
     *
     * When using shader objects, ALL graphics state must be set dynamically before each draw.
     * This struct provides sensible defaults and a single apply() call to record all state
     * into a command buffer.
     *
     * Lightweight and copyable — no GPU resources owned.
     */
    struct FLUX_API GraphicsState {
        // --- Viewport / Scissor ---
        std::vector<VkViewport> viewports = {{}};
        std::vector<VkRect2D> scissors = {{}};

        // --- Input Assembly ---
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkBool32 primitive_restart_enable = VK_FALSE;

        // --- Vertex Input (VK_EXT_vertex_input_dynamic_state) ---
        std::vector<VkVertexInputBindingDescription2EXT> vertex_bindings;
        std::vector<VkVertexInputAttributeDescription2EXT> vertex_attributes;

        // --- Rasterization ---
        VkBool32 rasterizer_discard_enable = VK_FALSE;
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
        VkBool32 depth_bias_enable = VK_FALSE;
        VkBool32 depth_clamp_enable = VK_FALSE;

        // --- Multisampling ---
        VkSampleCountFlagBits rasterization_samples = VK_SAMPLE_COUNT_1_BIT;
        VkSampleMask sample_mask = ~0u;
        VkBool32 alpha_to_coverage_enable = VK_FALSE;
        VkBool32 alpha_to_one_enable = VK_FALSE;

        // --- Depth / Stencil ---
        VkBool32 depth_test_enable = VK_FALSE;
        VkBool32 depth_write_enable = VK_FALSE;
        VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
        VkBool32 depth_bounds_test_enable = VK_FALSE;
        VkBool32 stencil_test_enable = VK_FALSE;
        VkStencilOpState stencil_front = {};
        VkStencilOpState stencil_back = {};

        // --- Color Blend (per-attachment) ---
        std::vector<VkBool32> color_blend_enables = {VK_FALSE};
        std::vector<VkColorBlendEquationEXT> color_blend_equations = {{}};
        std::vector<VkColorComponentFlags> color_write_masks = {
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };
        VkBool32 logic_op_enable = VK_FALSE;

        /**
         * Apply all dynamic state to the command buffer.
         * Must be called before every draw when using shader objects.
         */
        auto apply(VkCommandBuffer cmd, const Device &device) const -> void;

        /**
         * Default state for a fullscreen triangle pass (no vertex input, no depth).
         */
        static auto create_fullscreen_default() -> GraphicsState;

        /**
         * Default state for a standard 3D mesh pass (depth test + write, backface culling).
         */
        static auto create_mesh_default() -> GraphicsState;
    };

} // namespace flux
