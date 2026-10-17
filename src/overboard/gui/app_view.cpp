/**
 * @file    app_view.cpp
 * @author  Marvin Smith
 * @date    2026-05-29
 *
 * @brief   LVGL application view implementation
 */
#include <overboard/gui/app_view.hpp>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/apps/app_registration.hpp>
#include <overboard/gui/app_menu.hpp>
#include <overboard/gui/app_registry.hpp>
#include <overboard/gui/keyboard_display.hpp>
#include <overboard/gui/lvgl_theme.hpp>
#include <overboard/hal/display_config.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::gui {

// Use panel indices from app registration
using ovb::apps::PANEL_STATUS;
using ovb::apps::PANEL_CALCULATOR;
using ovb::apps::PANEL_SETTINGS;
using ovb::apps::PANEL_MENU;

/*******************************/
/*            Impl             */
/*******************************/
struct App_View::Impl {

    /// @brief Panel Manager
    std::unique_ptr<Panel_Manager>              m_panels;

    /// @brief Settings Manager
    std::shared_ptr<core::Settings_Manager>     m_settings;

    /// @brief System Info
    hal::I_System_Info*                         m_system_info;

    /// @brief Menu Index
    int                                         m_menu_idx       { -1 };

    /// @brief Previous Panel Index
    int                                         m_prev_panel_idx { PANEL_STATUS };

    /// @brief Panel Container
    lv_obj_t*              m_panel_container = nullptr;

    /// @brief Layer Manager
    core::Layer_Manager*   m_layers          = nullptr;

    /// @brief Base Layer
    int                    m_base_layer      { 0 };    ///< Layer index before shift was engaged

    /// @brief Shift Active
    bool                   m_shift_active    { false };
};

/*******************************/
/*          Destructor         */
/*******************************/
App_View::~App_View() = default;

/*******************************/
/*          Constructor        */
/*******************************/
App_View::App_View( lv_obj_t*                      root,
                    [[maybe_unused]] const ovb::core::Grid_Layout&  layout,
                    ovb::math::Calc_Engine&        engine,
                    ovb::core::Layer_Manager&      layers,
                    std::shared_ptr<ovb::core::Settings_Manager> settings,
                    ovb::hal::I_System_Info&        system_info )
    : m_impl(std::make_unique<Impl>())
{
    m_impl->m_layers      = &layers;
    m_impl->m_settings    = settings;
    m_impl->m_system_info = &system_info;
    lv_obj_set_style_pad_all(root, 0, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_set_style_bg_color(root, lvgl_color(LVGL_COLOR_BG_SCREEN), LV_PART_MAIN);

    m_impl->m_panel_container = lv_obj_create(root);
    lv_obj_set_size(m_impl->m_panel_container, hal::LCD_WIDTH, hal::LCD_HEIGHT);
    lv_obj_align(m_impl->m_panel_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(m_impl->m_panel_container, lvgl_color(LVGL_COLOR_BG_SCREEN), LV_PART_MAIN);
    lv_obj_set_style_border_width(m_impl->m_panel_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m_impl->m_panel_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(m_impl->m_panel_container, 0, LV_PART_MAIN);
    lv_obj_set_scrollable(m_impl->m_panel_container, false);

    m_impl->m_panels = std::make_unique<Panel_Manager>(m_impl->m_panel_container);

    // Create app registry and register all apps
    App_Registry registry;
    apps::register_all_apps( registry,
                             engine,
                             layers,
                             *m_impl->m_panels,
                             m_impl->m_settings,
                             *m_impl->m_system_info );

    // Get apps and menu items from registry
    auto apps = registry.create_apps();
    auto menu_items = registry.get_menu_items();
    LOG_DEBUG("App_View: Created ", std::to_string(apps.size()), " apps, ", std::to_string(menu_items.size()), " menu items");

    // App_Menu owns apps, registers them with Panel_Manager, and handles selection
    auto menu = std::make_shared<App_Menu>(std::move(apps), *m_impl->m_panels, *m_impl->m_system_info, std::move(menu_items),
        [this](int panel_index) {
            m_impl->m_panels->push(panel_index);
        }, 0);
    m_impl->m_menu_idx = m_impl->m_panels->register_panel(menu);
    LOG_DEBUG("App_View: Registered menu panel at index ", std::to_string(m_impl->m_menu_idx));

    // Determine boot target from settings (default: "status")
    const std::string default_app = m_impl->m_settings
        ? m_impl->m_settings->get<std::string>("ui.default_app", std::string("status"))
        : std::string("status");
    const int default_panel = apps::resolve_default_panel(default_app);
    LOG_DEBUG("App_View: default_app='", default_app, "' -> panel ", std::to_string(default_panel));

    if (default_panel == apps::PANEL_MENU) {
        m_impl->m_panels->push(m_impl->m_menu_idx);
        LOG_DEBUG("App_View: Booting to menu");
    } else {
        // Push menu first so ESCAPE from the app returns to menu
        m_impl->m_panels->push(m_impl->m_menu_idx);
        m_impl->m_panels->push(default_panel);
        LOG_DEBUG("App_View: Booting directly to panel ", std::to_string(default_panel));
    }

    LOG_TRACE("App_View: Constructor complete");
}

/****************************/
/*       Handle Input       */
/****************************/
void App_View::handle_input(core::Action_Code action) {
    LOG_DEBUG("App_View::handle_input: action_code=", std::to_string(static_cast<int>(action)), " (", core::action_code_to_display(action), ")");

    // Handle modifier keys globally — switch layer and let display update via callback
    if (m_impl->m_layers) {
        const int layer_count = m_impl->m_layers->layer_count();
        switch (action) {
            case core::Action_Code::LEFT_SHIFT:
            case core::Action_Code::RIGHT_SHIFT: {
                if (!m_impl->m_shift_active) {
                    // Find a layer named "Shift" (index 1 by convention)
                    m_impl->m_base_layer   = m_impl->m_layers->active_layer();
                    m_impl->m_shift_active = true;
                    const int shift_idx  = (layer_count > 1) ? 1 : 0;
                    m_impl->m_layers->set_layer(shift_idx);
                } else {
                    m_impl->m_shift_active = false;
                    m_impl->m_layers->set_layer(m_impl->m_base_layer);
                }
                return;
            }
            case core::Action_Code::CAPS_LOCK: {
                // Toggle: if already on Caps layer (index 2), return to base; else go to Caps
                const int caps_idx = (layer_count > 2) ? 2 : 0;
                if (m_impl->m_layers->active_layer() == caps_idx) {
                    m_impl->m_layers->set_layer(0);
                } else {
                    m_impl->m_layers->set_layer(caps_idx);
                }
                m_impl->m_shift_active = false;
                return;
            }
            default:
                break;
        }
        // After any non-modifier key press, release shift (momentary behaviour)
        if (m_impl->m_shift_active) {
            m_impl->m_shift_active = false;
            m_impl->m_layers->set_layer(m_impl->m_base_layer);
        }
    }

    m_impl->m_panels->handle_input(action);
}

/****************************/
/*       Handle Text        */
/****************************/
bool App_View::handle_text(char32_t codepoint) {
    LOG_DEBUG("App_View::handle_text: codepoint=", std::to_string(static_cast<uint32_t>(codepoint)));
    return m_impl->m_panels->handle_text(codepoint);
}

/******************************/
/*     Handle Input Key       */
/******************************/
void App_View::handle_input_key(core::Input_Key key) {
    LOG_DEBUG("App_View::handle_input_key: key=", core::input_key_to_string(key));
    m_impl->m_panels->handle_input_key(key);
}

/****************************/
/*          Refresh         */
/****************************/
void App_View::refresh() {
    m_impl->m_panels->refresh();
}

/******************************/
/*        Update Layer        */
/******************************/
void App_View::update_layer() {
    // Keyboard layer updates now handled by SDL_App's Key_Mapping_Info
}

/****************************/
/*          Render          */
/****************************/
void App_View::render() {
    lv_timer_handler();
}

/****************************/
/*  Get Active Panel Label  */
/****************************/
std::string App_View::get_active_panel_label(int key_index) const {
    if (auto* panel = m_impl->m_panels->active_panel()) {
        return panel->get_custom_label(key_index);
    }
    return "";
}

/****************************/
/*  Set Panel Change Callback */
/****************************/
void App_View::set_panel_change_callback(std::function<void(I_Panel*)> cb) {
    m_impl->m_panels->set_panel_change_callback(std::move(cb));
}

/*******************************/
/* Trigger Panel Change Callback */
/*******************************/
void App_View::trigger_panel_change_callback() {
    m_impl->m_panels->trigger_panel_change_callback();
}

} // namespace ovb::gui
