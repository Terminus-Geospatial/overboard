/**
 * @file    settings_store.cpp
 * @author  Marvin Smith
 * @date    2026-06-13
 *
 * @brief   PicoCalc settings storage implementation (stub)
 */
#include "settings_store.hpp"

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::picocalc {

/****************************/
/*     Constructor          */
/****************************/
PicoCalc_Settings_Store::PicoCalc_Settings_Store(const std::filesystem::path& path)
    : m_path(path) {
    LOG_DEBUG("PicoCalc_Settings_Store created (uninitialized), path=", path.string());
}

/****************************/
/*     Initialize           */
/****************************/
bool PicoCalc_Settings_Store::init() {
    // TODO: Initialize SD card and mount FAT filesystem
    // 1. Initialize SD card SPI
    // 2. Mount FAT filesystem
    // 3. Check if writable

    LOG_DEBUG("PicoCalc_Settings_Store::init() - stub, SD/FAT not yet implemented");
    m_initialized = false;
    m_sd_mounted  = false;
    return false;
}

/****************************/
/*     Load                 */
/****************************/
bool PicoCalc_Settings_Store::load(Settings_Tree& out_tree) {
    if (!m_initialized) {
        LOG_WARN("Load called before init()");
        out_tree = Settings_Tree();
        return true;  // Return empty tree, not an error
    }

    if (!m_sd_mounted) {
        LOG_ERROR("SD card not mounted");
        out_tree = Settings_Tree();
        return true;
    }

    // TODO: Read settings.toml from SD card using f_open/f_read
    LOG_DEBUG("PicoCalc load() - stub");
    out_tree = Settings_Tree();
    return true;
}

/****************************/
/*     Save                 */
/****************************/
bool PicoCalc_Settings_Store::save(const Settings_Tree& tree) {

    if (!m_initialized) {
        LOG_ERROR("Save called before init()");
        return false;
    }

    if (!m_sd_mounted) {
        LOG_ERROR("SD card not mounted");
        return false;
    }

    // TODO: Write settings.toml to SD card using f_open/f_write
    LOG_DEBUG("PicoCalc save() - stub");
    return false;
}

/****************************/
/*     Exists               */
/****************************/
bool PicoCalc_Settings_Store::exists() const {
    if (!m_initialized || !m_sd_mounted) {
        return false;
    }

    // TODO: Check if settings.toml exists using f_stat
    return false;
}

/****************************/
/*     Location             */
/****************************/
std::string PicoCalc_Settings_Store::location() const {
    return m_path.string() + " (SD card FAT32 - not implemented)";
}

/****************************/
/*     Is Writable          */
/****************************/
bool PicoCalc_Settings_Store::is_writable() const {
    return m_initialized && m_sd_mounted;
}

} // namespace ovb::hal::picocalc
