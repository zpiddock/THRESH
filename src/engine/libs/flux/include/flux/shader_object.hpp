#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <filesystem>
#include <functional>
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
     * RAII wrapper for VkShaderEXT (VK_EXT_shader_object).
     * Replaces the old VkShaderModule-based Shader class.
     *
     * Supports hot-reload: detects file changes and recreates the shader object
     * without requiring pipeline recreation or GPU stalls.
     */
    class FLUX_API ShaderObject {
    public:
        enum class Stage {
            Vertex,
            Fragment,
            Compute,
            Geometry,
            TessellationControl,
            TessellationEvaluation
        };

        using ReloadCallback = std::function<void()>;

        struct Config {
            Device *device = nullptr;
            std::filesystem::path filepath;
            Stage stage = Stage::Vertex;
            VkShaderStageFlagBits next_stage = static_cast<VkShaderStageFlagBits>(0);
            std::vector<VkDescriptorSetLayout> set_layouts;
            std::vector<VkPushConstantRange> push_constant_ranges;
            bool enable_hot_reload = true;
            bool optimize = true;
            bool throw_on_error = true;
        };

        struct CompileResult {
            bool success = false;
            std::string error_message;
        };

        explicit ShaderObject(const Config &config);
        ~ShaderObject();

        ShaderObject(const ShaderObject &) = delete;
        auto operator=(const ShaderObject &) -> ShaderObject & = delete;

        ShaderObject(ShaderObject &&) noexcept;
        auto operator=(ShaderObject &&) noexcept -> ShaderObject &;

        [[nodiscard]] auto get_handle() const -> VkShaderEXT { return m_shader; }
        [[nodiscard]] auto get_stage() const -> VkShaderStageFlagBits;
        [[nodiscard]] auto get_filepath() const -> const std::filesystem::path & { return m_filepath; }
        [[nodiscard]] auto is_valid() const -> bool { return m_shader != VK_NULL_HANDLE; }
        [[nodiscard]] auto get_last_error() const -> const std::string & { return m_last_error; }

        /**
         * Get the retained SPIR-V bytecode.
         * Needed by ShaderProgram for linked shader creation.
         */
        [[nodiscard]] auto get_spirv() const -> const std::vector<std::uint32_t> & { return m_spirv; }

        /**
         * Manually reload the shader from disk.
         * Creates a new VkShaderEXT, swaps the handle, fires the callback.
         * The old handle should be queued for deferred deletion by the callback owner.
         * Returns true if reload was successful.
         */
        auto reload() -> bool;

        /**
         * Check if the shader file has been modified and reload if necessary.
         * Returns true if shader was reloaded.
         */
        auto check_and_reload() -> bool;

        auto set_reload_callback(ReloadCallback callback) -> void {
            m_reload_callback = std::move(callback);
        }

        static auto stage_to_vk(Stage stage) -> VkShaderStageFlagBits;

    private:
        auto load_spirv() -> std::vector<std::uint32_t>;
        auto create_shader_object(const std::vector<std::uint32_t> &spirv) -> void;
        auto destroy_shader_object() -> void;

        Device *m_device = nullptr;
        VkShaderEXT m_shader = VK_NULL_HANDLE;
        std::filesystem::path m_filepath;
        Stage m_stage;
        VkShaderStageFlagBits m_next_stage;
        std::vector<VkDescriptorSetLayout> m_set_layouts;
        std::vector<VkPushConstantRange> m_push_constant_ranges;
        bool m_hot_reload_enabled;
        bool m_optimize;
        bool m_throw_on_error;
        std::filesystem::file_time_type m_last_write_time;
        ReloadCallback m_reload_callback;
        std::string m_last_error;
        std::vector<std::uint32_t> m_spirv;
    };

} // namespace flux
