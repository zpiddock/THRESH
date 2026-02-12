#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

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
 * RAII wrapper for VkShaderModule with hot-reload support.
 * Monitors shader files for changes and automatically reloads them.
 */
    class FLUX_API Shader {
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
            VkDevice device = VK_NULL_HANDLE;
            std::filesystem::path filepath; // Can be .glsl or .spv
            Stage stage = Stage::Vertex;
            bool enable_hot_reload = true;
            bool optimize = true; // Optimize GLSL during compilation
            bool throw_on_error = true; // If false, compilation errors set error state instead of throwing
        };

        /**
         * Result of shader compilation attempt.
         */
        struct CompileResult {
            bool success = false;
            std::string error_message;
        };

        explicit Shader(const Config &config);

        ~Shader();

        Shader(const Shader &) = delete;

        Shader &operator=(const Shader &) = delete;

        Shader(Shader &&) noexcept;

        Shader &operator=(Shader &&) noexcept;

        auto get_module() const -> VkShaderModule { return m_shader_module; }
        auto get_stage() const -> VkShaderStageFlagBits { return stage_to_vk_flags(m_stage); }
        auto get_filepath() const -> const std::filesystem::path & { return m_filepath; }

        /**
         * Check if the shader compiled successfully.
         * Only relevant when throw_on_error is false in config.
         */
        [[nodiscard]] auto is_valid() const -> bool { return m_shader_module != VK_NULL_HANDLE; }

        /**
         * Get the last compilation error (if any).
         */
        [[nodiscard]] auto get_last_error() const -> const std::string& { return m_last_error; }

        /**
     * Manually reload the shader from disk.
     * Returns true if reload was successful.
     */
        auto reload() -> bool;

        /**
     * Check if the shader file has been modified and reload if necessary.
     * Returns true if shader was reloaded.
     */
        auto check_and_reload() -> bool;

        /**
     * Set a callback to be invoked when the shader is reloaded.
     * Useful for recreating pipelines that use this shader.
     */
        auto set_reload_callback(ReloadCallback callback) -> void { m_reload_callback = std::move(callback); }

    private:
        auto load_shader_code() -> std::vector<char>;

        auto create_shader_module(const std::vector<char> &code) -> void;

        auto cleanup_shader_module() -> void;

        static auto stage_to_vk_flags(Stage stage) -> VkShaderStageFlagBits;

        VkDevice m_device = VK_NULL_HANDLE;
        VkShaderModule m_shader_module = VK_NULL_HANDLE;
        std::filesystem::path m_filepath;
        Stage m_stage;
        bool m_hot_reload_enabled;
        bool m_throw_on_error;
        std::filesystem::file_time_type m_last_write_time;
        ReloadCallback m_reload_callback;
        std::string m_last_error;
    };
} // namespace batleth