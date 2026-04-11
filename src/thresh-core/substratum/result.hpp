#pragma once

#include <expected>
#include <string>
#include <functional>
#include <source_location>
#include <format>

namespace thresh {

// ============================================================================
//  Core aliases
// ============================================================================

/// Result<T> — operation that returns T or an error.
/// E defaults to std::string for quick ad-hoc errors.
/// Use a domain enum for subsystems where structured errors matter:
///   Result<FileData, IOError>
///   Result<Shader, ShaderError>
template<typename T, typename E = std::string>
using Result = std::expected<T, E>;

/// VoidResult — operation that succeeds or fails with no return value.
template<typename E = std::string>
using VoidResult = std::expected<void, E>;

// ============================================================================
//  Construction helpers
// ============================================================================

/// Wraps a value in a successful Result.
/// Typically not needed — implicit conversion handles it —
/// but useful for disambiguation in complex expressions.
template<typename T>
constexpr auto Ok(T&& value) {
    return std::forward<T>(value);
}

/// Wraps an error in a failed Result.
/// Works with strings, enums, or any error type:
///   return Err("something went wrong");
///   return Err(IOError::FileNotFound);
template<typename E>
constexpr auto Error(E&& error) {
    return std::unexpected(std::forward<E>(error));
}

/// Convenience overload — builds a formatted string error.
///   return Err("failed to open {}: {}", path, reason);
template<typename... Args>
constexpr auto Error(std::format_string<Args...> fmt, Args&&... args) {
    return std::unexpected(std::format(fmt, std::forward<Args>(args)...));
}

// ============================================================================
//  Propagation macro — equivalent to Rust's ? operator
//
//  Unwraps the value if Ok, otherwise returns the error immediately.
//  The enclosing function must return a compatible Result type.
//
//  Usage:
//    Result<Texture> LoadTexture(const std::string& path) {
//        auto file    = THRESH_TRY(ReadFile(path));
//        auto decoded = THRESH_TRY(Decode(file));
//        return Upload(decoded);
//    }
// ============================================================================

#define THRESH_TRY(expr)                                \
    ({                                                  \
        auto&& _thresh_result = (expr);                 \
        if (!_thresh_result) [[unlikely]]               \
            return ::thresh::Error(                       \
                std::move(_thresh_result).error());     \
        std::move(*_thresh_result);                     \
    })

// ============================================================================
//  Assertion helpers
//
//  Three-tier error strategy:
//
//  Tier 1 — programmer errors (precondition violations, API misuse)
//            → THRESH_ASSERT: crash loudly in debug, UB-annotated in release
//
//  Tier 2 — recoverable runtime failures (file not found, compile error)
//            → Return Result<T, E> — caller decides how to handle
//
//  Tier 3 — expected absence (cache miss, optional feature)
//            → Return std::optional<T> — no error semantics needed
// ============================================================================

#ifdef THRESH_DEBUG
    #define THRESH_ASSERT(cond, ...)                                        \
        do {                                                                \
            if (!(cond)) [[unlikely]] {                                     \
                auto loc = std::source_location::current();                 \
                /* Replace with your logger — e.g. THRESH_LOG_CRITICAL */  \
                std::fprintf(stderr,                                        \
                    "[THRESH ASSERT] %s:%u in %s\n  Condition: %s\n  " __VA_ARGS__ "\n", \
                    loc.file_name(), loc.line(), loc.function_name(), #cond); \
                std::abort();                                               \
            }                                                               \
        } while (false)
#else
    #define THRESH_ASSERT(cond, ...) \
        do { if (!(cond)) [[unlikely]] __builtin_unreachable(); } while (false)
#endif


// ============================================================================
//  Error mapping utility
//
//  Maps an error from one domain to another at subsystem boundaries.
//  Prevents lower-level error types leaking up the call stack.
//
//  Usage:
//    auto file = ReadFile(path)
//        .map_error(MapErr<IOError, AssetError>({
//            { IOError::FileNotFound, AssetError::NotFound },
//            { IOError::ReadFailure,  AssetError::CorruptData },
//        }));
// ============================================================================

template<typename From, typename To>
struct MapErr {
    std::initializer_list<std::pair<From, To>> mappings;
    To fallback;

    To operator()(From err) const {
        for (auto& [from, to] : mappings)
            if (from == err) return to;
        return fallback;
    }
};

} // namespace thresh


// ============================================================================
//  Usage reference
// ============================================================================
//
//  --- Basic usage (string errors) ---
//
//  thresh::Result<Texture> LoadTexture(const std::string& path) {
//      if (!std::filesystem::exists(path))
//          return thresh::Err("Texture not found: {}", path);
//      // ...
//      return texture;
//  }
//
//  --- Domain error types ---
//
//  thresh::Result<FileData, IOError> ReadFile(const std::string& path) {
//      if (!std::filesystem::exists(path))
//          return thresh::Err(IOError::FileNotFound);
//      // ...
//      return data;
//  }
//
//  --- Propagation with THRESH_TRY ---
//
//  thresh::Result<Texture> LoadTexture(const std::string& path) {
//      auto file    = THRESH_TRY(ReadFile(path));
//      auto decoded = THRESH_TRY(Decode(file));
//      return Upload(decoded);
//  }
//
//  --- Monadic chaining (C++23) ---
//
//  thresh::Result<Texture> LoadTexture(const std::string& path) {
//      return ReadFile(path)
//          .and_then(Decode)
//          .and_then(Upload);
//  }
//
//  --- Error mapping at boundaries ---
//
//  thresh::Result<Texture, AssetError> LoadTexture(const std::string& path) {
//      auto file = ReadFile(path).map_error([](IOError e) {
//          switch (e) {
//              case IOError::FileNotFound: return AssetError::NotFound;
//              default:                   return AssetError::CorruptData;
//          }
//      });
//      auto data = THRESH_TRY(file);
//      return Upload(data);
//  }
//
//  --- Consuming results ---
//
//  auto tex = LoadTexture("albedo.ktx2");
//  if (!tex) {
//      THRESH_LOG_ERROR("Load failed: {}", tex.error());
//      return;
//  }
//  Use(*tex);
//
//  // Or with a fallback
//  auto tex = LoadTexture("albedo.ktx2").value_or(m_fallbackTexture);
//
//  --- Assertions (Tier 1 — programmer errors only) ---
//
//  THRESH_ASSERT(mesh != nullptr, "Mesh cannot be null");