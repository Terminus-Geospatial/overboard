/**
 * @file    app_common.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared init/dispatch helpers for host apps (SDL, Pi Zero)
 *
 * The SDL and Pi Zero application classes wire the same pieces during
 * init — keymap presenter selection, layer/panel callbacks, and
 * per-panel screen dispatch. These free functions hold the shared
 * parts; each app keeps its own ordering and platform specifics.
 * MK20-specific construction lives in hal/mk20/factory.hpp.
 */
#pragma once

// C++ Standard Libraries
#include <functional>
#include <memory>
#include <optional>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/core/input_key.hpp>
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/core/screen.hpp>
#include <overboard/gui/app_view.hpp>
#include <overboard/gui/i_keymap_view.hpp>
#include <overboard/gui/key_mapping_info.hpp>

namespace ovb::hal {

/**
 * @brief Create the on-screen key-mapping presenter
 *
 * Returns nullptr when the target reserves no keymap region
 * (KBD_HEIGHT == 0).
 *
 * @param parent   LVGL parent object (display screen)
 * @param layout   Keyboard layout
 * @param layers   Layer manager
 * @param view     App view (label callback)
 * @param click_cb Invoked on button clicks with the layout key index
 */
std::unique_ptr<gui::Key_Mapping_Info> make_keymap_panel(
    lv_obj_t*                  parent,
    const core::Grid_Layout&   layout,
    const core::Layer_Manager& layers,
    gui::App_View&             view,
    std::function<void(int)>   click_cb );

/**
 * @brief Wire layer/panel-change callbacks onto the keymap presenter
 *
 * Subscribes Layer_Manager::on_layer_change and the view's
 * panel-change callback; both call update_layer() on the current
 * presenter and (re)wire overlay callbacks on each activated panel.
 *
 * @param view           App view owning the panel manager
 * @param layers         Layer manager to subscribe
 * @param get_keymap_view Returns the active presenter (may change
 *                       after init, so it's a getter, not a pointer)
 */
void wire_keymap_view_callbacks(
    gui::App_View&                            view,
    core::Layer_Manager&                      layers,
    const std::function<gui::I_Keymap_View*()>& get_keymap_view );

/**
 * @brief Dispatch a layout key against the active panel's screen table
 *
 * When a screen owns the active panel, a bound key resolves to panel
 * jump, action, or text; an unbound key is inert.
 *
 * @param screens    Per-panel screen tables from keyboard.json
 * @param view       App view (panel name, push_panel, handle_input/text)
 * @param key_index  Layout key index
 * @return true when a screen owned the keypress — nothing else should run
 */
bool dispatch_screen_key(
    const core::Screen_Set& screens,
    gui::App_View&          view,
    int                     key_index );

/**
 * @brief Full layout-key dispatch — the shared key-handling chain
 *
 * Order: flash the presenter → per-panel screen table → layer action →
 * Input_Key passthrough → single-char label as text. Layer-switch
 * actions (GO_HOME_LAYER, NEXT_LAYER, ...) route through
 * view.handle_input like everything else; the active panel owns them.
 *
 * @param layers      Layer manager (action + label lookups)
 * @param screens     Per-panel screen tables
 * @param view        App view (input routing)
 * @param keymap_view Active presenter for flash feedback (may be null)
 * @param key_index   Layout key index
 * @param input_key   Original Input_Key for NONE-action passthrough
 *                   (nullopt for serial/click sources)
 */
void dispatch_layout_key(
    core::Layer_Manager&               layers,
    const core::Screen_Set&            screens,
    gui::App_View&                     view,
    gui::I_Keymap_View*                keymap_view,
    int                                key_index,
    std::optional<core::Input_Key>     input_key );

} // namespace ovb::hal
