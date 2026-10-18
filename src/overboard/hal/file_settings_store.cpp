/**
 * @file    file_settings_store.cpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared filesystem-backed settings storage implementation
 */
#include <overboard/hal/file_settings_store.hpp>

// C++ Standard Libraries
#include <fstream>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal {

/****************************/
/*     Constructor          */
/****************************/
File_Settings_Store::File_Settings_Store(
    const std::filesystem::path& config_dir,
    const std::string& file_name,
    const std::optional<std::filesystem::path>& defaults_path
)
    : m_config_dir(config_dir),
      m_settings_file(config_dir / file_name),
      m_defaults_path(defaults_path) {
    LOG_DEBUG("File_Settings_Store: config_dir=", m_config_dir.string(),
              ", file=", m_settings_file.string());
}

/****************************/
/*     Load                 */
/****************************/
bool File_Settings_Store::load(Settings_Tree& out_tree) {
    Settings_Tree merged;

    // Seed with the defaults template when one was configured
    if (m_defaults_path && std::filesystem::exists(*m_defaults_path)) {
        try {
            merged = Settings_Tree(toml::parse_file(m_defaults_path->string()));
            LOG_DEBUG("Loaded defaults from ", m_defaults_path->string());
        } catch (const toml::parse_error& e) {
            LOG_WARN("Failed to parse defaults file: ", e.what());
        }
    }

    // Overlay user settings on top of defaults
    if (exists()) {
        try {
            toml::table user_table = toml::parse_file(m_settings_file.string());
            Settings_Tree user_tree(std::move(user_table));
            merged.merge(user_tree);
            LOG_DEBUG("Overlaid user settings from ", m_settings_file.string());
        } catch (const toml::parse_error& e) {
            LOG_ERROR("Failed to parse user settings file: ",
                      m_settings_file.string(), " - ", e.what());
            // Continue with defaults only
        }
    } else {
        LOG_DEBUG("No user settings file found, using defaults only");
    }

    out_tree = std::move(merged);
    return true;
}

/****************************/
/*     Save                 */
/****************************/
bool File_Settings_Store::save(const Settings_Tree& tree) {
    if (!ensure_config_dir()) {
        LOG_ERROR("Failed to create config directory: ", m_config_dir.string());
        return false;
    }

    std::ofstream file(m_settings_file);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open settings file for writing: ",
                  m_settings_file.string());
        return false;
    }

    // Serialize TOML
    file << tree.data();

    if (!file.good()) {
        LOG_ERROR("Failed to write settings file: ", m_settings_file.string());
        return false;
    }

    LOG_DEBUG("Saved settings to ", m_settings_file.string());
    return true;
}

/****************************/
/*     Exists               */
/****************************/
bool File_Settings_Store::exists() const {
    return std::filesystem::exists(m_settings_file);
}

/****************************/
/*     Location             */
/****************************/
std::string File_Settings_Store::location() const {
    return m_settings_file.string();
}

/****************************/
/*     Is Writable          */
/****************************/
bool File_Settings_Store::is_writable() const {
    // Walk up to the first existing ancestor and check its permissions —
    // the config dir itself may not exist yet.
    std::filesystem::path dir = m_config_dir;
    while (!std::filesystem::exists(dir)) {
        if (!dir.has_parent_path() || dir == dir.parent_path()) {
            return false;
        }
        dir = dir.parent_path();
    }

    auto perms = std::filesystem::status(dir).permissions();
    return (perms & std::filesystem::perms::owner_write) != std::filesystem::perms::none;
}

/****************************/
/*     Ensure Config Dir    */
/****************************/
bool File_Settings_Store::ensure_config_dir() {
    if (std::filesystem::exists(m_config_dir)) {
        return true;
    }

    try {
        std::filesystem::create_directories(m_config_dir);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        LOG_ERROR("Failed to create config directory: ", e.what());
        return false;
    }
}

} // namespace ovb::hal
