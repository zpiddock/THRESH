#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

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
 * File change event types
 */
enum class FileChangeType {
    Modified,
    Created,
    Deleted,
    Renamed
};

/**
 * Information about a file change event
 */
struct FileChangeEvent {
    std::filesystem::path path;
    FileChangeType type;
};

/**
 * Watches a directory for file changes using ReadDirectoryChangesW (Windows).
 *
 * Features:
 * - Non-blocking polling
 * - Debouncing of rapid changes
 * - Optional file extension filtering
 */
class SUBSTRATUM_API FileWatcher {
public:
    struct Config {
        std::filesystem::path directory;
        bool watch_subdirectories = true;
        std::chrono::milliseconds debounce_time{100};
        std::vector<std::string> extensions_filter;  // Empty = watch all
    };

    explicit FileWatcher(const Config& config);
    ~FileWatcher();

    // Non-copyable
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /**
     * Poll for file changes (non-blocking).
     * Returns changed files since last poll, with debouncing applied.
     */
    [[nodiscard]] auto poll() -> std::vector<FileChangeEvent>;

    /**
     * Check if path matches extension filter.
     */
    [[nodiscard]] auto matches_filter(const std::filesystem::path& path) const -> bool;

    /**
     * Get watched directory.
     */
    [[nodiscard]] auto get_directory() const -> const std::filesystem::path& { return m_config.directory; }

    /**
     * Check if watcher is valid/active.
     */
    [[nodiscard]] auto is_valid() const -> bool { return m_valid; }

private:
    void start_watch();
    void process_events();

    Config m_config;
    bool m_valid = false;

#ifdef _WIN32
    HANDLE m_directory_handle = INVALID_HANDLE_VALUE;
    OVERLAPPED m_overlapped{};
    std::vector<uint8_t> m_buffer;
    bool m_pending_read = false;
#endif

    // Debouncing: track last change time per file
    struct PendingChange {
        FileChangeType type;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::unordered_map<std::string, PendingChange> m_pending_changes;
};

} // namespace federation
