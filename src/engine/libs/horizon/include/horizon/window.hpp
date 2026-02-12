#pragma once

#include "GLFW/glfw3.h"
#include <string>
#include <cstdint>

#ifdef _WIN32
#ifdef HORIZON_EXPORTS
#define HORIZON_API __declspec(dllexport)
#else
#define HORIZON_API __declspec(dllimport)
#endif
#else
#define HORIZON_API
#endif

namespace horizon {
    /**
 * RAII wrapper around GLFW window.
 * Handles window creation, destruction, and basic window operations.
 */
    class HORIZON_API Window {
    public:
        struct Config {
            std::string title = "Vulkan Application";
            std::uint32_t width = 1280;
            std::uint32_t height = 720;
            bool resizable = true;
            bool maximized = true;
        };

        explicit Window(const Config &config);

        ~Window();

        Window(const Window &) = delete;

        Window &operator=(const Window &) = delete;

        Window(Window &&) noexcept;

        Window &operator=(Window &&) noexcept;

        auto should_close() const -> bool;

        auto get_native_handle() -> GLFWwindow *;

        auto get_framebuffer_size() const -> std::pair<std::uint32_t, std::uint32_t>;

        auto poll_events() -> void;

        auto wait_events() -> void;

        auto set_input_mode(int mode, int value) -> void;

    private:
        GLFWwindow *m_window = nullptr;
        Config m_config;
    };
} // namespace borg