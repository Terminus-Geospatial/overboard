/**
 * @file target.hpp
 * @brief Build-time target selection
 *
 * Includes the appropriate target configuration based on build flags.
 */

#pragma once

// Project Libraries
#ifdef TARGET_SDL
    #include <overboard/hal/config/target_sdl.hpp>
#elif defined(TARGET_RP2350)
    #include <overboard/hal/config/target_picocalc.hpp>
#elif defined(TARGET_ZERO)
    #include <overboard/hal/config/target_pi_zero.hpp>
#else
    #error "No target defined. Define TARGET_SDL, TARGET_RP2350, or TARGET_ZERO."
#endif

namespace ovb::hal::config {

/// @brief Target-specific layout factory function
using Layout_Factory = core::Grid_Layout(*)();

} // namespace ovb::hal::config
