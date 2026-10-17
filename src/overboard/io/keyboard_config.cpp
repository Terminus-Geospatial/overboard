/**
 * @file    keyboard_config.cpp
 * @author  Marvin Smith
 * @date    2026-06-03
 *
 * @brief   Unified keyboard configuration parser implementation
 */
#include "keyboard_config.hpp"

// C++ Standard Libraries
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

// Third-Party Libraries
#include <nlohmann/json.hpp>

// Project Libraries
#include <overboard/resources/embedded_json.hpp>

namespace ovb::io {

using json = nlohmann::json;

/*******************************/
/*  Parse Key Position Object  */
/*******************************/
static Key_Position parse_position(const json& pos_json) {
    Key_Position pos{};
    pos.x = pos_json.value("x", 0.0);
    pos.y = pos_json.value("y", 0.0);
    pos.w = pos_json.value("w", 1.0);
    pos.h = pos_json.value("h", 1.0);
    return pos;
}

/*******************************/
/* Parse Layer Key Override    */
/*******************************/
static Layer_Key parse_layer_key(const json& key_json) {
    Layer_Key key{};
    key.code = key_json.value("code", "NONE");
    key.label = key_json.value("label", "");
    return key;
}

/*******************************/
/*  Parse Keyboard Config      */
/*******************************/
Keyboard_Config parse_keyboard_config(const std::filesystem::path& json_path) {
    // Read JSON file
    std::ifstream file(json_path);
    if (!file.is_open()) {
#ifdef __cpp_exceptions
        throw std::runtime_error("Failed to open keyboard config: " + json_path.string());
#else
        std::cerr << "[keyboard_config] Failed to open: " << json_path.string() << "\n";
        return {};
#endif
    }

    json data;
#ifdef __cpp_exceptions
    try {
        file >> data;
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
#else
    file >> data;
#endif

    Keyboard_Config config{};

    // Parse metadata
    config.name = data.value("name", "Unknown");
    config.description = data.value("description", "");

    // Parse layout parameters
    if (data.contains("layout")) {
        const auto& layout = data["layout"];
        config.layout.key_width = layout.value("key_width", 40);
        config.layout.key_height = layout.value("key_height", 40);
        config.layout.key_spacing = layout.value("key_spacing", 2);
        config.layout.origin_x = layout.value("origin_x", 8);
        config.layout.origin_y = layout.value("origin_y", 6);
    }

    // Parse key definitions
    if (data.contains("keys")) {
        for (const auto& [key_id, key_data] : data["keys"].items()) {
            Key_Definition def{};
            def.id = key_id;
            def.input_key = key_data.value("input_key", "");

            if (key_data.contains("position")) {
                def.position = parse_position(key_data["position"]);
            }

            config.keys[key_id] = std::move(def);
        }
    }

    // Parse layers
    if (data.contains("layers")) {
        for (const auto& layer_data : data["layers"]) {
            Layer layer{};
            layer.name = layer_data.value("name", "Unknown");

            if (layer_data.contains("keys")) {
                for (const auto& [key_id, key_json] : layer_data["keys"].items()) {
                    layer.keys[key_id] = parse_layer_key(key_json);
                }
            }

            config.layers.push_back(std::move(layer));
        }
    }

    // Parse optional attached device (e.g. MK20 macropad)
    if (data.contains("device")) {
        const auto& dev = data["device"];
        config.device = Keyboard_Config::Device_Config{
            .type = dev.value("type", ""),
            .port = dev.value("port", "auto"),
        };
    }

    return config;
}

/**********************************/
/*  Parse Keyboard Config String  */
/**********************************/
Keyboard_Config parse_keyboard_config_string(const std::string& json_string) {
    json data;
#ifdef __cpp_exceptions
    try {
        data = json::parse(json_string);
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse JSON string: " + std::string(e.what()));
    }
#else
    data = json::parse(json_string);
#endif

    Keyboard_Config config{};

    // Parse metadata
    config.name = data.value("name", "Unknown");
    config.description = data.value("description", "");

    // Parse layout parameters
    if (data.contains("layout")) {
        const auto& layout = data["layout"];
        config.layout.key_width = layout.value("key_width", 40);
        config.layout.key_height = layout.value("key_height", 40);
        config.layout.key_spacing = layout.value("key_spacing", 2);
        config.layout.origin_x = layout.value("origin_x", 8);
        config.layout.origin_y = layout.value("origin_y", 6);
    }

    // Parse key definitions
    if (data.contains("keys")) {
        for (const auto& [key_id, key_data] : data["keys"].items()) {
            Key_Definition def{};
            def.id = key_id;
            def.input_key = key_data.value("input_key", "");

            if (key_data.contains("position")) {
                def.position = parse_position(key_data["position"]);
            }

            config.keys[key_id] = std::move(def);
        }
    }

    // Parse layers
    if (data.contains("layers")) {
        for (const auto& layer_data : data["layers"]) {
            Layer layer{};
            layer.name = layer_data.value("name", "Unknown");

            if (layer_data.contains("keys")) {
                for (const auto& [key_id, key_data] : layer_data["keys"].items()) {
                    Layer_Key key{};
                    key.code = key_data.value("code", "NONE");
                    key.label = key_data.value("label", "");
                    layer.keys[key_id] = std::move(key);
                }
            }

            config.layers.push_back(std::move(layer));
        }
    }

    // Parse optional attached device (e.g. MK20 macropad)
    if (data.contains("device")) {
        const auto& dev = data["device"];
        config.device = Keyboard_Config::Device_Config{
            .type = dev.value("type", ""),
            .port = dev.value("port", "auto"),
        };
    }

    return config;
}

/*******************************/
/*  Get Active Layer Index     */
/*******************************/
size_t get_active_layer_index(const Keyboard_Config& config, bool shift_pressed, bool caps_lock_on) {
    // Find layer by name - this is a simple implementation
    // Priority: Shift > Caps > Basic

    if (shift_pressed) {
        for (size_t i = 0; i < config.layers.size(); ++i) {
            if (config.layers[i].name == "Shift") {
                return i;
            }
        }
    }

    if (caps_lock_on) {
        for (size_t i = 0; i < config.layers.size(); ++i) {
            if (config.layers[i].name == "Caps") {
                return i;
            }
        }
    }

    // Default to first layer (Basic)
    return 0;
}

/*******************************/
/*  Get Key Display Label      */
/*******************************/
std::string get_key_display_label(const Key_Definition& key_def, const Layer_Key* layer_key) {
    // Layer label takes highest priority
    if (layer_key && !layer_key->label.empty()) {
        return layer_key->label;
    }

    // Derive from input_key
    const std::string& ik = key_def.input_key;

    // Arrow keys
    if (ik == "UP") return "↑";
    if (ik == "DOWN") return "↓";
    if (ik == "LEFT") return "←";
    if (ik == "RIGHT") return "→";

    // Function keys
    if (ik.starts_with("F")) return ik;

    // Special keys
    if (ik == "ESCAPE") return "Esc";
    if (ik == "TAB") return "Tab";
    if (ik == "CAPS_LOCK") return "CapsLk";
    if (ik == "DELETE") return "Del";
    if (ik == "BACKSPACE") return "Bsp";
    if (ik == "ENTER") return "Enter";
    if (ik == "SPACE") return "␣";
    if (ik == "SHIFT") return "Shift";
    if (ik == "CONTROL") return "Ctrl";
    if (ik == "ALT") return "Alt";

    // Symbol keys
    if (ik == "GRAVE") return "`";
    if (ik == "SLASH") return "/";
    if (ik == "BACKSLASH") return "\\";
    if (ik == "MINUS") return "-";
    if (ik == "EQUAL") return "=";
    if (ik == "BRACKET_LEFT") return "[";
    if (ik == "BRACKET_RIGHT") return "]";
    if (ik == "SEMICOLON") return ";";
    if (ik == "APOSTROPHE") return "'";
    if (ik == "COMMA") return ",";
    if (ik == "PERIOD") return ".";
    if (ik == "AMPERSAND") return "&";

    // Digit keys
    if (ik.starts_with("DIGIT_")) return ik.substr(6);

    // Letter keys
    if (ik.starts_with("KEY_")) {
        std::string letter = ik.substr(4);
        // Convert to lowercase
        if (!letter.empty()) {
            letter[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(letter[0])));
        }
        return letter;
    }

    // Last resort: return the key ID
    return key_def.id;
}

/*******************************/
/*  Get Key Action Code        */
/*******************************/
std::string get_key_action_code(const Key_Definition& key_def, const Layer_Key* layer_key) {
    // Layer code takes priority
    if (layer_key && !layer_key->code.empty()) {
        return layer_key->code;
    }

    // Try to infer from input_key
    if (!key_def.input_key.empty()) {
        // Map common input keys to action codes
        if (key_def.input_key.starts_with("KEY_")) {
            return "LETTER_" + key_def.input_key.substr(4);
        }
        if (key_def.input_key.starts_with("DIGIT_")) {
            return key_def.input_key;  // Already correct
        }
        if (key_def.input_key == "SPACE") {
            return "SPACE";
        }
        if (key_def.input_key == "ENTER") {
            return "RETURN";
        }
        return key_def.input_key;
    }

    return "NONE";
}

/*******************************/
/*  Keyboard Config to Grid    */
/*******************************/
core::Grid_Layout to_grid_layout(const Keyboard_Config& config) {
    // Sort keys by numeric ID to ensure consistent ordering
    std::vector<std::pair<int, const Key_Definition*>> sorted_keys;
    for (const auto& [id_str, key_def] : config.keys) {
        sorted_keys.emplace_back(std::stoi(id_str), &key_def);
    }
    std::sort(sorted_keys.begin(), sorted_keys.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::vector<core::Key_Position> positions;
    positions.reserve(sorted_keys.size());

    double max_col = 0.0;
    double max_row = 0.0;

    for (const auto& [id, key_def] : sorted_keys) {
        const auto& p = key_def->position;
        int col      = static_cast<int>(p.x);
        int row      = static_cast<int>(p.y);
        int col_span = static_cast<int>(p.w);
        int row_span = static_cast<int>(p.h);
        float col_gap = static_cast<float>(p.x - static_cast<double>(col));
        float row_gap = static_cast<float>(p.y - static_cast<double>(row));
        float width   = static_cast<float>(p.w);
        float height  = static_cast<float>(p.h);

        positions.emplace_back(col, row, col_span, row_span, col_gap, row_gap, width, height);
        max_col = std::max(max_col, p.x + p.w);
        max_row = std::max(max_row, p.y + p.h);
    }

    int grid_cols = static_cast<int>(std::ceil(max_col));
    int grid_rows = static_cast<int>(std::ceil(max_row));

    return core::Grid_Layout(grid_cols, grid_rows, std::move(positions));
}

/*******************************/
/*  Load Keyboard Config       */
/*******************************/
Keyboard_Config load_keyboard_config(
    [[maybe_unused]] const std::filesystem::path& layout_path,
    bool use_embedded_fallback
) {
#ifdef __EMSCRIPTEN__
    // WASM builds use embedded JSON exclusively; avoid filesystem access
    if (use_embedded_fallback) {
        return parse_keyboard_config_string(
            std::string(ovb::resources::embedded_json_data,
                       ovb::resources::embedded_json_size)
        );
    }
    throw std::runtime_error("No embedded keyboard config available for WASM");
#else
    // Try loading from file
    std::filesystem::path json_path = layout_path;
    if (std::filesystem::is_directory(layout_path)) {
        json_path = layout_path / "keyboard.json";
    }

#ifdef __cpp_exceptions
    try {
        return parse_keyboard_config(json_path);
    } catch (const std::exception& e) {
        // If embedded fallback is enabled, try that
        if (use_embedded_fallback) {
            try {
                return parse_keyboard_config_string(
                    std::string(ovb::resources::embedded_json_data,
                               ovb::resources::embedded_json_size)
                );
            } catch (const std::exception& e2) {
                throw std::runtime_error(
                    "Failed to load keyboard config from file (" +
                    std::string(e.what()) + ") and embedded resource (" +
                    std::string(e2.what()) + ")"
                );
            }
        }
        // Re-throw original exception if no fallback
        throw;
    }
#else
    Keyboard_Config cfg = parse_keyboard_config(json_path);
    if (cfg.keys.empty() && use_embedded_fallback) {
        cfg = parse_keyboard_config_string(
            std::string(ovb::resources::embedded_json_data,
                        ovb::resources::embedded_json_size)
        );
    }
    return cfg;
#endif
#endif
}

/*******************************/
/*  Convert Config to Keymap   */
/*******************************/
core::Keymap config_to_keymap(const Keyboard_Config& config) {
    std::array<core::Layer, core::LAYER_COUNT> layers;
    layers.fill(core::Layer{});  // Initialize with empty layers

    for (std::size_t i = 0; i < config.layers.size() && i < static_cast<std::size_t>(core::LAYER_COUNT); ++i) {
        const auto& config_layer = config.layers[i];
        layers[i].name = config_layer.name;

        // Convert layer keys from string ID map to vector indexed by key ID
        for (const auto& [key_id_str, layer_key] : config_layer.keys) {
            std::size_t key_id = static_cast<std::size_t>(std::stoi(key_id_str));

            // Convert code string to Action_Code
            core::Action_Code code = core::string_to_action_code(layer_key.code);

            // Ensure layer.keys and labels vectors are large enough
            if (layers[i].keys.size() <= key_id) {
                layers[i].keys.resize(key_id + 1, core::Action_Code::NONE);
                layers[i].labels.resize(key_id + 1, "");
            }

            layers[i].keys[key_id]   = code;
            layers[i].labels[key_id] = layer_key.label;
        }
    }

    return core::Keymap(layers);
}

} // namespace ovb::io
