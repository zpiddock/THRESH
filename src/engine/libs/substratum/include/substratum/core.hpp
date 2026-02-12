#pragma once

#ifdef _WIN32
    #ifdef SUBSTRATUM_EXPORTS
        #define SUBSTRATUM_API __declspec(dllexport)
    #else
        #define SUBSTRATUM_API __declspec(dllimport)
    #endif
#else
    #define SUBSTRATUM_API
#endif

namespace substratum {
    /**
 * Core initialization and shutdown for the Federation library.
 * This provides foundational utilities for the engine.
 */
    class SUBSTRATUM_API Core {
    public:
        Core() = default;

        ~Core() = default;

        Core(const Core &) = delete;

        Core &operator=(const Core &) = delete;

        Core(Core &&) = default;

        Core &operator=(Core &&) = default;

        auto initialize() -> bool;

        auto shutdown() -> void;
    };
} // namespace federation