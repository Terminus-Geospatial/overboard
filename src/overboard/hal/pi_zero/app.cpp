/**
 * @file    app.cpp
 * @author  Marvin Smith
 * @date    2026-06-05
 *
 * @brief   Pi Zero DRM application implementation
 */
#include <overboard/hal/pi_zero/app.hpp>

// C++ Standard Libraries
#include <array>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/core/action_code.hpp>
#include <overboard/core/input_key.hpp>
#include <overboard/core/keymap.hpp>
#include <overboard/core/settings_manager.hpp>
#include <overboard/gui/app_view.hpp>
#include <overboard/hal/display_config.hpp>
#include <overboard/hal/pi_zero/system_info.hpp>
#include <overboard/hal/settings_store_factory.hpp>
#include <overboard/io/keyboard_config.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/resources/embedded_json.hpp>

namespace ovb::hal::pi_zero {

/********************************/
/*          Constructor         */
/********************************/
PiZero_App::PiZero_App(const core::Grid_Layout& layout)
    : m_layout(layout),
      m_layers(m_keymap) {
}

/********************************/
/*          Destructor          */
/********************************/
PiZero_App::~PiZero_App() = default;

/************************************/
/*          Create the app          */
/************************************/
std::unique_ptr<PiZero_App> PiZero_App::create(const core::Grid_Layout& layout,
                                               const std::filesystem::path& layout_path) {
    auto app = std::unique_ptr<PiZero_App>(new PiZero_App(layout));
    app->m_layout_path = layout_path;

    // Load keyboard configuration with embedded fallback
    io::Keyboard_Config keyboard_config;
    try {
        keyboard_config = io::load_keyboard_config(layout_path);
        app->m_keymap = io::config_to_keymap(keyboard_config);
    } catch (const std::exception& e) {
        std::cerr << "Failed to load keyboard config: " << e.what() << "\n";
        return nullptr;
    }
    app->m_device_config = keyboard_config.device;

    // Build Input_Key -> layout key index map from keyboard_config.keys
    app->m_input_key_map.fill(-1);
    for (const auto& [id_str, key_def] : keyboard_config.keys) {
        if (key_def.input_key.empty()) continue;
        auto input_key = core::string_to_input_key(key_def.input_key);
        if (input_key == core::Input_Key::NONE) continue;
        int key_idx = std::stoi(id_str);
        auto slot = static_cast<std::size_t>(input_key);
        if (slot < app->m_input_key_map.size()) {
            app->m_input_key_map[slot] = key_idx;
        }
    }

    if (!app->init()) {
        return nullptr;
    }
    return app;
}

/****************************/
/*   Resolve MK20 serial    */
/****************************/
static std::string resolve_mk20_port(const std::string& configured) {
    if (!configured.empty() && configured != "auto") {
        return configured;
    }
    // Auto-discover: Linux exposes the device as ttyACM*/ttyUSB*
    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        const auto name = entry.path().filename().string();
        if (name.starts_with("ttyACM") || name.starts_with("ttyUSB")) {
            return entry.path().string();
        }
    }
#if defined(__APPLE__)
    for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
        if (entry.path().filename().string().starts_with("cu.usbmodem")) {
            return entry.path().string();
        }
    }
#endif
    return {};
}

/****************************/
/*          Init            */
/****************************/
bool PiZero_App::init() {
    LOG_DEBUG("PiZero_App: Initializing...");

    // Initialize LVGL
    lv_init();

    // Create DRM display
    try {
        m_display = std::make_unique<Display_DRM>(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    } catch (const std::exception& e) {
        LOG_ERROR("PiZero_App: Failed to create display: ", e.what());
        return false;
    }

    // Create settings manager
    auto settings_store = Settings_Store_Factory::create();
    auto settings = std::make_shared<core::Settings_Manager>(std::move(settings_store));

    // Create system info and main application view
    m_system_info = std::make_unique<PiZero_System_Info>();
    m_system_info->init();
    m_view = std::make_unique<gui::App_View>(
        m_display->screen(),
        m_layout,
        m_engine,
        m_layers,
        settings,
        *m_system_info
    );

    // Create input handler (try common keyboard devices)
    std::vector<std::string> input_devices = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        "/dev/input/by-path/platform-3f804000.i2c-event",  // Pi Zero I2C keyboard
    };
    m_input = std::make_unique<Linux_Input>(input_devices);

    // Create keymap presenter — MK20 macropad if the config declares one,
    // otherwise fall back to the on-screen key mapping panel.
    m_keymap_view = nullptr;

#if OVB_HAS_MK20
    if (m_device_config && m_device_config->type == "mk20") {
        const auto port = resolve_mk20_port(m_device_config->port);
        if (!port.empty()) {
            auto device = std::make_shared<mpad::Device>(port);
            if (device->is_open()) {
                m_mk20_input = std::make_unique<mk20::Mk20_Input>(device);
                auto mk20_keymap = std::make_unique<mk20::Mk20_Keymap>(
                    device, m_layout, m_layers,
                    [this](int key_index) { return m_view->get_active_panel_label(key_index); });
                m_keymap_view = mk20_keymap.get();
                m_mk20_keymap = std::move(mk20_keymap);
                m_mk20_device = std::move(device);
                LOG_INFO("PiZero_App: MK20 macropad connected on ", port);
            } else {
                LOG_WARN("PiZero_App: MK20 configured but failed to open ", port);
            }
        } else {
            LOG_WARN("PiZero_App: MK20 configured but no serial device found");
        }
    }
#endif

#if SHOW_KEY_MAPPING
    if (!m_keymap_view) {
        if constexpr (hal::KBD_HEIGHT > 0) {
            LOG_DEBUG("PiZero_App: Creating Key_Mapping_Info panel");
            m_key_mapping_info = std::make_unique<gui::Key_Mapping_Info>(
                m_display->screen(), m_layout, m_layers,
                hal::FULL_WIDTH, hal::KBD_HEIGHT,
                [this](int key_index) { return m_view->get_active_panel_label(key_index); });
            lv_obj_set_pos(m_key_mapping_info->container(), 0, hal::LCD_HEIGHT);

            // Make it interactive — wire button clicks to the same handler as physical keys
            m_key_mapping_info->set_click_callback([this](int key_index) {
                on_key_clicked(key_index, this);
            });
            m_keymap_view = m_key_mapping_info.get();
            LOG_DEBUG("PiZero_App: Key_Mapping_Info panel created");
        }
    }
#endif

    // Subscribe to layer changes
    m_layers.on_layer_change([this]([[maybe_unused]] int layer_index) {
        if (m_keymap_view) {
            m_keymap_view->update_layer();
        }
    });

    // Subscribe to panel changes to update labels and wire overlay callbacks
    m_view->set_panel_change_callback([this]([[maybe_unused]] gui::I_Panel* panel) {
        if (m_keymap_view) {
            m_keymap_view->update_layer();
        }
        if (panel && m_keymap_view) {
            panel->set_overlay_callbacks(
                [this](const std::string& title, const std::vector<gui::I_Panel::Overlay_Key_Desc>& keys) {
                    m_keymap_view->push_overlay(title, keys);
                },
                [this]() {
                    m_keymap_view->pop_overlay();
                });
        }
    });

    // Trigger callback manually for the initial panel (already activated during App_View construction)
    m_view->trigger_panel_change_callback();

    // Set initial layer
    m_layers.set_layer(0);

    m_initialized = true;
    LOG_DEBUG("PiZero_App: Initialization complete");
    return true;
}

/****************************/
/*          Run             */
/****************************/
void PiZero_App::run() {
    if (!m_initialized) {
        LOG_ERROR("PiZero_App: Not initialized, cannot run");
        return;
    }

    LOG_DEBUG("PiZero_App: Starting main loop");

    constexpr auto FRAME_TIME = std::chrono::milliseconds(16);  // ~60 FPS

    while (!m_should_quit) {
        auto frame_start = std::chrono::steady_clock::now();

        // Pump input events
        m_input->pump();

        // Process input events
        Key_Event event;
        while (m_input->poll(event)) {
            if (event.kind == Key_Event_Kind::Action) {
                if (event.type == Key_Event_Type::Press) {
                    handle_key(event.key_index);
                }
            }
        }

#if OVB_HAS_MK20
        // Process MK20 serial key events — these carry resolved layout
        // indices, so they bypass the Input_Key translation map.
        if (m_mk20_input) {
            while (m_mk20_input->poll(event)) {
                if (event.kind == Key_Event_Kind::Action &&
                    event.type == Key_Event_Type::Press) {
                    handle_layout_key(event.key_index, -1);
                }
            }
        }
#endif

        // Update LVGL
        m_display->refresh();

        // Calculate sleep time to maintain frame rate
        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);
        if (elapsed < FRAME_TIME) {
            std::this_thread::sleep_for(FRAME_TIME - elapsed);
        }
    }

    LOG_DEBUG("PiZero_App: Main loop exiting");
}

/****************************/
/*      Should quit         */
/****************************/
bool PiZero_App::should_quit() const {
    return m_should_quit;
}

/****************************/
/*       Get display        */
/****************************/
I_Display& PiZero_App::get_display() {
    // TODO: Implement I_Display wrapper for Display_DRM
    throw std::runtime_error("get_display not fully implemented for Pi Zero");
}

/****************************/
/*       Get input          */
/****************************/
I_Input& PiZero_App::get_input() {
    return *m_input;
}

/****************************/
/*      Handle key          */
/****************************/
void PiZero_App::handle_key(int key_index) {
    // key_index here is actually a raw Input_Key enum value from Linux_Input::poll.
    // Translate it to the layout key index via m_input_key_map.
    auto slot = static_cast<std::size_t>(key_index);
    int layout_idx = (slot < m_input_key_map.size()) ? m_input_key_map[slot] : -1;
    if (layout_idx < 0) {
        return;  // Unmapped physical key
    }
    handle_layout_key(layout_idx, key_index);
}

/****************************/
/*    Handle layout key     */
/****************************/
void PiZero_App::handle_layout_key(int layout_idx, int raw_input_key) {
    // Flash the key on the active presenter for visual feedback
    if (m_keymap_view) {
        m_keymap_view->flash_key(layout_idx);
    }

    // Get the action from the current layer
    core::Action_Code action = m_layers.action_at(layout_idx);

    // Handle layer switching actions
    if (action == core::Action_Code::GO_HOME_LAYER) {
        m_layers.set_layer(0);
        m_view->refresh();
        return;
    }
    if (action == core::Action_Code::NEXT_LAYER) {
        m_layers.next_layer();
        m_view->refresh();
        return;
    }
    if (action == core::Action_Code::PREV_LAYER) {
        m_layers.prev_layer();
        m_view->refresh();
        return;
    }

    // If action is NONE, pass through as raw Input_Key (for F-keys, etc.)
    if (action == core::Action_Code::NONE) {
        if (raw_input_key >= 0) {
            m_view->handle_input_key(static_cast<core::Input_Key>(raw_input_key));
        }
        return;
    }

    // Pass action to the view for handling
    m_view->handle_input(action);
}

/****************************/
/*    On key clicked        */
/****************************/
void PiZero_App::on_key_clicked(int key_index, void* user_data) {
    auto* app = static_cast<PiZero_App*>(user_data);
    app->handle_layout_key(key_index, -1);
}

} // namespace ovb::hal::pi_zero
