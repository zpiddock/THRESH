//
// Created by Admin on 06/04/2026.
//

#pragma once

namespace thresh {

class Application {
    public:
        virtual ~Application() = default;

        virtual auto startup() -> void {}

        virtual auto update(float delta_time) -> void = 0;

        virtual auto render() -> void {}

        virtual auto shutdown() -> void;
};

} // thresh
