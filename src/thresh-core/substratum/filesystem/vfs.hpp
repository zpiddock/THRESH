//
// Created by Admin on 11/04/2026.
//

#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace substratum {

    struct VFSMount {
        std::string real_path;
        std::string mount_point = "/";
        bool append = true;
    };

/**
    * Virtual Filesystem abstraction over PhysicsFS.
    *
    * Provides transparent access to files regardless of whether they
    * reside in a real directory (development) or an archive (release).
    *
    * All methods are static — the VFS is process-global (PhysicsFS
    * manages a single search path internally).
    *
    * Usage:
    *   VFS::init(nullptr);
    *   VFS::mount("./assets", "/", true);       // dev: mount folder
    *   VFS::mount("data.pak", "/", true);        // release: mount archive
    *   auto bytes = VFS::read_file("textures/stone.ktx2");
    *   VFS::shutdown();
    */
    class VFS {
        public:
            VFS() = delete;

            /**
             * Initialize PhysicsFS.
             * @param argv0 The argv[0] from main(), or nullptr. Used by PhysicsFS
             *              to locate the application directory on some platforms.
             * @param write_dir Optional write directory for PhysicsFS to write to.
             * @return true on success
             */
            static auto init(const char* argv0, const std::optional<std::string>& write_dir) -> bool;

            /**
             * Shut down PhysicsFS. Closes all open files, blanks the search path.
             */
            static auto shutdown() -> void;

            /**
             * @return true if init() has been called successfully and shutdown() hasn't.
             */
            [[nodiscard]] static auto is_initialized() -> bool;

            /**
             * Mount a real directory or archive into the virtual filesystem.
             * @param real_path  Filesystem path to a directory or archive (.zip, .7z, etc.)
             * @param mount_point  Virtual directory to mount at (e.g. "/" or "/shaders")
             * @param append  If true, append to the search path; if false, prepend
             * @return true on success
             */
            static auto mount(const std::string& real_path,
                              const std::string& mount_point,
                              bool               append = true) -> bool;

            /**
             * Mount a real directory or archive into the virtual filesystem.
             * @param mount  Struct Version containing real_path, mount_point & append
             * @return true on success
             */
            static auto mount(const VFSMount& mount) -> bool;

            /**
             * Unmount a previously mounted directory or archive.
             * @param real_path  The same path passed to mount()
             * @return true on success
             */
            static auto unmount(const std::string& real_path) -> bool;

            /**
             * Read an entire file into a byte vector.
             * @param virtual_path  Virtual path relative to the search path (e.g. "textures/stone.png")
             * @return File contents, or an empty vector on failure
             */
            [[nodiscard]] static auto read_file(const std::string& virtual_path) -> std::vector<uint8_t>;

            /**
             * Read an entire file as a UTF-8 string.
             * Convenience for text files (shaders, configs, etc.).
             * @param virtual_path  Virtual path
             * @return File contents as string, or empty string on failure
             */
            [[nodiscard]] static auto read_file_string(const std::string& virtual_path) -> std::string;

            /**
             * Check whether a file exists in the virtual filesystem.
             */
            [[nodiscard]] static auto exists(const std::string& virtual_path) -> bool;

            /**
             * List the contents of a virtual directory.
             * @param virtual_dir  Virtual directory path (e.g. "textures" or "/")
             * @return List of filenames (not full paths) in that directory
             */
            [[nodiscard]] static auto enumerate(const std::string& virtual_dir) -> std::vector<std::string>;

            /**
             * Get the real directory that a virtual file resides in.
             * Useful for integrating with FileWatcher (which needs real paths).
             * @param virtual_path  Virtual path to query
             * @return Real directory path, or empty string on failure
             */
            [[nodiscard]] static auto get_real_dir(const std::string& virtual_path) -> std::string;

            static auto write_file_string(const std::string& virtual_path, const std::string& contents) -> bool;

        private:
            static bool s_initialized;
    };
} // substratum
