/**
 * @file    factory.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   MK20 macropad device/presenter construction helpers
 *
 * Only compiled on targets with OVB_HAS_MK20 (SDL dev and Pi Zero).
 */
#pragma once

// C++ Standard Libraries
#include <memory>
#include <optional>
#include <string>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/core/screen.hpp>
#include <overboard/gui/app_view.hpp>
#include <overboard/hal/i_system_info.hpp>
#include <overboard/hal/mk20/keymap.hpp>
#include <overboard/io/keyboard_config.hpp>
#include <overboard/mpad/device.hpp>

namespace ovb::hal::mk20 {

/**
 * @brief Connect to a configured MK20 macropad
 *
 * Resolves the configured port (or auto-discovers), opens the device,
 * and logs the outcome. The returned device is shared because
 * Mk20_Input and Mk20_Keymap both hold it.
 *
 * @param device_config Optional "device" block from keyboard.json
 * @param log_tag       Prefix for log lines (e.g. "SDL_App")
 * @return Open device, or nullptr when unconfigured/unavailable
 */
std::shared_ptr<mpad::Device> connect_device(
    const std::optional<io::Keyboard_Config::Device_Config>& device_config,
    const std::string& log_tag );

/**
 * @brief Create the MK20 keymap presenter
 *
 * Label and panel-name callbacks are bound to the App_View.
 *
 * @param device      Open MK20 device from connect_device
 * @param layout      Keyboard layout
 * @param layers      Layer manager
 * @param screens     Per-panel screen tables
 * @param view        App view (label + panel-name callbacks)
 * @param system_info Optional host info for strip telemetry
 */
std::unique_ptr<Mk20_Keymap> make_keymap(
    std::shared_ptr<mpad::Device> device,
    const core::Grid_Layout&      layout,
    const core::Layer_Manager&    layers,
    const core::Screen_Set&       screens,
    gui::App_View&                view,
    I_System_Info*                system_info );

} // namespace ovb::hal::mk20
