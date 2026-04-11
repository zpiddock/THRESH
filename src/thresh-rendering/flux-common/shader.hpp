//
// Created by Admin on 11/04/2026.
//

#pragma once

namespace flux {

class Shader {
    public:
        enum class Stage {
            Vertex,
            Fragment,
            Geometry,
            TessControl,
            TessEval,
            Compute
        };

        virtual ~Shader() = default;
};

} // flux
