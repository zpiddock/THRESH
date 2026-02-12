#pragma once

#include <filesystem>
#include <string>
#include <fstream>
#include <ser20/archives/json.hpp>
#include <ser20/types/vector.hpp>

#include "log.hpp"

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
 * ConfigManager - Generic configuration loader/saver with Cereal serialization
 *
 * Usage:
 *   auto config = ConfigManager::load<MyConfig>("config.json");
 *   ConfigManager::save(config, "config.json");
 */
class SUBSTRATUM_API ConfigManager {
public:
    /**
     * Load configuration from JSON file.
     * If file doesn't exist or is invalid, returns default-constructed T.
     * Automatically creates config file with defaults if missing.
     */
    template<typename T>
    static auto load(const std::filesystem::path& filepath) -> T {
        T config{};  // Default construct

        if (!std::filesystem::exists(filepath)) {
            FED_INFO("Config file not found: {}, creating with defaults", filepath.string());
            save(config, filepath);
            return config;
        }

        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                FED_ERROR("Failed to open config file: {}", filepath.string());
                return config;
            }

            ser20::JSONInputArchive archive(file);
            archive(config);
            FED_INFO("Loaded config from: {}", filepath.string());

        } catch (const std::exception& e) {
            FED_ERROR("Failed to parse config file: {} - {}", filepath.string(), e.what());
            FED_INFO("Using default configuration");
        }

        return config;
    }

    /**
     * Save configuration to JSON file with pretty printing.
     */
    template<typename T>
    static auto save(const T& config, const std::filesystem::path& filepath) -> bool {
        try {
            // Create parent directories if they don't exist
            if (auto parent = filepath.parent_path(); !parent.empty()) {
                std::filesystem::create_directories(parent);
            }

            std::ofstream file(filepath);
            if (!file.is_open()) {
                FED_ERROR("Failed to create config file: {}", filepath.string());
                return false;
            }

            ser20::JSONOutputArchive archive(file,
                ser20::JSONOutputArchive::Options::Default());
            archive(ser20::make_nvp("config", config));

            FED_INFO("Saved config to: {}", filepath.string());
            return true;

        } catch (const std::exception& e) {
            FED_ERROR("Failed to save config file: {} - {}", filepath.string(), e.what());
            return false;
        }
    }

    /**
     * Validate config file exists and is parseable.
     */
    template<typename T>
    static auto validate(const std::filesystem::path& filepath) -> bool {
        if (!std::filesystem::exists(filepath)) {
            return false;
        }

        try {
            T config{};
            std::ifstream file(filepath);
            ser20::JSONInputArchive archive(file);
            archive(config);
            return true;
        } catch (...) {
            return false;
        }
    }

private:
};

} // namespace substratum
