#include "substratum/log.hpp"
#include "thresh/engine.hpp"
#include "thresh/engine_config.hpp"
#include "thresh/render_graph.hpp"
#include "flux/render_graph_pass.hpp"

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

    engine.run(
        // Graph setup: single pass that clears the backbuffer to cyan
        [](thresh::RenderGraph &graph) {
            auto &builder = graph.begin_build();
            auto backbuffer = graph.get_backbuffer_handle();

            // Clear pass - clears backbuffer to THRESH brand cyan (#00F0FF)
            builder.add_graphics_pass("clear_pass", [](const flux::PassExecutionContext &ctx) {
                    // No draw commands needed - the render graph handles
                    // vkCmdBeginRendering with the clear load op automatically
                })
                .set_color_attachment(0, backbuffer, VK_ATTACHMENT_LOAD_OP_CLEAR,
                    {.float32 = {0.0f / 255.0f, 240.0f / 255.0f, 255.0f / 255.0f, 1.0f}});
        },
        // Update: nothing to do yet
        [](thresh::FrameSnapshot & /*snapshot*/, float /*delta_time*/) {
        }
    );

    SUB_INFO("THRESH Demo Game exited cleanly");
    return 0;
}
