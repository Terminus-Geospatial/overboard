/**
 * @file    file_settings_store.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared filesystem-backed settings storage
 *
 * Single implementation used by the SDL, WASM, and Pi Zero targets.
 * The caller supplies the config directory (platform policy lives in
 * Settings_Store_Factory) and an optional defaults file whose contents
 * are loaded first and overlaid by the user file.
 */
#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <optional>
#include <string>

// Project Libraries
#include <overboard/hal/i_settings_store.hpp>

namespace ovb::hal {

/**
 * @brief File-based TOML settings store
 *
 * Writes a single TOML file under a caller-provided config directory.
 * When a defaults path is supplied, its contents seed the tree on load
 * and the user file is merged over it.
 */
class File_Settings_Store : public I_Settings_Store {

    public:

        /**
         * @brief Constructor
         * @param config_dir Directory holding the settings file
         * @param file_name Settings filename (default: "settings.toml")
         * @param defaults_path Optional TOML file merged beneath user settings
         */
        explicit File_Settings_Store(
            const std::filesystem::path& config_dir,
            const std::string& file_name = "settings.toml",
            const std::optional<std::filesystem::path>& defaults_path = std::nullopt
        );

        /**
         * @brief Load settings (defaults first, user file overlaid)
         */
        bool load(Settings_Tree& out_tree) override;

        /**
         * @brief Save settings to file
         */
        bool save(const Settings_Tree& tree) override;

        /**
         * @brief Check if settings file exists
         */
        bool exists() const override;

        /**
         * @brief Get full path to settings file
         */
        std::string location() const override;

        /**
         * @brief Check if storage directory is writable
         */
        bool is_writable() const override;

        /**
         * @brief Get the config directory path
         */
        std::filesystem::path config_dir() const { return m_config_dir; }

    private:
        /**
         * @brief Ensure config directory exists
         */
        bool ensure_config_dir();

        /// @brief Configuration directory path
        std::filesystem::path m_config_dir;

        /// @brief Settings file path
        std::filesystem::path m_settings_file;

        /// @brief Optional defaults file merged beneath user settings
        std::optional<std::filesystem::path> m_defaults_path;
};

} // namespace ovb::hal
