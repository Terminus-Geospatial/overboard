/**
 * @file    app_view.hpp
 * @author  Marvin Smith
 * @date    2026-05-29
 *
 * @brief   LVGL application view — panel system host + optional keyboard display
 *
 * Owns a Panel_Manager and an optional Keyboard_Display. The Panel_Manager
 * starts with Status_Page and switches to Calculator_App on first keypress.
 * Platform-agnostic: works on both SDL simulator and embedded targets.
 */
#pragma once

// C++ Standard Libraries
#include <cstdint>
#include <functional>
#include <memory>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/core/input_key.hpp>
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/core/settings_manager.hpp>
#include <overboard/gui/panel_manager.hpp>
#include <overboard/hal/i_display.hpp>
#include <overboard/hal/i_system_info.hpp>
#include <overboard/math/calc_engine.hpp>

namespace ovb::gui {

/**
 * @brief LVGL application view
 *
 * Attaches LCD_Section (top) and Keyboard_Display (bottom) to a provided
 * LVGL root object. The root is typically the active screen supplied by
 * the platform HAL after window creation.
 */
class App_View : public hal::I_Display {
    public:

        /**
         * @brief Construct and build the application view
         *
         * @param root     LVGL root object to attach widgets to
         * @param layout   Keyboard grid layout for key positioning
         * @param engine   Calculation engine for expression state and history
         * @param layers   Layer manager for key label management
         * @param settings Application settings manager
         */
        App_View( lv_obj_t*                               root,
                  const core::Grid_Layout&                layout,
                  math::Calc_Engine&                      engine,
                  core::Layer_Manager&                    layers,
                  std::shared_ptr<core::Settings_Manager> settings,
                  hal::I_System_Info&                     system_info );

        /**
         * @brief Destructor
         */
        ~App_View() override;

        /**
         * @brief Route an action through the active panel
         * @param action The action to dispatch
         */
        void handle_input(core::Action_Code action);

        /**
         * @brief Forward a resolved text codepoint to the active panel
         * @param codepoint UTF-32 character from platform text input
         * @return true if text was consumed and panel refreshed itself
         */
        bool handle_text(char32_t codepoint);

        /**
         * @brief Forward an Input_Key to the active panel for context-dependent handling
         * @param key Input_Key from standard keyboard
         */
        void handle_input_key(core::Input_Key key);

        /// @brief Refresh the active panel from current state
        void refresh()      override;

        /// @brief Re-render keyboard display for the current layer
        void update_layer() override;

        /// @brief Drive the LVGL render loop (call once per frame)
        void render()       override;

        /**
         * @brief Get custom label for a key from the active panel
         * @param key_index The key index to query
         * @return Custom label string, or empty if none
         */
        std::string get_active_panel_label(int key_index) const;

        /// @brief Name of the active panel ("" when none)
        std::string active_panel_name() const;

        /**
         * @brief Push a registered panel by name (MK20 screen jump keys)
         * @param name Panel name matching I_Panel::name()
         * @return true if the panel was found and pushed
         */
        bool push_panel(const std::string& name);

        /**
         * @brief Register a callback fired when the active panel changes
         * @param cb Callback function to invoke on panel change
         */
        void set_panel_change_callback(std::function<void(I_Panel*)> cb);

        /**
         * @brief Manually trigger panel change callback for current active panel
         *
         * Used to wire up callbacks after they are set, for panels that were
         * already activated during construction.
         */
        void trigger_panel_change_callback();

    private:

        struct Impl;
        std::unique_ptr<Impl> m_impl;
};

} // namespace ovb::gui
