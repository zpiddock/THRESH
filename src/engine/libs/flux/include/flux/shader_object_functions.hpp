#pragma once

#include <vulkan/vulkan.h>

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
     * Holds dynamically loaded function pointers for VK_EXT_shader_object,
     * VK_EXT_vertex_input_dynamic_state, and related EXT dynamic state commands.
     *
     * Core Vulkan 1.3 dynamic state functions (vkCmdSetCullMode, vkCmdSetDepthTestEnable,
     * vkCmdSetViewportWithCount, etc.) are NOT included here — they are available
     * directly without vkGetDeviceProcAddr.
     *
     * Loaded once after device creation via load().
     */
    struct FLUX_API ShaderObjectFunctions {
        // VK_EXT_shader_object core
        PFN_vkCreateShadersEXT create_shaders = nullptr;
        PFN_vkDestroyShaderEXT destroy_shader = nullptr;
        PFN_vkCmdBindShadersEXT cmd_bind_shaders = nullptr;
        PFN_vkGetShaderBinaryDataEXT get_shader_binary_data = nullptr;

        // EXT dynamic state — rasterization
        PFN_vkCmdSetPolygonModeEXT cmd_set_polygon_mode = nullptr;
        PFN_vkCmdSetRasterizationSamplesEXT cmd_set_rasterization_samples = nullptr;
        PFN_vkCmdSetSampleMaskEXT cmd_set_sample_mask = nullptr;
        PFN_vkCmdSetAlphaToCoverageEnableEXT cmd_set_alpha_to_coverage_enable = nullptr;
        PFN_vkCmdSetAlphaToOneEnableEXT cmd_set_alpha_to_one_enable = nullptr;

        // EXT dynamic state — color blend
        PFN_vkCmdSetColorBlendEnableEXT cmd_set_color_blend_enable = nullptr;
        PFN_vkCmdSetColorBlendEquationEXT cmd_set_color_blend_equation = nullptr;
        PFN_vkCmdSetColorWriteMaskEXT cmd_set_color_write_mask = nullptr;
        PFN_vkCmdSetLogicOpEnableEXT cmd_set_logic_op_enable = nullptr;

        // EXT dynamic state — depth
        PFN_vkCmdSetDepthClampEnableEXT cmd_set_depth_clamp_enable = nullptr;

        // VK_EXT_vertex_input_dynamic_state
        PFN_vkCmdSetVertexInputEXT cmd_set_vertex_input = nullptr;

        // EXT dynamic state — tessellation
        PFN_vkCmdSetPatchControlPointsEXT cmd_set_patch_control_points = nullptr;
        PFN_vkCmdSetTessellationDomainOriginEXT cmd_set_tessellation_domain_origin = nullptr;

        /**
         * Load all function pointers from the given device.
         * Throws std::runtime_error if critical functions (create/destroy/bind) cannot be loaded.
         */
        auto load(VkDevice device) -> void;

        /**
         * Check if all critical functions were loaded successfully.
         */
        [[nodiscard]] auto is_loaded() const -> bool;
    };

} // namespace flux
