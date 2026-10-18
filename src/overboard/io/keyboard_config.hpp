/**
 * @file    keyboard_config.hpp
 * @author  Marvin Smith
 * @date    2026-06-03
 *
 * @brief   Unified keyboard configuration parser
 *
 * Parses the unified keyboard.json format which combines key definitions,
 * positions, and layer mappings into a single file.
 */
#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/keymap.hpp>
#include <overboard/core/screen.hpp>

namespace ovb::io {

/**
 * @brief Key position and sizing
 */
struct Key_Position {
    double x;        ///< X offset in key units
    double y;        ///< Y offset in key units
    double w;        ///< Width in key units (default 1.0)
    double h;        ///< Height in key units (default 1.0)
};

/**
 * @brief Static key definition (hardware info + position)
 */
struct Key_Definition {
    std::string id;              ///< Numeric key ID
    std::string input_key;       ///< Hardware-agnostic input key name (e.g., "KEY_Q", "DIGIT_1")
    Key_Position position;       ///< Visual position
};

/**
 * @brief Layer-specific key mapping
 */
struct Layer_Key {
    std::string code;            ///< Action code (e.g., "NONE", "DIGIT_1", "LETTER_Q")
    std::string label;           ///< Display label for this layer
};

/**
 * @brief Layer definition
 */
struct Layer {
    std::string name;                              ///< Layer name (e.g., "Basic", "Shift", "Caps")
    std::map<std::string, Layer_Key> keys;         ///< Key ID -> Layer mapping
};

/**
 * @brief Complete keyboard configuration
 */
struct Keyboard_Config {
    std::string name;                                ///< Keyboard name
    std::string description;                         ///< Keyboard description

    struct Layout_Params {
        int key_width;       ///< Key width in pixels
        int key_height;      ///< Key height in pixels
        int key_spacing;     ///< Spacing between keys in pixels
        int origin_x;        ///< X origin offset
        int origin_y;        ///< Y origin offset
    } layout;

    std::map<std::string, Key_Definition> keys;      ///< Key ID -> Definition
    std::vector<Layer> layers;                         ///< Layer definitions

    /// @brief Optional attached input device (e.g. MK20 macropad)
    struct Device_Config {
        std::string type;    ///< Device type (e.g. "mk20")
        std::string port;    ///< Serial port path, or "auto" to discover
    };
    std::optional<Device_Config> device;              ///< Present if config declares a device

    /// @brief Per-panel MK20 screens, keyed by panel name ("screens" section)
    core::Screen_Set screens;
};

/**
 * @brief Parse unified keyboard JSON configuration
 * @param json_path Path to the keyboard.json file
 * @return Parsed configuration
 */
Keyboard_Config parse_keyboard_config(const std::filesystem::path& json_path);

/**
 * @brief Parse unified keyboard JSON configuration from string
 * @param json_string JSON string content
 * @return Parsed configuration
 */
Keyboard_Config parse_keyboard_config_string(const std::string& json_string);

/**
 * @brief Get the active layer based on modifier state
 * @param config Keyboard configuration
 * @param shift_pressed Whether shift is pressed
 * @param caps_lock_on Whether caps lock is on
 * @return Index of the active layer (0 = Basic, 1 = Shift, 2 = Caps)
 */
size_t get_active_layer_index(const Keyboard_Config& config, bool shift_pressed, bool caps_lock_on);

/**
 * @brief Get effective key label for display
 * @param key_def Key definition
 * @param layer_key Layer-specific override (can be nullptr)
 * @return Display label to show
 */
std::string get_key_display_label(const Key_Definition& key_def, const Layer_Key* layer_key);

/**
 * @brief Get effective action code
 * @param key_def Key definition
 * @param layer_key Layer-specific override (can be nullptr)
 * @return Action code
 */
std::string get_key_action_code(const Key_Definition& key_def, const Layer_Key* layer_key);

/**
 * @brief Convert a Keyboard_Config into a Grid_Layout
 * @param config Parsed keyboard configuration
 * @return Grid layout for use in display/input systems
 */
core::Grid_Layout to_grid_layout(const Keyboard_Config& config);

/**
 * @brief Load keyboard config with embedded fallback
 * @param layout_path Path to keyboard.json (or parent directory containing it)
 * @param use_embedded_fallback Whether to try embedded resource on file load failure (default: true)
 * @return Loaded keyboard configuration
 * @throws std::runtime_error if loading fails
 */
Keyboard_Config load_keyboard_config(
    const std::filesystem::path& layout_path,
    bool use_embedded_fallback = true
);

/**
 * @brief Convert Keyboard_Config layers to core::Keymap
 * @param config Keyboard configuration with layer definitions
 * @return Initialized Keymap with all layers converted
 */
core::Keymap config_to_keymap(const Keyboard_Config& config);

} // namespace ovb::io
