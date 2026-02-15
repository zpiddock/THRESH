#include "thresh/camera.hpp"
#include "horizon/window.hpp"
#include "horizon/input.hpp"
#include "substratum/log.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace thresh {

    Camera::Camera(horizon::Window &window)
        : m_window{window} {
    }

    auto Camera::update(float dt) -> void {
        // Read and reset mouse deltas (thread-safe)
        float dx = 0.0f;
        float dy = 0.0f;
        {
            std::lock_guard lock(m_mouse_mutex);
            dx = m_mouse_dx;
            dy = m_mouse_dy;
            m_mouse_dx = 0.0f;
            m_mouse_dy = 0.0f;
        }

        // Apply mouse look
        // GLFW: mouse-down = positive dy, but we want mouse-down = look-down (negative pitch)
        // The Vulkan Y-flip in projection doesn't affect world-space directions,
        // so we negate dy here for natural mouse feel.
        m_yaw += dx * m_sensitivity;
        m_pitch -= dy * m_sensitivity;
        m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

        // Calculate movement directions
        auto forward = calculate_forward();
        auto right = calculate_right();
        auto up = glm::vec3{0.0f, 1.0f, 0.0f};

        // Poll keyboard for movement (Input::is_key_pressed is thread-safe)
        auto *glfw_window = m_window.get_native_handle();
        auto speed = m_move_speed * dt;

        if (::glfwGetKey(glfw_window, GLFW_KEY_W) == GLFW_PRESS) {
            m_position += forward * speed;
        }
        if (::glfwGetKey(glfw_window, GLFW_KEY_S) == GLFW_PRESS) {
            m_position -= forward * speed;
        }
        if (::glfwGetKey(glfw_window, GLFW_KEY_D) == GLFW_PRESS) {
            m_position += right * speed;
        }
        if (::glfwGetKey(glfw_window, GLFW_KEY_A) == GLFW_PRESS) {
            m_position -= right * speed;
        }
        if (::glfwGetKey(glfw_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            m_position += up * speed;
        }
        if (::glfwGetKey(glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            m_position -= up * speed;
        }
    }

    auto Camera::get_view_matrix() const -> glm::mat4 {
        auto forward = calculate_forward();
        return glm::lookAt(m_position, m_position + forward, glm::vec3{0.0f, 1.0f, 0.0f});
    }

    auto Camera::get_projection_matrix(float aspect_ratio) const -> glm::mat4 {
        auto proj = glm::perspective(glm::radians(m_fov), aspect_ratio, m_near, m_far);
        // Flip Y for Vulkan's NDC (Y points down in clip space)
        proj[1][1] *= -1.0f;
        return proj;
    }

    auto Camera::on_mouse_move(GLFWwindow * /*window*/, double xpos, double ypos) -> void {
        if (!m_mouse_captured) {
            return;
        }

        if (m_first_mouse) {
            m_last_mouse_x = xpos;
            m_last_mouse_y = ypos;
            m_first_mouse = false;
            return;
        }

        auto dx = static_cast<float>(xpos - m_last_mouse_x);
        auto dy = static_cast<float>(ypos - m_last_mouse_y);
        m_last_mouse_x = xpos;
        m_last_mouse_y = ypos;

        std::lock_guard lock(m_mouse_mutex);
        m_mouse_dx += dx;
        m_mouse_dy += dy;
    }

    auto Camera::on_mouse_button(GLFWwindow* /*window*/, int button, int action, int /*mods*/) -> void {
        if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            if (action == GLFW_PRESS) {
                m_mouse_captured = true;
                m_first_mouse = true;
                m_window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else if (action == GLFW_RELEASE) {
                m_mouse_captured = false;
                m_window.set_input_mode(GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }

    auto Camera::calculate_forward() const -> glm::vec3 {
        auto yaw_rad = glm::radians(m_yaw);
        auto pitch_rad = glm::radians(m_pitch);

        return glm::normalize(glm::vec3{
            std::cos(yaw_rad) * std::cos(pitch_rad),
            std::sin(pitch_rad),
            std::sin(yaw_rad) * std::cos(pitch_rad)
        });
    }

    auto Camera::calculate_right() const -> glm::vec3 {
        auto forward = calculate_forward();
        return glm::normalize(glm::cross(forward, glm::vec3{0.0f, 1.0f, 0.0f}));
    }

} // namespace thresh
