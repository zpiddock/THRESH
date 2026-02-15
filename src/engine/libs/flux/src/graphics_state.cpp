#include "flux/graphics_state.hpp"
#include "flux/device.hpp"
#include "substratum/log.hpp"

namespace flux {

    auto GraphicsState::apply(VkCommandBuffer cmd, const Device &device) const -> void {
        const auto &fn = device.get_shader_object_fn();

        // --- Viewport / Scissor (Vulkan 1.3 core) ---
        ::vkCmdSetViewportWithCount(cmd,
            static_cast<std::uint32_t>(viewports.size()),
            viewports.data());

        ::vkCmdSetScissorWithCount(cmd,
            static_cast<std::uint32_t>(scissors.size()),
            scissors.data());

        // --- Input Assembly (Vulkan 1.3 core) ---
        ::vkCmdSetPrimitiveTopology(cmd, topology);
        ::vkCmdSetPrimitiveRestartEnable(cmd, primitive_restart_enable);

        // --- Vertex Input (VK_EXT_vertex_input_dynamic_state) ---
        fn.cmd_set_vertex_input(cmd,
            static_cast<std::uint32_t>(vertex_bindings.size()),
            vertex_bindings.empty() ? nullptr : vertex_bindings.data(),
            static_cast<std::uint32_t>(vertex_attributes.size()),
            vertex_attributes.empty() ? nullptr : vertex_attributes.data());

        // --- Rasterization (Vulkan 1.3 core) ---
        ::vkCmdSetRasterizerDiscardEnable(cmd, rasterizer_discard_enable);
        ::vkCmdSetCullMode(cmd, cull_mode);
        ::vkCmdSetFrontFace(cmd, front_face);
        ::vkCmdSetDepthBiasEnable(cmd, depth_bias_enable);

        // --- Rasterization (EXT) ---
        fn.cmd_set_polygon_mode(cmd, polygon_mode);
        fn.cmd_set_depth_clamp_enable(cmd, depth_clamp_enable);

        // --- Multisampling (EXT) ---
        fn.cmd_set_rasterization_samples(cmd, rasterization_samples);
        fn.cmd_set_sample_mask(cmd, rasterization_samples, &sample_mask);
        fn.cmd_set_alpha_to_coverage_enable(cmd, alpha_to_coverage_enable);
        fn.cmd_set_alpha_to_one_enable(cmd, alpha_to_one_enable);

        // --- Depth / Stencil (Vulkan 1.3 core) ---
        ::vkCmdSetDepthTestEnable(cmd, depth_test_enable);
        ::vkCmdSetDepthWriteEnable(cmd, depth_write_enable);
        ::vkCmdSetDepthCompareOp(cmd, depth_compare_op);
        ::vkCmdSetDepthBoundsTestEnable(cmd, depth_bounds_test_enable);
        ::vkCmdSetStencilTestEnable(cmd, stencil_test_enable);

        ::vkCmdSetStencilOp(cmd, VK_STENCIL_FACE_FRONT_BIT,
            stencil_front.failOp, stencil_front.passOp,
            stencil_front.depthFailOp, stencil_front.compareOp);

        ::vkCmdSetStencilOp(cmd, VK_STENCIL_FACE_BACK_BIT,
            stencil_back.failOp, stencil_back.passOp,
            stencil_back.depthFailOp, stencil_back.compareOp);

        // --- Color Blend (EXT) ---
        auto attachment_count = static_cast<std::uint32_t>(color_blend_enables.size());

        fn.cmd_set_color_blend_enable(cmd, 0, attachment_count, color_blend_enables.data());
        fn.cmd_set_color_blend_equation(cmd, 0, attachment_count, color_blend_equations.data());
        fn.cmd_set_color_write_mask(cmd, 0, attachment_count, color_write_masks.data());
        fn.cmd_set_logic_op_enable(cmd, logic_op_enable);
    }

    auto GraphicsState::create_fullscreen_default() -> GraphicsState {
        GraphicsState state;
        state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        state.cull_mode = VK_CULL_MODE_NONE;
        state.depth_test_enable = VK_FALSE;
        state.depth_write_enable = VK_FALSE;
        return state;
    }

    auto GraphicsState::create_mesh_default() -> GraphicsState {
        GraphicsState state;
        state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        state.cull_mode = VK_CULL_MODE_BACK_BIT;
        state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        state.depth_test_enable = VK_TRUE;
        state.depth_write_enable = VK_TRUE;
        state.depth_compare_op = VK_COMPARE_OP_LESS;

        // Default alpha blending
        state.color_blend_enables = {VK_TRUE};
        state.color_blend_equations = {{
            VK_BLEND_FACTOR_SRC_ALPHA,
            VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            VK_BLEND_OP_ADD,
            VK_BLEND_FACTOR_ONE,
            VK_BLEND_FACTOR_ZERO,
            VK_BLEND_OP_ADD
        }};
        state.color_write_masks = {
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
            | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
        };

        return state;
    }

} // namespace flux
