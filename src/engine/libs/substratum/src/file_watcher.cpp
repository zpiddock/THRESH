#include "substratum/file_watcher.hpp"
#include "substratum/log.hpp"
#include <algorithm>

namespace substratum {

FileWatcher::FileWatcher(const Config& config)
    : m_config(config)
    , m_buffer(64 * 1024)  // 64KB buffer for change notifications
{
    if (!std::filesystem::exists(m_config.directory)) {
        SUB_ERROR("FileWatcher: Directory does not exist: {}", m_config.directory.string());
        return;
    }

    start_watch();
}

FileWatcher::~FileWatcher() {
#ifdef _WIN32
    if (m_directory_handle != INVALID_HANDLE_VALUE) {
        CancelIo(m_directory_handle);
        CloseHandle(m_directory_handle);
    }
    if (m_overlapped.hEvent != nullptr) {
        CloseHandle(m_overlapped.hEvent);
    }
#endif
}

void FileWatcher::start_watch() {
#ifdef _WIN32
    // Open directory for monitoring
    m_directory_handle = CreateFileW(
        m_config.directory.wstring().c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr
    );

    if (m_directory_handle == INVALID_HANDLE_VALUE) {
        SUB_ERROR("FileWatcher: Failed to open directory: {}", m_config.directory.string());
        return;
    }

    // Create event for overlapped I/O
    m_overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (m_overlapped.hEvent == nullptr) {
        SUB_ERROR("FileWatcher: Failed to create event");
        CloseHandle(m_directory_handle);
        m_directory_handle = INVALID_HANDLE_VALUE;
        return;
    }

    // Start async read
    DWORD bytes_returned = 0;
    BOOL success = ReadDirectoryChangesW(
        m_directory_handle,
        m_buffer.data(),
        static_cast<DWORD>(m_buffer.size()),
        m_config.watch_subdirectories ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION,
        &bytes_returned,
        &m_overlapped,
        nullptr
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        SUB_ERROR("FileWatcher: ReadDirectoryChangesW failed");
        CloseHandle(m_overlapped.hEvent);
        CloseHandle(m_directory_handle);
        m_directory_handle = INVALID_HANDLE_VALUE;
        return;
    }

    m_pending_read = true;
    m_valid = true;
    SUB_DEBUG("FileWatcher: Watching directory: {}", m_config.directory.string());
#else
    FED_WARN("FileWatcher: Not implemented for this platform");
#endif
}

void FileWatcher::process_events() {
#ifdef _WIN32
    if (!m_pending_read || m_directory_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    // Check if read completed (non-blocking)
    DWORD bytes_transferred = 0;
    BOOL result = GetOverlappedResult(m_directory_handle, &m_overlapped, &bytes_transferred, FALSE);

    if (!result) {
        if (GetLastError() == ERROR_IO_INCOMPLETE) {
            return;  // Still pending, that's fine
        }
        SUB_ERROR("FileWatcher: GetOverlappedResult failed");
        return;
    }

    // Process the buffer
    if (bytes_transferred > 0) {
        auto* info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(m_buffer.data());
        auto now = std::chrono::steady_clock::now();

        while (true) {
            // Extract filename
            std::wstring filename_w(info->FileName, info->FileNameLength / sizeof(wchar_t));
            std::filesystem::path filepath = m_config.directory / filename_w;
            std::string filepath_str = filepath.string();

            // Determine change type
            FileChangeType change_type;
            switch (info->Action) {
                case FILE_ACTION_ADDED:
                    change_type = FileChangeType::Created;
                    break;
                case FILE_ACTION_REMOVED:
                    change_type = FileChangeType::Deleted;
                    break;
                case FILE_ACTION_MODIFIED:
                    change_type = FileChangeType::Modified;
                    break;
                case FILE_ACTION_RENAMED_OLD_NAME:
                case FILE_ACTION_RENAMED_NEW_NAME:
                    change_type = FileChangeType::Renamed;
                    break;
                default:
                    change_type = FileChangeType::Modified;
                    break;
            }

            // Check extension filter
            if (matches_filter(filepath)) {
                m_pending_changes[filepath_str] = {change_type, now};
            }

            // Move to next entry
            if (info->NextEntryOffset == 0) {
                break;
            }
            info = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<uint8_t*>(info) + info->NextEntryOffset
            );
        }
    }

    // Reset event and start new read
    ResetEvent(m_overlapped.hEvent);
    DWORD bytes_returned = 0;
    BOOL success = ReadDirectoryChangesW(
        m_directory_handle,
        m_buffer.data(),
        static_cast<DWORD>(m_buffer.size()),
        m_config.watch_subdirectories ? TRUE : FALSE,
        FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_CREATION,
        &bytes_returned,
        &m_overlapped,
        nullptr
    );

    if (!success && GetLastError() != ERROR_IO_PENDING) {
        SUB_ERROR("FileWatcher: Failed to restart ReadDirectoryChangesW");
        m_pending_read = false;
    }
#endif
}

auto FileWatcher::poll() -> std::vector<FileChangeEvent> {
    std::vector<FileChangeEvent> events;

    if (!m_valid) {
        return events;
    }

    // Process any pending OS events
    process_events();

    // Check debounced changes
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> ready_changes;

    for (const auto& [path, change] : m_pending_changes) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - change.timestamp);
        if (elapsed >= m_config.debounce_time) {
            ready_changes.push_back(path);
            events.push_back({std::filesystem::path(path), change.type});
        }
    }

    // Remove processed changes
    for (const auto& path : ready_changes) {
        m_pending_changes.erase(path);
    }

    return events;
}

auto FileWatcher::matches_filter(const std::filesystem::path& path) const -> bool {
    if (m_config.extensions_filter.empty()) {
        return true;  // No filter = accept all
    }

    std::string ext = path.extension().string();
    // Convert to lowercase for comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& filter_ext : m_config.extensions_filter) {
        std::string filter_lower = filter_ext;
        std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
        if (ext == filter_lower) {
            return true;
        }
    }

    return false;
}

} // namespace federation
