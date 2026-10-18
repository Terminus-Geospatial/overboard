/**
 * @file    app_common.cpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared init/dispatch helpers for host apps (SDL, Pi Zero)
 */
#include <overboard/hal/app_common.hpp>

// Project Libraries
#include <overboard/core/action_code.hpp>
#include <overboard/hal/display_config.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal {

/****************************/
/*    Make keymap panel     */
/****************************/
std::unique_ptr<gui::Key_Mapping_Info> make_keymap_panel(
    lv_obj_t*                  parent,
    const core::Grid_Layout&   layout,
    const core::Layer_Manager& layers,
    gui::App_View&             view,
    std::function<void(int)>   click_cb ) {
    if constexpr (hal::KBD_HEIGHT > 0) {
        auto panel = std::make_unique<gui::Key_Mapping_Info>(
            parent, layout, layers,
            hal::FULL_WIDTH, hal::KBD_HEIGHT,
            [&view](int key_index) { return view.get_active_panel_label(key_index); });
        lv_obj_set_pos(panel->container(), 0, hal::LCD_HEIGHT);
        panel->set_click_callback(std::move(click_cb));
        return panel;
    }
    return nullptr;
}

/****************************/
/*  Wire keymap callbacks   */
/****************************/
void wire_keymap_view_callbacks(
    gui::App_View&                              view,
    core::Layer_Manager&                        layers,
    const std::function<gui::I_Keymap_View*()>& get_keymap_view ) {
    layers.on_layer_change([get_keymap_view]([[maybe_unused]] int layer_index) {
        if (auto* keymap_view = get_keymap_view()) {
            keymap_view->update_layer();
        }
    });

    view.set_panel_change_callback([get_keymap_view](gui::I_Panel* panel) {
        auto* keymap_view = get_keymap_view();
        if (keymap_view) {
            keymap_view->update_layer();
        }
        if (panel && keymap_view) {
            panel->set_overlay_callbacks(
                [get_keymap_view](const std::string& title,
                                  const std::vector<gui::I_Panel::Overlay_Key_Desc>& keys) {
                    if (auto* kv = get_keymap_view()) {
                        kv->push_overlay(title, keys);
                    }
                },
                [get_keymap_view]() {
                    if (auto* kv = get_keymap_view()) {
                        kv->pop_overlay();
                    }
                });
        }
    });
}

/****************************/
/*   Dispatch screen key    */
/****************************/
bool dispatch_screen_key(
    const core::Screen_Set& screens,
    gui::App_View&          view,
    int                     key_index ) {
    const auto* screen = screens.find(view.active_panel_name());
    if (!screen) {
        return false;
    }

    if (const auto* key = screen->key_at(key_index)) {
        LOG_DEBUG("Keypress: screen '", screen->name,
                  "' key_index=", std::to_string(key_index));
        if (!key->panel.empty()) {
            view.push_panel(key->panel);
        } else if (key->action != core::Action_Code::NONE) {
            view.handle_input(key->action);
        } else if (key->text != 0) {
            view.handle_text(key->text);
        }
    }
    return true;
}

/****************************/
/*   Dispatch layout key    */
/****************************/
void dispatch_layout_key(
    core::Layer_Manager&               layers,
    const core::Screen_Set&            screens,
    gui::App_View&                     view,
    gui::I_Keymap_View*                keymap_view,
    int                                key_index,
    std::optional<core::Input_Key>     input_key ) {
    // Flash the key on the active presenter for visual feedback
    if (keymap_view) {
        keymap_view->flash_key(key_index);
    }

    // Per-panel screens own their key table while active
    if (dispatch_screen_key(screens, view, key_index)) {
        return;
    }

    // Layer actions route through the view — the active panel owns
    // layer-switch and all other action codes
    const core::Action_Code code = layers.action_at(key_index);
    if (code != core::Action_Code::NONE) {
        LOG_DEBUG("Keypress: key_index=", key_index,
                  ", action_code=", static_cast<int>(code),
                  " (", core::action_code_to_display(code), ")");
        view.handle_input(code);
        return;
    }

    // No action — pass through as raw Input_Key (F-keys, etc.)
    if (input_key && *input_key != core::Input_Key::NONE) {
        LOG_DEBUG("Keypress: key_index=", key_index, " -> Input_Key passthrough");
        view.handle_input_key(*input_key);
        return;
    }

    // Fall back to text for single-char labels with no binding
    const std::string label = layers.label_at(key_index);
    if (label.length() == 1) {
        LOG_DEBUG("Keypress: key_index=", key_index, ", label='", label, "' -> text");
        view.handle_text(static_cast<char32_t>(label[0]));
        return;
    }

    LOG_DEBUG("Keypress: key_index=", key_index, ", no action, label='", label, "' -> ignored");
}

} // namespace ovb::hal
