#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/engine_config.hpp"
#include "thresh/renderer.hpp"
#include "thresh/render_graph.hpp"
#include "thresh/camera.hpp"
#include "flux/render_graph_pass.hpp"
#include "flux/render_graph_resource.hpp"
#include "flux/buffer.hpp"
#include "flux/shader_program.hpp"
#include "flux/graphics_state.hpp"
#include "flux/device.hpp"
#include "horizon/window.hpp"
#include "horizon/input.hpp"
#include "cube_vertices.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <mutex>
#include <memory>
#include <filesystem>

// Shared state between update and render threads
struct SharedRenderData {
    std::mutex mutex;
    glm::mat4 mvp = glm::mat4(1.0f);
};

auto main() -> int {
    SUB_INFO("THRESH Demo Game starting");

    substratum::Logger::set_level(substratum::LogLevel::Debug);

    auto engine_cfg = thresh::EngineConfig{
        .application_name = "THRESH Demo Game",
        .window_title = "THRΞSH Demo",
        .window_width = 1280,
        .window_height = 720,
        .window_resizable = true,
        .window_maximized = false,
        .enable_validation = true,
        .enable_shader_hot_reload = false
    };

    auto engine = thresh::Engine(engine_cfg);

    // --- Camera ---
    auto camera = thresh::Camera(engine.get_window());
    camera.set_position({0.0f, 0.0f, 3.0f});
    camera.set_fov(70.0f);
    engine.get_input().add_subscriber(&camera);

    // --- Vertex buffer ---
    auto &device = engine.get_renderer().get_device_ref();

    auto vertex_buffer = flux::Buffer(
        device,
        sizeof(demo::Vertex),
        static_cast<std::uint32_t>(demo::CUBE_VERTICES.size()),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    vertex_buffer.map();
    vertex_buffer.write_to_buffer(
        const_cast<demo::Vertex *>(demo::CUBE_VERTICES.data()),
        sizeof(demo::Vertex) * demo::CUBE_VERTICES.size()
    );

    // --- Shader program ---
    auto shader_dir = std::filesystem::current_path() / "shaders";

    auto push_range = VkPushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4)
    };

    auto shader_program = flux::ShaderProgram({
        .device = &device,
        .name = "cube_shader",
        .stages = {
            {
                .filepath = shader_dir / "cube.vert",
                .stage = flux::ShaderObject::Stage::Vertex,
                .next_stage = VK_SHADER_STAGE_FRAGMENT_BIT
            },
            {
                .filepath = shader_dir / "cube.frag",
                .stage = flux::ShaderObject::Stage::Fragment,
                .next_stage = static_cast<VkShaderStageFlagBits>(0)
            }
        },
        .set_layouts = {},
        .push_constant_ranges = {push_range},
        .mode = flux::ShaderProgram::Mode::Unlinked,
        .enable_hot_reload = false
    });

    // --- Graphics state ---
    auto gfx_state = flux::GraphicsState::create_mesh_default();

    // Vertex input: binding 0, stride = sizeof(Vertex), per-vertex
    gfx_state.vertex_bindings = {{
        .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_BINDING_DESCRIPTION_2_EXT,
        .pNext = nullptr,
        .binding = 0,
        .stride = sizeof(demo::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        .divisor = 1
    }};

    // Attributes: loc 0 = position (vec3, offset 0), loc 1 = color (vec3, offset 12)
    gfx_state.vertex_attributes = {
        {
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
            .pNext = nullptr,
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(demo::Vertex, position)
        },
        {
            .sType = VK_STRUCTURE_TYPE_VERTEX_INPUT_ATTRIBUTE_DESCRIPTION_2_EXT,
            .pNext = nullptr,
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(demo::Vertex, color)
        }
    };

    // --- Shared render data ---
    auto shared_data = std::make_shared<SharedRenderData>();

    engine.run(
        // --- Graph setup callback (called on initial build + resize) ---
        [&engine, &vertex_buffer, &shader_program, &gfx_state, shared_data](thresh::RenderGraph &graph) {
            auto &builder = graph.begin_build();
            auto backbuffer = graph.get_backbuffer_handle();

            // Create transient depth image matching swapchain extent
            auto extent = engine.get_renderer().get_swapchain_extent();
            auto depth_image = builder.create_image(
                "depth",
                flux::ImageResourceDesc::create_2d(
                    VK_FORMAT_D32_SFLOAT,
                    extent.width,
                    extent.height,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                )
            );

            // Forward pass — draws the colored cube
            builder.add_graphics_pass("forward_pass",
                    [&vertex_buffer, &shader_program, &gfx_state, shared_data](const flux::PassExecutionContext &ctx) {
                        auto cmd = ctx.command_buffer;

                        // Set viewport and scissor from render extent
                        auto vp = VkViewport{
                            .x = 0.0f,
                            .y = 0.0f,
                            .width = static_cast<float>(ctx.render_extent.width),
                            .height = static_cast<float>(ctx.render_extent.height),
                            .minDepth = 0.0f,
                            .maxDepth = 1.0f
                        };

                        auto scissor = VkRect2D{
                            .offset = {0, 0},
                            .extent = ctx.render_extent
                        };

                        // Update graphics state viewport/scissor
                        auto state = gfx_state;
                        state.viewports = {vp};
                        state.scissors = {scissor};

                        // Apply all dynamic state
                        state.apply(cmd, *ctx.device);

                        // Bind shaders
                        shader_program.bind(cmd);

                        // Push MVP
                        glm::mat4 mvp;
                        {
                            std::lock_guard lock(shared_data->mutex);
                            mvp = shared_data->mvp;
                        }

                        ::vkCmdPushConstants(
                            cmd,
                            shader_program.get_pipeline_layout(),
                            VK_SHADER_STAGE_VERTEX_BIT,
                            0,
                            sizeof(glm::mat4),
                            &mvp
                        );

                        // Bind vertex buffer
                        VkBuffer buffers[] = {vertex_buffer.get_buffer()};
                        VkDeviceSize offsets[] = {0};
                        ::vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

                        // Draw the cube (36 vertices, 1 instance)
                        ::vkCmdDraw(cmd, 36, 1, 0, 0);
                    })
                .set_color_attachment(0, backbuffer, VK_ATTACHMENT_LOAD_OP_CLEAR,
                    // Near-black background (THRESH Deep: #06080C)
                    {.float32 = {0.024f, 0.031f, 0.047f, 1.0f}})
                .set_depth_attachment(depth_image, VK_ATTACHMENT_LOAD_OP_CLEAR, {1.0f, 0});
        },
        // --- Update callback (called each tick on update thread) ---
        [&camera, &engine, shared_data](thresh::FrameSnapshot & /*snapshot*/, float dt) {
            camera.update(dt);

            auto [fb_w, fb_h] = engine.get_window().get_framebuffer_size();
            auto aspect = (fb_h > 0) ? static_cast<float>(fb_w) / static_cast<float>(fb_h) : 1.0f;

            auto view = camera.get_view_matrix();
            auto proj = camera.get_projection_matrix(aspect);
            auto model = glm::mat4(1.0f);
            auto mvp = proj * view * model;

            std::lock_guard lock(shared_data->mutex);
            shared_data->mvp = mvp;
        }
    );

    engine.get_input().remove_subscriber(&camera);

    SUB_INFO("THRESH Demo Game exited cleanly");
    return 0;
}
