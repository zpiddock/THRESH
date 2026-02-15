#pragma once

#include "flux/shader_object.hpp"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
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
     * Groups related shader stages into a program. Manages both linked (optimized,
     * release) and unlinked (hot-reloadable, dev) shader object variants.
     *
     * In Unlinked mode: each stage is an independent VkShaderEXT.
     *   Hot-reload replaces individual stages without touching others.
     *
     * In Linked mode: all stages are created together via VK_SHADER_CREATE_LINK_STAGE_BIT_EXT.
     *   Better driver optimization; no hot-reload support.
     *
     * Owns the VkPipelineLayout since shader objects require it for descriptor/push
     * constant compatibility.
     */
    class FLUX_API ShaderProgram {
    public:
        enum class Mode {
            Unlinked,  // Development: individual shader objects, hot-reloadable
            Linked     // Release: linked shader objects, optimized
        };

        struct StageConfig {
            std::filesystem::path filepath;
            ShaderObject::Stage stage;
            VkShaderStageFlagBits next_stage = static_cast<VkShaderStageFlagBits>(0);
        };

        struct Config {
            Device *device = nullptr;
            std::string name;
            std::vector<StageConfig> stages;
            std::vector<VkDescriptorSetLayout> set_layouts;
            std::vector<VkPushConstantRange> push_constant_ranges;
            Mode mode = Mode::Unlinked;
            bool enable_hot_reload = true;
        };

        explicit ShaderProgram(const Config &config);
        ~ShaderProgram();

        ShaderProgram(const ShaderProgram &) = delete;
        auto operator=(const ShaderProgram &) -> ShaderProgram & = delete;

        ShaderProgram(ShaderProgram &&) noexcept;
        auto operator=(ShaderProgram &&) noexcept -> ShaderProgram &;

        /**
         * Bind all shader stages to the command buffer.
         * Issues a single vkCmdBindShadersEXT call with all stages.
         */
        auto bind(VkCommandBuffer cmd) const -> void;

        /**
         * Check all stages for file modifications and reload as needed.
         * Only effective in Unlinked mode.
         * Returns true if any stage was reloaded.
         */
        auto check_and_reload() -> bool;

        [[nodiscard]] auto get_pipeline_layout() const -> VkPipelineLayout { return m_pipeline_layout; }
        [[nodiscard]] auto get_name() const -> const std::string & { return m_name; }
        [[nodiscard]] auto get_mode() const -> Mode { return m_mode; }
        [[nodiscard]] auto is_valid() const -> bool;

    private:
        auto create_pipeline_layout() -> void;
        auto create_linked_shaders() -> void;
        auto destroy_linked_shaders() -> void;
        auto cleanup() -> void;

        Device *m_device = nullptr;
        std::string m_name;
        Mode m_mode;

        // Pipeline layout (shared by all stages)
        VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayout> m_set_layouts;
        std::vector<VkPushConstantRange> m_push_constant_ranges;

        // Stage configs (preserved for linked shader creation)
        std::vector<StageConfig> m_stage_configs;

        // Unlinked mode: owns individual ShaderObjects
        std::vector<std::unique_ptr<ShaderObject>> m_unlinked_shaders;

        // Linked mode: owns VkShaderEXT handles directly (created in one batch)
        std::vector<VkShaderEXT> m_linked_shaders;
        std::vector<VkShaderStageFlagBits> m_linked_stages;

        // Stage flags for bind() (used in both modes)
        std::vector<VkShaderStageFlagBits> m_stage_flags;

        bool m_hot_reload_enabled;
    };

} // namespace flux
