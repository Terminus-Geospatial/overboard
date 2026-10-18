/**
 * @file    screen.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Per-panel MK20 "screen" definitions
 *
 * A Screen is the complete macropad presentation for one app panel:
 * strip telemetry items, dial bindings, and a per-key table that
 * overrides layer dispatch while the panel is active. Parsed from the
 * optional "screens" section of keyboard.json.
 */
#pragma once

// C++ Standard Libraries
#include <map>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/core/action_code.hpp>

namespace ovb::core {

/**
 * @brief One bound key on a screen
 *
 * Resolution order at dispatch: panel > action > text. A key with none
 * of these set is present visually (label) but inert.
 */
struct Screen_Key {
    Action_Code action { Action_Code::NONE };  ///< Route via handle_input
    std::string label;                         ///< Cap label
    std::string panel;                         ///< Non-empty: push this panel
    std::string image;                         ///< PNG icon path (resolved at parse time)
    char32_t    text  { 0 };                   ///< Non-zero: route via handle_text
};

/**
 * @brief Static or data-bound text on the 428x142 status strip
 */
struct Strip_Item {
    int         x { 0 }, y { 0 }, w { 0 }, h { 0 };
    int         font_size { 0 };  ///< Point size; 0 = derive from box height
    std::string text;         ///< Static text (when system_data empty)
    std::string system_data;  ///< Telemetry channel bound via push_system_data
};

/**
 * @brief The pad presentation for one named panel
 */
struct Screen {
    std::string name;

    /// Grid keys 0-19 plus encoder pseudo-keys 20-25
    /// (20=L click, 21=L CCW, 22=L CW, 23=R click, 24=R CCW, 25=R CW)
    std::map<int, Screen_Key> keys;

    /// Device-native function for the right dial (e.g.
    /// "encoder_system_volume"); empty = encoder_keyboard F-keys
    std::string right_dial_function;

    /// Status-strip widgets (top 428x142 area)
    std::vector<Strip_Item> strip;

    /// Look up a binding by key index; nullptr when unbound
    const Screen_Key* key_at( int key_index ) const {
        const auto it = keys.find( key_index );
        return (it != keys.end()) ? &it->second : nullptr;
    }

    /// True when any strip item binds a telemetry channel
    bool has_data_strip() const {
        for (const auto& item : strip) {
            if (!item.system_data.empty()) {
                return true;
            }
        }
        return false;
    }
};

/**
 * @brief All configured screens, keyed by panel name
 */
class Screen_Set {
    public:

        /**
         * @brief Find the screen for a panel name
         * @param panel_name I_Panel::name() of the active panel
         * @return Screen pointer, or nullptr when the panel has no screen
         */
        const Screen* find( const std::string& panel_name ) const {
            const auto it = m_screens.find( panel_name );
            return (it != m_screens.end()) ? &it->second : nullptr;
        }

        /// @brief True when no screens are defined
        bool empty() const { return m_screens.empty(); }

        /// @brief Number of defined screens
        std::size_t size() const { return m_screens.size(); }

        std::map<std::string, Screen> m_screens;
};

} // namespace ovb::core
