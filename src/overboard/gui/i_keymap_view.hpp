/**
 * @file    i_keymap_view.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   Abstract keymap presenter interface.
 *
 * Presents the current layer's key labels on some output surface.
 * Implementations: Key_Mapping_Info (on-screen LVGL panel) and
 * hal::mk20::Keymap (physical MK20 macropad, pushes themes over serial).
 */
#pragma once

// C++ Standard Libraries
#include <string>
#include <vector>

// Project Libraries
#include <overboard/gui/i_panel.hpp>

namespace ovb::gui {

/**
 * @brief Keymap presenter interface
 *
 * Subscribed to layer changes and panel overlays; renders the current
 * layer's key labels on whatever surface the implementation drives.
 */
class I_Keymap_View {
    public:

        /// @brief Virtual destructor
        virtual ~I_Keymap_View() = default;

        /// @brief Re-render key labels for the current layer
        virtual void update_layer() = 0;

        /**
         * @brief Push an overlay frame onto the stack
         * @param title Header text for this overlay
         * @param keys  Overlay keys to display on this frame
         */
        virtual void push_overlay(
            const std::string& title,
            const std::vector<I_Panel::Overlay_Key_Desc>& keys ) = 0;

        /// @brief Pop the top overlay frame
        virtual void pop_overlay() = 0;

        /// @brief Briefly flash a key to show it was pressed
        virtual void flash_key( int key_index ) = 0;
};

} // namespace ovb::gui
