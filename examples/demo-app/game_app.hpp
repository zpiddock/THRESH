//
// Created by Admin on 06/04/2026.
//

#pragma once
#include "thresh/application.hpp"

namespace demo {

class GameApp : public thresh::Application {
    public:
        auto update(float delta_time) -> void override;

        auto shutdown() -> void override;
};

} // demo
