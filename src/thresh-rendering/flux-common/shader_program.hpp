//
// Created by Admin on 11/04/2026.
//

#pragma once

namespace flux {

class ShaderProgram {
    public:
        virtual ~ShaderProgram() = default;

        virtual auto bind() -> void = 0;

        virtual auto unbind() -> void = 0;
};

} // flux
