#pragma once

#include "horizon/input.hpp"

#include <glm/glm.hpp>
#include <mutex>

#ifdef _WIN32
#ifdef THRESH_EXPORTS
#define THRESH_API __declspec(dllexport)
#else
#define THRESH_API __declspec(dllimport)
#endif
#else
#define THRESH_API
#endif

namespace horizon {
    class Window;
} // namespace horizon

namespace thresh {

    /**
     * Base first-person camera with WASD movement and mouse look.
     *
     * Implements horizon::IInputSubscriber to receive mouse events.
     * Mouse look is activated by holding the right mouse button.
     *
     * Thread safety: mouse deltas are accumulated under a mutex so that
     * on_mouse_move (called on the main/GLFW thread) and update (called
     * on the update thread) can safely share data.
     */
    class THRESH_API Camera : public horizon::IInputSubscriber {
    public:
        explicit Camera(horizon::Window &window);
        ~Camera() override = default;

        Camera(const Camera &) = delete;
        Camera &operator=(const Camera &) = delete;

        /**
         * Update camera position and orientation.
         * Call once per tick on the update thread.
         * @param dt Delta time in seconds
         */
        auto update(float dt) -> void;

        /**
         * Get the view matrix (world -> camera).
         */
        [[nodiscard]] auto get_view_matrix() const -> glm::mat4;

        /**
         * Get the projection matrix (camera -> clip).
         * @param aspect_ratio Width / Height
         */
        [[nodiscard]] auto get_projection_matrix(float aspect_ratio) const -> glm::mat4;

        // Position
        auto set_position(const glm::vec3 &pos) -> void { m_position = pos; }
        [[nodiscard]] auto get_position() const -> const glm::vec3 & { return m_position; }

        // Orientation
        auto set_yaw(float yaw) -> void { m_yaw = yaw; }
        auto set_pitch(float pitch) -> void { m_pitch = pitch; }
        [[nodiscard]] auto get_yaw() const -> float { return m_yaw; }
        [[nodiscard]] auto get_pitch() const -> float { return m_pitch; }

        // Configuration
        auto set_move_speed(float speed) -> void { m_move_speed = speed; }
        auto set_sensitivity(float sens) -> void { m_sensitivity = sens; }
        auto set_fov(float fov_degrees) -> void { m_fov = fov_degrees; }
        auto set_near_far(float near_plane, float far_plane) -> void { m_near = near_plane; m_far = far_plane; }

        [[nodiscard]] auto get_move_speed() const -> float { return m_move_speed; }
        [[nodiscard]] auto get_sensitivity() const -> float { return m_sensitivity; }
        [[nodiscard]] auto get_fov() const -> float { return m_fov; }

        // Input subscriber callbacks (called on main thread)
        auto on_mouse_move(GLFWwindow *window, double xpos, double ypos) -> void override;
        auto on_mouse_button(GLFWwindow *window, int button, int action, int mods) -> void override;

    private:
        [[nodiscard]] auto calculate_forward() const -> glm::vec3;
        [[nodiscard]] auto calculate_right() const -> glm::vec3;

        horizon::Window &m_window;

        // Camera state
        glm::vec3 m_position = {0.0f, 0.0f, 0.0f};
        float m_yaw = -90.0f;    // Look along -Z by default
        float m_pitch = 0.0f;

        // Projection parameters
        float m_fov = 70.0f;     // Degrees
        float m_near = 0.1f;
        float m_far = 1000.0f;

        // Movement
        float m_move_speed = 5.0f;
        float m_sensitivity = 0.1f;

        // Mouse state (main thread writes, update thread reads)
        std::mutex m_mouse_mutex;
        float m_mouse_dx = 0.0f;
        float m_mouse_dy = 0.0f;
        double m_last_mouse_x = 0.0;
        double m_last_mouse_y = 0.0;
        bool m_mouse_captured = false;
        bool m_first_mouse = true;
    };

} // namespace thresh
