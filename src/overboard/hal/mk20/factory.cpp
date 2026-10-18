/**
 * @file    factory.cpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   MK20 macropad device/presenter construction helpers
 */
#include <overboard/hal/mk20/factory.hpp>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>
#include <overboard/mpad/serial_port.hpp>

namespace ovb::hal::mk20 {

/****************************/
/*   Connect MK20 device    */
/****************************/
std::shared_ptr<mpad::Device> connect_device(
    const std::optional<io::Keyboard_Config::Device_Config>& device_config,
    const std::string& log_tag ) {
    if (!device_config || device_config->type != "mk20") {
        return nullptr;
    }

    const auto port = mpad::discover_serial_port(device_config->port);
    if (port.empty()) {
        LOG_WARN(log_tag, ": MK20 configured but no serial device found");
        return nullptr;
    }

    auto device = std::make_shared<mpad::Device>(port.string());
    if (!device->is_open()) {
        LOG_WARN(log_tag, ": MK20 configured but failed to open ", port.string());
        return nullptr;
    }

    LOG_INFO(log_tag, ": MK20 macropad connected on ", port.string());
    return device;
}

/****************************/
/*    Make MK20 keymap      */
/****************************/
std::unique_ptr<Mk20_Keymap> make_keymap(
    std::shared_ptr<mpad::Device> device,
    const core::Grid_Layout&      layout,
    const core::Layer_Manager&    layers,
    const core::Screen_Set&       screens,
    gui::App_View&                view,
    I_System_Info*                system_info ) {
    return std::make_unique<Mk20_Keymap>(
        std::move(device), layout, layers, screens,
        [&view](int key_index) { return view.get_active_panel_label(key_index); },
        [&view]() { return view.active_panel_name(); },
        system_info);
}

} // namespace ovb::hal::mk20
