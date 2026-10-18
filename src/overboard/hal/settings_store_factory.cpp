/**
 * @file    settings_store_factory.cpp
 * @author  Marvin Smith
 * @date    2026-06-13
 *
 * @brief   Settings store factory implementation
 */
#include "settings_store_factory.hpp"

// C++ Standard Libraries
#include <cstdlib>
#include <filesystem>

// Platform-specific implementations
#ifdef TARGET_RP2350
    #include <overboard/hal/picocalc/settings_store.hpp>
#else
    #include <overboard/hal/file_settings_store.hpp>
#endif

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal {

#ifndef TARGET_RP2350

#if defined(__linux__)

/**
 * @brief XDG/Linux config directory for SDL and Pi Zero targets
 *
 * $XDG_CONFIG_HOME/app_name, falling back to ~/.config/app_name, then
 * /home/pi (embedded fallback), then /tmp.
 */
static std::filesystem::path host_config_dir(const std::string& app_name) {
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config) {
        return std::filesystem::path(xdg_config) / app_name;
    }

    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / ".config" / app_name;
    }

    if (std::filesystem::exists("/home/pi")) {
        return std::filesystem::path("/home/pi/.config") / app_name;
    }

    return std::filesystem::path("/tmp") / (app_name + "_config");
}

#elif defined(__APPLE__)

/**
 * @brief macOS config directory for SDL target
 *
 * $HOME/Library/Application Support/app_name, then /tmp.
 */
static std::filesystem::path host_config_dir(const std::string& app_name) {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) /
               "Library" / "Application Support" / app_name;
    }
    return std::filesystem::path("/tmp") / (app_name + "_config");
}

#elif defined(_WIN32)

/**
 * @brief Windows config directory for SDL target
 *
 * %APPDATA%\app_name, then C:\temp.
 */
static std::filesystem::path host_config_dir(const std::string& app_name) {
    const char* appdata = std::getenv("APPDATA");
    if (appdata) {
        return std::filesystem::path(appdata) / app_name;
    }
    return std::filesystem::path("C:\\temp") / (app_name + "_config");
}

#else

static std::filesystem::path host_config_dir(const std::string& app_name) {
    return std::filesystem::path("/tmp") / (app_name + "_config");
}

#endif

#endif // !TARGET_RP2350

/*********************************/
/*      Create Store Instance    */
/*********************************/
std::unique_ptr<I_Settings_Store> Settings_Store_Factory::create() {
#if defined(TARGET_RP2350)
    LOG_INFO("Creating PicoCalc settings store (SD card - stub)");
    auto store = std::make_unique<picocalc::PicoCalc_Settings_Store>();
    store->init();  // Initialize SD card
    return store;
#elif defined(TARGET_ZERO)
    LOG_INFO("Creating Pi Zero settings store (filesystem)");
    return std::make_unique<File_Settings_Store>(host_config_dir("kbd_calc"));
#elif defined(TARGET_SDL)
    LOG_INFO("Creating SDL settings store (filesystem)");
    return std::make_unique<File_Settings_Store>(
        host_config_dir("kbd_calc"),
        "settings.toml",
        std::filesystem::path("data/configs/settings.toml"));
#else
    #error "No target defined. Define TARGET_SDL, TARGET_RP2350, or TARGET_ZERO."
#endif
}

/*********************************/
/*      Platform Name            */
/*********************************/
std::string_view Settings_Store_Factory::platform_name() {
#ifdef TARGET_SDL
    #ifdef TARGET_PICOSDL
        return "PicoSDL Simulator";
    #else
        return "SDL Simulator";
    #endif
#elif defined(TARGET_RP2350)
    return "PicoCalc (RP2350)";
#elif defined(TARGET_ZERO)
    return "Pi Zero";
#else
    return "Unknown";
#endif
}

/*********************************/
/*      Is Simulator             */
/*********************************/
bool Settings_Store_Factory::is_simulator() {
#ifdef TARGET_SDL
    return true;
#else
    return false;
#endif
}

/*********************************/
/*      Is Embedded              */
/*********************************/
bool Settings_Store_Factory::is_embedded() {
#ifdef TARGET_RP2350
    return true;
#else
    return false;
#endif
}

} // namespace ovb::hal
