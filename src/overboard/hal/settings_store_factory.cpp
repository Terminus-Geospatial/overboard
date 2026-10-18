/**
 * @file    settings_store_factory.cpp
 * @author  Marvin Smith
 * @date    2026-06-13
 *
 * @brief   Settings store factory implementation
 */
#include "settings_store_factory.hpp"

// Platform-specific implementations
#ifdef TARGET_SDL
    #include <overboard/hal/sdl/settings_store.hpp>
#elif defined(TARGET_RP2350)
    #include <overboard/hal/picocalc/settings_store.hpp>
#elif defined(TARGET_ZERO)
    #include <overboard/hal/pi_zero/settings_store.hpp>
#else
    #error "No target defined. Define TARGET_SDL, TARGET_RP2350, or TARGET_ZERO."
#endif

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal {

/*********************************/
/*      Create Store Instance    */
/*********************************/
std::unique_ptr<I_Settings_Store> Settings_Store_Factory::create() {
#ifdef TARGET_SDL
    LOG_INFO("Creating SDL settings store (filesystem)");
    return std::make_unique<sdl::SDL_Settings_Store>();
#elif defined(TARGET_RP2350)
    LOG_INFO("Creating PicoCalc settings store (SD card - stub)");
    auto store = std::make_unique<picocalc::PicoCalc_Settings_Store>();
    store->init();  // Initialize SD card
    return store;
#elif defined(TARGET_ZERO)
    LOG_INFO("Creating Pi Zero settings store (filesystem)");
    return std::make_unique<pi_zero::PiZero_Settings_Store>();
#else
    #error "No target defined"
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
