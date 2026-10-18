/**
 * @file    app.cpp
 * @author  Marvin Smith
 * @date    2026-05-22
 *
 * @brief   SDL simulator application implementation
 */
#include <overboard/hal/sdl/app.hpp>

// C++ Standard Libraries
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Third-Party Libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <SDL2/SDL.h>
#pragma GCC diagnostic pop
#include <lvgl.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Project Libraries
#include <overboard/core/action_code.hpp>
#include <overboard/core/input_key.hpp>
#include <overboard/core/keymap.hpp>
#include <overboard/core/settings_manager.hpp>
#include <overboard/gui/app_view.hpp>
#include <overboard/hal/app_common.hpp>
#include <overboard/hal/display_config.hpp>
#include <overboard/hal/sdl/input.hpp>
#include <overboard/hal/sdl/keymap.hpp>
#include <overboard/hal/sdl/system_info.hpp>
#include <overboard/hal/settings_store_factory.hpp>
#include <overboard/io/keyboard_config.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/resources/embedded_json.hpp>

#if OVB_HAS_MK20
#include <overboard/hal/mk20/factory.hpp>
#endif

namespace ovb::hal::sdl {

/********************************/
/*          Constructor         */
/********************************/
SDL_App::SDL_App(const core::Grid_Layout& layout)
    : m_layout(layout),
      m_layers(m_keymap) {
}

/********************************/
/*          Destructor          */
/********************************/
SDL_App::~SDL_App() {
    LOG_DEBUG("SDL_App: destructor starting");
    // Clear callbacks first to prevent dangling references during destruction
    if (m_view) {
        m_view->set_panel_change_callback(nullptr);
    }
    m_layers.on_layer_change(nullptr);
    LOG_DEBUG("SDL_App: callbacks cleared, automatic member destruction will proceed");
    // Members destroyed automatically in reverse order: m_input → m_system_info → m_view → m_key_mapping_info → m_display
    // Then SDL_Quit() is called by Display destructor
}

/************************************/
/*          Create the app          */
/************************************/
std::unique_ptr<SDL_App> SDL_App::create( const core::Grid_Layout&     layout,
                                          const std::filesystem::path& layout_path ) {
    auto app = std::unique_ptr<SDL_App>(new SDL_App(layout));

    // Load keyboard configuration with embedded fallback
    try {
        io::Keyboard_Config keyboard_config = io::load_keyboard_config(layout_path);
        app->m_keymap = io::config_to_keymap(keyboard_config);
        app->m_screens = std::move(keyboard_config.screens);
        app->m_device_config = keyboard_config.device;
        app->m_layout_path = layout_path;

        // Build input_key -> key_index mapping from keyboard.json
        int bindings_count = 0;
        int max_key_id = 0;
        for (const auto& [key_id_str, key_def] : keyboard_config.keys) {
            max_key_id = std::max(max_key_id, std::stoi(key_id_str));
        }
        app->m_key_input_map.assign(static_cast<size_t>(max_key_id + 1), core::Input_Key::NONE);
        for (const auto& [key_id_str, key_def] : keyboard_config.keys) {
            int key_id = std::stoi(key_id_str);
            core::Input_Key input_key = core::string_to_input_key(key_def.input_key);
            if (input_key != core::Input_Key::NONE) {
                app->m_sdl_keymap.bind(input_key, key_id);
                app->m_key_input_map[static_cast<size_t>(key_id)] = input_key;
                bindings_count++;
            }
        }
        LOG_TRACE("Input key mapping loaded: ", bindings_count, " keys");
    } catch (const std::exception& e) {
        std::cerr << "Failed to load keyboard config: " << e.what() << "\n";
        return nullptr;
    }

    if (!app->init()) {
        return nullptr;
    }
    return app;
}

/****************************************************/
/*          Initialize the SDL application          */
/****************************************************/
bool SDL_App::init() {
    // Set up signal handlers early (before LVGL init) to ensure Ctrl-C works
    setup_signal_handlers();

    // Initialise LVGL before SDL so the SDL driver is registered
    lv_init();

#ifndef __EMSCRIPTEN__
    // Native SDL targets need explicit SDL_Init
    // Emscripten: LVGL's lv_sdl_window_create calls SDL_Init internally
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n";
        return false;
    }
#endif

    m_initialized = true;

    try {
#if OVB_HAS_MK20
        // Connect the macropad before display creation — when it is
        // the presenter, the window only needs the LCD region.
        m_mk20_device = mk20::connect_device(m_device_config, "SDL_App");
        if (m_mk20_device) {
            m_mk20_input = std::make_unique<mk20::Mk20_Input>(m_mk20_device);
        }
#endif

        // Create SDL window (HAL). When the MK20 is the presenter there
        // is no on-screen keymap panel, so the window is LCD-height only.
        const int display_height = m_mk20_device ? hal::LCD_HEIGHT
                                                 : DISPLAY_HEIGHT;
        LOG_TRACE("Creating SDL display");
        m_display = std::make_unique<Display>("Calculator", DISPLAY_WIDTH, display_height);

        // Create settings manager
        LOG_DEBUG("Creating Settings_Manager");
        auto settings_store = Settings_Store_Factory::create();
        auto settings = std::make_shared<core::Settings_Manager>(std::move(settings_store));

        // Create system info and GUI view
        LOG_DEBUG("Creating System_Info and App_View");
        m_system_info = std::make_unique<SDL_System_Info>();
        m_view = std::make_unique<gui::App_View>(
            m_display->screen(),
            m_layout,
            m_engine,
            m_layers,
            settings,
            *m_system_info);
        LOG_TRACE("App_View created successfully");

        // Create SDL input handler for physical keyboard mapping
        LOG_TRACE("Creating SDL input handler");
        m_input = std::make_unique<SDL_Input>();
        m_input->keymap() = m_sdl_keymap;

        // Create keymap presenter — MK20 macropad if connected,
        // otherwise fall back to the on-screen key mapping panel.
        m_keymap_view = nullptr;

#if OVB_HAS_MK20
        if (m_mk20_device) {
            m_mk20_keymap = mk20::make_keymap(m_mk20_device, m_layout, m_layers,
                                              m_screens, *m_view, m_system_info.get());
            m_keymap_view = m_mk20_keymap.get();
        }
#endif

#if SHOW_KEY_MAPPING
        // Fall back to the on-screen key mapping panel
        // (KBD_HEIGHT > 0 means a keyboard region was reserved below the LCD)
        if (!m_keymap_view) {
            m_key_mapping_info = make_keymap_panel(
                m_display->screen(), m_layout, m_layers, *m_view,
                [this](int key_index) { on_key_clicked(key_index, this); });
            if (m_key_mapping_info) {
                m_keymap_view = m_key_mapping_info.get();
            }
        }
#endif

        // Wire layer/panel-change callbacks onto the presenter
        wire_keymap_view_callbacks(*m_view, m_layers,
                                   [this]() { return m_keymap_view; });

        // Trigger callback manually for the initial panel (already activated during App_View construction)
        m_view->trigger_panel_change_callback();

        LOG_DEBUG("Rendering initial view");
        m_view->render();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "SDL_App init error: " << e.what() << "\n";
        return false;
    }
}

/************************************/
/*          Run the app             */
/************************************/
#ifdef __EMSCRIPTEN__
static void sdl_app_run_frame(void* user_data) {
    auto* app = static_cast<SDL_App*>(user_data);
    app->run_frame();
}
#endif

void SDL_App::run_frame() {
    // Accurate tick tracking
    static uint32_t last_tick = SDL_GetTicks();
    uint32_t now  = SDL_GetTicks();
    uint32_t elapsed = now - last_tick;
    last_tick = now;
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
    }

    // Pump SDL events (handles keyboard and mouse hit-testing)
    m_input->pump();

    // Process keyboard events from SDL_Input
    hal::Key_Event key_event;
    while (m_input->poll(key_event)) {
        if (key_event.type == hal::Key_Event_Type::Press) {
            if (key_event.kind == hal::Key_Event_Kind::Action) {
                // Macropad: key_index -> action via layers
                handle_key(key_event.key_index);
            } else if (key_event.kind == hal::Key_Event_Kind::Direct_Action) {
                // Standard keyboard: Input_Key -> Action_Code directly
                handle_direct_action(key_event.input_key);
            } else {
                // Text input - flash the matching key on the mapping panel
                if (m_key_mapping_info && key_event.codepoint > 0 && key_event.codepoint < 128) {
                    std::string ch(1, static_cast<char>(key_event.codepoint));
                    int idx = m_key_mapping_info->find_key_by_label(ch);
                    if (idx >= 0) {
                        m_key_mapping_info->flash_key(idx);
                    }
                }
                // Panel handles refresh if consumed
                if (!m_view->handle_text(key_event.codepoint)) {
                    m_view->refresh();
                }
            }
        }
    }

#if OVB_HAS_MK20
    // MK20 serial key events carry layout indices directly (same
    // convention as the SDL Action path), so they feed handle_key.
    if (m_mk20_input) {
        while (m_mk20_input->poll(key_event)) {
            if (key_event.kind == hal::Key_Event_Kind::Action &&
                key_event.type == hal::Key_Event_Type::Press) {
                handle_key(key_event.key_index);
            }
        }
    }
    if (m_mk20_keymap) {
        m_mk20_keymap->tick();
    }
#endif

    // Track shift state for future mode system
    // Note: Hardcoded Shift layer switching removed.
    // Layer switching will be handled by the app mode system.
    [[maybe_unused]] int current_shift_state = m_input->is_shift_pressed() ? 1 : 0;

    m_view->render();
}

void SDL_App::run() {
    LOG_DEBUG("SDL_App::run() started");

#ifdef __EMSCRIPTEN__
    // Browser target: use emscripten main loop (no synchronous sleep)
    emscripten_set_main_loop_arg(sdl_app_run_frame, this, 0, 1);
#else
    // Native target: blocking loop with SDL_Delay
    int loop_count = 0;
    while (!m_should_quit && !m_input->should_quit()) {
        uint32_t frame_start = SDL_GetTicks();
        run_frame();
        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (frame_time > 30) {
            LOG_WARN("Slow frame: ", frame_time, "ms");
        }
        if (frame_time < 16) {
            SDL_Delay(16 - frame_time);
        }
        loop_count++;
    }
    LOG_DEBUG("SDL_App::run() exiting, loop_count=" + std::to_string(loop_count));
#endif
}

/************************************/
/*       Key Dispatch               */
/************************************/
void SDL_App::on_key_clicked(int key_index, void* user_data) {
    LOG_DEBUG("LVGL button clicked: key_index=" + std::to_string(key_index));
    static_cast<SDL_App*>(user_data)->handle_key(key_index);
}

/*********************************/
/*        Handle Keypress        */
/*********************************/
void SDL_App::handle_key(int key_index) {
    // Resolve the key's Input_Key for NONE-action passthrough
    std::optional<core::Input_Key> input_key;
    const auto slot = static_cast<size_t>(key_index);
    if (slot < m_key_input_map.size() && m_key_input_map[slot] != core::Input_Key::NONE) {
        input_key = m_key_input_map[slot];
    }

    dispatch_layout_key(m_layers, m_screens, *m_view, m_keymap_view,
                        key_index, input_key);
}

/****************************************/
/*   Handle Direct Action (standard kbd) */
/****************************************/
void SDL_App::handle_direct_action(core::Input_Key key) {
    LOG_DEBUG("Standard keyboard: input_key=" + core::input_key_to_string(key));
    // Pass Input_Key directly to GUI - panels decide what it means
    m_view->handle_input_key(key);
}

/*******************************/
/*          Get Display        */
/*******************************/
I_Display& SDL_App::get_display() {
    return *m_view;
}

/****************************************/
/*      Check if the app should quit    */
/****************************************/
bool SDL_App::should_quit() const {
    return m_input ? m_input->should_quit() : m_should_quit;
}

} // namespace ovb::hal::sdl
