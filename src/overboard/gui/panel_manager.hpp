/**
 * @file    panel_manager.hpp
 * @author  Marvin Smith
 * @date    2026-06-01
 *
 * @brief   Manages a stack of application panels
 *
 * Owns all registered panels, maintains an active-panel stack, and
 * routes input and refresh calls to the top-of-stack panel.
 */
#pragma once

// C++ Standard Libraries
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/core/action_code.hpp>
#include <overboard/core/input_key.hpp>
#include <overboard/gui/i_panel.hpp>

namespace ovb::gui {

/**
 * @brief Owns and switches between application panels
 *
 * Panels are registered up front. The manager pushes/pops panels
 * onto a stack; the top-of-stack panel receives all input and
 * refresh calls. Switching to a new panel calls deactivate() on
 * the previous one and activate() on the new one.
 */
class Panel_Manager {

    public:

        /**
         * @brief Construct with the LVGL parent to render panels into
         * @param parent LVGL object that panels attach widgets to
         */
        explicit Panel_Manager(lv_obj_t* parent);

        /**
         * @brief Destructor
         */
        ~Panel_Manager() = default;

        /**
         * @brief Register a panel and return its assigned index
         * @param panel Shared pointer to the panel
         * @return Index that can be passed to push()
         */
        int register_panel(std::shared_ptr<I_Panel> panel);

        /**
         * @brief Push a panel onto the stack and activate it
         * @param index Panel index from register_panel()
         */
        void push(int index);

        /**
         * @brief Push a panel by its I_Panel::name()
         * @param name Panel name to find and activate
         * @return true if a panel with that name was found and pushed
         */
        bool push_by_name(const std::string& name);

        /**
         * @brief Pop the top panel and reactivate the one below
         * Does nothing if the stack has only one entry.
         */
        void pop();

        /**
         * @brief Route an action to the active panel
         * @param action Action to dispatch
         * @return true if consumed
         */
        bool handle_input(core::Action_Code action);

        /**
         * @brief Forward a text codepoint to the active panel
         * @param codepoint UTF-32 character
         * @return true if consumed
         */
        bool handle_text(char32_t codepoint);

        /**
         * @brief Forward an Input_Key to the active panel for context-dependent handling
         * @param key Input_Key from standard keyboard
         * @return true if consumed
         */
        bool handle_input_key(core::Input_Key key);

        /// @brief Refresh the active panel
        void refresh();

        /**
         * @brief Get the active panel, or nullptr if stack is empty
         */
        I_Panel* active_panel();

        /// @brief Number of registered panels
        int panel_count() const;

        /**
         * @brief Register a callback fired when the active panel changes
         * @param cb Callback function to invoke on panel change
         */
        void set_panel_change_callback(std::function<void(I_Panel*)> cb);

        /**
         * @brief Manually trigger panel change callback for current active panel
         */
        void trigger_panel_change_callback();

    private:

        /// @brief LVGL parent object for panels
        lv_obj_t*                              m_parent;

        /// @brief Registered panels by index
        std::vector<std::shared_ptr<I_Panel>>  m_panels;

        /// @brief Stack of active panel indices
        std::vector<int>                       m_stack;

        /// @brief Callback fired when active panel changes
        std::function<void(I_Panel*)>         m_panel_change_cb;
};

} // namespace ovb::gui
