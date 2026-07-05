//
// Created by Admin on 06/04/2026.
//

#pragma once

#include "thresh/application.hpp"
#include "thresh/debug_utils/debug_ui.hpp"
#include "thresh/editor/editor_ui.hpp"

namespace demo {

class GameApp : public thresh::Application {
    public:
        auto startup() -> void override;

        auto update(float delta_time) -> void override;

        auto render() -> void override;

        auto debug_render() -> void override;

        auto shutdown() -> void override;

    private:

        std::unique_ptr<thresh::edit::EditorUI> m_debug_ui;
};

} // demo
