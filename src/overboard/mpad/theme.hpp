/**
 * @file    theme.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 .Theme file model and encoder.
 *
 * A .Theme file is a binary container:
 *   - tagged-map header (language, keyMacroValue, keyMacro)
 *   - 4 zero bytes + u32BE length + UTF-8 JSON layout + 0x0A
 *   - asset count + assets (UTF-16BE path + raw data)
 *   - 4-byte zero trailer
 *
 * Ported from the Python reference implementation (terminus.mpad).
 */
#pragma once

// C++ Standard Libraries
#include <cstdint>
#include <string>
#include <vector>

// Third-Party Libraries
#include <nlohmann/json.hpp>

// Project Libraries
#include <overboard/mpad/protocol.hpp>

namespace ovb::mpad {

/// Fixed 92-byte blob carried by every vendor theme — the literal
/// base64 *text* (ASCII), not the decoded bytes. A wrong/empty value
/// leaves `text` keys completely inert while `keyboard` keys keep
/// working — confirmed on real hardware.
inline const std::vector<std::byte> KEY_MACRO_VALUE = {
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'E'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'A'},std::byte{'A'},std::byte{'A'},
    std::byte{'A'},std::byte{'='}
};

/// Canvas geometry (device is 640x656 logical)
inline constexpr int CANVAS_W   { 640 };
inline constexpr int CANVAS_H   { 656 };
inline constexpr int KEY_SIZE   { 128 };
inline constexpr int KEY_GRID_Y { 144 };  ///< keys start below the 428x142 strip

/// Encoder pseudo-rows and positions
inline constexpr int ENCODER_LEFT_ROW  { 100 };
inline constexpr int ENCODER_RIGHT_ROW { 103 };
inline constexpr int ENCODER_LEFT_X    { 106 };
inline constexpr int ENCODER_RIGHT_X   { 320 };
inline constexpr int ENCODER_Y         { 0 };

/// Boilerplate encoder array required on every main-screen page
inline const nlohmann::json DEFAULT_ENCODER = nlohmann::json::array({
    {{"col", 0}, {"keyString", ""},  {"keycode", 0},     {"row", 103}},
    {{"col", 0}, {"keyString", ""},  {"keycode", 0},     {"row", 104}},
    {{"col", 0}, {"keyString", "E"}, {"keycode", 8},     {"row", 100}},
    {{"col", 0}, {"keyString", ""},  {"keycode", 38992}, {"row", 105}},
});

/// A single binary asset embedded in the theme (icon images etc.)
struct Theme_Asset {
    std::string           path;
    std::vector<std::byte> data;
};

/// A complete .Theme file
struct Theme_File {
    nlohmann::json            pages = nlohmann::json::array();
    std::vector<Theme_Asset>  assets;
    std::string               current_page;
    std::string               layout_version { "V3.0" };
    int                       language { 0 };

    /// Conventional device path: /data/theme/MK20/<name>/<name>.Theme
    static std::string device_path( const std::string& name ) {
        return "/data/theme/MK20/" + name + "/" + name + ".Theme";
    }
};

// ---------------------------------------------------------------------------
// Key actions — encoded into each key's controlData as base64(tagged-map)
// ---------------------------------------------------------------------------

/// Base64-encode a tagged action map (the controlData wire format)
std::string encode_action( const Map& fields );

/// Host-visible action: device reports the press; types nothing itself
Map text_action( const std::string& input_text,
                 const std::string& description = "Text",
                 bool is_enter = false );

/// Device-native HID keystroke action
Map keyboard_action( int keycode, const std::string& key_string = "" );

/// Rotation-aware encoder action — distinct keystrokes for CW/click/CCW.
/// Field names use the vendor's right/middle/left ordering
/// (right=CW, middle=click, left=CCW).
Map encoder_keyboard_action( int cw_keycode,
                             const std::string& cw_key_string = "",
                             int click_keycode = 0,
                             const std::string& click_key_string = "",
                             int ccw_keycode = 0,
                             const std::string& ccw_key_string = "",
                             const std::string& description = "Encoder" );

// ---------------------------------------------------------------------------
// Item builders. All numeric fields are JSON strings per vendor files.
// ---------------------------------------------------------------------------

/// Type-115 key. `title` renders over the icon when titleParam.ShowTitle.
nlohmann::json key_item( int row, int col,
                         const Map& action,
                         const std::string& icon_asset_path,
                         const std::string& item_id,
                         const std::string& title = "" );

/// Invisible encoder item at a pseudo-row (100=left, 103=right).
nlohmann::json encoder_item( int pseudo_row,
                             const Map& action,
                             const std::string& icon_asset_path,
                             const std::string& item_id );

/// Page container with required boilerplate
nlohmann::json make_page( const nlohmann::json& items,
                          const std::string& page_name );

/// Encode a Theme_File to the binary .Theme wire format
std::vector<std::byte> encode_theme( const Theme_File& theme );

} // namespace ovb::mpad
