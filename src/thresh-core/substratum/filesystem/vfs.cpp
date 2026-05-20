//
// Created by Admin on 11/04/2026.
//

#include "vfs.hpp"

#include <string>
#include <vector>

#include "physfs.h"
#include "substratum/log.hpp"

namespace substratum {
 bool VFS::s_initialized = false;

    auto VFS::init(const char *argv0, const std::optional<std::string>& write_dir) -> bool {
        if (s_initialized) {
            SUB_WARN("VFS::init() called but already initialized");
            return true;
        }

        if (!PHYSFS_init(argv0)) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_FATAL("PhysicsFS init failed: {}", PHYSFS_getErrorByCode(error_code));
            return false;
        }

        s_initialized = true;

        PHYSFS_Version linked{};
        PHYSFS_getLinkedVersion(&linked);
        SUB_INFO("VFS initialized (PhysicsFS {}.{}.{})",
                 static_cast<int>(linked.major),
                 static_cast<int>(linked.minor),
                 static_cast<int>(linked.patch));

        if (write_dir != std::nullopt) {

            PHYSFS_setWriteDir(write_dir->c_str());
            SUB_INFO("VFS WriteDir to '{}'", write_dir->c_str());
        }

        return true;
    }

    auto VFS::shutdown() -> void {
        if (!s_initialized) {
            return;
        }

        if (!PHYSFS_deinit()) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_ERROR("PhysicsFS deinit failed: {}", PHYSFS_getErrorByCode(error_code));
        }

        s_initialized = false;
        SUB_INFO("VFS shutdown");
    }

    auto VFS::is_initialized() -> bool {
        return s_initialized;
    }

    auto VFS::mount(const std::string &real_path,
                    const std::string &mount_point,
                    bool append) -> bool {
        if (!s_initialized) {
            SUB_ERROR("VFS::mount() called before init()");
            return false;
        }

        if (!PHYSFS_mount(real_path.c_str(), mount_point.c_str(), append ? 1 : 0)) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_ERROR("VFS mount failed for '{}' at '{}': {}",
                      real_path, mount_point, PHYSFS_getErrorByCode(error_code));
            return false;
        }

        SUB_INFO("VFS mounted '{}' at '{}'", real_path, mount_point);
        return true;
    }

    auto VFS::mount(const VFSMount& mount) -> bool {

        return VFS::mount(mount.real_path, mount.mount_point, mount.append);
    }

    auto VFS::unmount(const std::string &real_path) -> bool {
        if (!s_initialized) {
            SUB_ERROR("VFS::unmount() called before init()");
            return false;
        }

        if (!PHYSFS_unmount(real_path.c_str())) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_ERROR("VFS unmount failed for '{}': {}",
                      real_path, PHYSFS_getErrorByCode(error_code));
            return false;
        }

        SUB_INFO("VFS unmounted '{}'", real_path);
        return true;
    }

    auto VFS::read_file(const std::string &virtual_path) -> std::vector<std::uint8_t> {
        if (!s_initialized) {
            SUB_ERROR("VFS::read_file() called before init()");
            return {};
        }

        auto *file = PHYSFS_openRead(virtual_path.c_str());
        if (!file) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_ERROR("VFS failed to open '{}': {}", virtual_path, PHYSFS_getErrorByCode(error_code));
            return {};
        }

        auto length = PHYSFS_fileLength(file);
        if (length < 0) {
            SUB_ERROR("VFS failed to get length of '{}'", virtual_path);
            PHYSFS_close(file);
            return {};
        }

        auto data = std::vector<std::uint8_t>(static_cast<std::size_t>(length));
        auto bytes_read = PHYSFS_readBytes(file, data.data(), static_cast<PHYSFS_uint64>(length));
        PHYSFS_close(file);

        if (bytes_read != length) {
            SUB_ERROR("VFS partial read of '{}': read {} of {} bytes",
                      virtual_path, bytes_read, length);
            return {};
        }

        return data;
    }

    auto VFS::read_file_string(const std::string &virtual_path) -> std::string {
        auto data = read_file(virtual_path);
        if (data.empty()) {
            return {};
        }
        return {reinterpret_cast<const char *>(data.data()), data.size()};
    }

    auto VFS::exists(const std::string &virtual_path) -> bool {
        if (!s_initialized) {
            return false;
        }
        return PHYSFS_exists(virtual_path.c_str()) != 0;
    }

    auto VFS::enumerate(const std::string &virtual_dir) -> std::vector<std::string> {
        if (!s_initialized) {
            SUB_ERROR("VFS::enumerate() called before init()");
            return {};
        }

        auto result = std::vector<std::string>{};
        auto **files = PHYSFS_enumerateFiles(virtual_dir.c_str());
        if (!files) {
            auto error_code = PHYSFS_getLastErrorCode();
            SUB_ERROR("VFS enumerate failed for '{}': {}",
                      virtual_dir, PHYSFS_getErrorByCode(error_code));
            return {};
        }

        for (auto **i = files; *i != nullptr; ++i) {
            result.emplace_back(*i);
        }

        PHYSFS_freeList(files);
        return result;
    }

    auto VFS::get_real_dir(const std::string &virtual_path) -> std::string {
        if (!s_initialized) {
            SUB_ERROR("VFS::get_real_dir() called before init()");
            return {};
        }

        auto *dir = PHYSFS_getRealDir(virtual_path.c_str());
        if (!dir) {
            return {};
        }
        return {dir};
    }

    auto VFS::write_file_string(const std::string &virtual_path,
                                const std::string &contents) -> bool {

        if (!s_initialized) {
            SUB_ERROR("VFS::write_file_string() called before init()");
            return false;
        }

        auto* handle = PHYSFS_openWrite(virtual_path.c_str());
        if (!handle) {
            SUB_ERROR("VFS open failed for '{}': {}", virtual_path, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
            return false;
        }

        const auto written = PHYSFS_writeBytes(handle, contents.data(), contents.size());

        PHYSFS_close(handle);
        if (written != contents.size()) {

            SUB_ERROR("VFS::write_file_string() failed for '{}': {} / {}", virtual_path, written, contents.size());
            return false;
        }
        return true;
    }
} // namespace substratum