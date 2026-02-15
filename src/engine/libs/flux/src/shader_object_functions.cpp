#include "flux/shader_object_functions.hpp"
#include "substratum/log.hpp"
#include <stdexcept>

namespace flux {

    namespace {
        template <typename T>
        auto load_fn(VkDevice device, const char *name) -> T {
            auto fn = reinterpret_cast<T>(::vkGetDeviceProcAddr(device, name));
            if (fn) {
                SUB_DEBUG("  Loaded: {}", name);
            } else {
                SUB_WARN("  Failed to load: {}", name);
            }
            return fn;
        }
    } // namespace

    auto ShaderObjectFunctions::load(VkDevice device) -> void {
        SUB_INFO("Loading VK_EXT_shader_object function pointers...");

        // VK_EXT_shader_object core (critical)
        create_shaders = load_fn<PFN_vkCreateShadersEXT>(device, "vkCreateShadersEXT");
        destroy_shader = load_fn<PFN_vkDestroyShaderEXT>(device, "vkDestroyShaderEXT");
        cmd_bind_shaders = load_fn<PFN_vkCmdBindShadersEXT>(device, "vkCmdBindShadersEXT");
        get_shader_binary_data = load_fn<PFN_vkGetShaderBinaryDataEXT>(device, "vkGetShaderBinaryDataEXT");

        if (!create_shaders || !destroy_shader || !cmd_bind_shaders) {
            SUB_FATAL("Failed to load critical VK_EXT_shader_object functions");
            throw std::runtime_error("Failed to load critical VK_EXT_shader_object functions");
        }

        // EXT dynamic state — rasterization
        cmd_set_polygon_mode = load_fn<PFN_vkCmdSetPolygonModeEXT>(device, "vkCmdSetPolygonModeEXT");
        cmd_set_rasterization_samples = load_fn<PFN_vkCmdSetRasterizationSamplesEXT>(device, "vkCmdSetRasterizationSamplesEXT");
        cmd_set_sample_mask = load_fn<PFN_vkCmdSetSampleMaskEXT>(device, "vkCmdSetSampleMaskEXT");
        cmd_set_alpha_to_coverage_enable = load_fn<PFN_vkCmdSetAlphaToCoverageEnableEXT>(device, "vkCmdSetAlphaToCoverageEnableEXT");
        cmd_set_alpha_to_one_enable = load_fn<PFN_vkCmdSetAlphaToOneEnableEXT>(device, "vkCmdSetAlphaToOneEnableEXT");

        // EXT dynamic state — color blend
        cmd_set_color_blend_enable = load_fn<PFN_vkCmdSetColorBlendEnableEXT>(device, "vkCmdSetColorBlendEnableEXT");
        cmd_set_color_blend_equation = load_fn<PFN_vkCmdSetColorBlendEquationEXT>(device, "vkCmdSetColorBlendEquationEXT");
        cmd_set_color_write_mask = load_fn<PFN_vkCmdSetColorWriteMaskEXT>(device, "vkCmdSetColorWriteMaskEXT");
        cmd_set_logic_op_enable = load_fn<PFN_vkCmdSetLogicOpEnableEXT>(device, "vkCmdSetLogicOpEnableEXT");

        // EXT dynamic state — depth
        cmd_set_depth_clamp_enable = load_fn<PFN_vkCmdSetDepthClampEnableEXT>(device, "vkCmdSetDepthClampEnableEXT");

        // VK_EXT_vertex_input_dynamic_state
        cmd_set_vertex_input = load_fn<PFN_vkCmdSetVertexInputEXT>(device, "vkCmdSetVertexInputEXT");

        // EXT dynamic state — tessellation
        cmd_set_patch_control_points = load_fn<PFN_vkCmdSetPatchControlPointsEXT>(device, "vkCmdSetPatchControlPointsEXT");
        cmd_set_tessellation_domain_origin = load_fn<PFN_vkCmdSetTessellationDomainOriginEXT>(device, "vkCmdSetTessellationDomainOriginEXT");

        SUB_INFO("VK_EXT_shader_object function pointers loaded successfully");
    }

    auto ShaderObjectFunctions::is_loaded() const -> bool {
        return create_shaders != nullptr
            && destroy_shader != nullptr
            && cmd_bind_shaders != nullptr;
    }

} // namespace flux
