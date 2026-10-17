/**
 * @file    theme.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 .Theme file encoder implementation.
 */
#include <overboard/mpad/theme.hpp>

// C++ Standard Libraries
#include <cstring>

namespace ovb::mpad {

namespace {

/***************************/
/*     Base64 encoder      */
/***************************/
std::string base64_encode( std::span<const std::byte> data ) {
    static constexpr char ALPHABET[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve( (data.size() + 2) / 3 * 4 );
    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const uint32_t n = (static_cast<uint32_t>( data[i] ) << 16)
                         | (static_cast<uint32_t>( data[i + 1] ) << 8)
                         | static_cast<uint32_t>( data[i + 2] );
        out += ALPHABET[(n >> 18) & 63];
        out += ALPHABET[(n >> 12) & 63];
        out += ALPHABET[(n >> 6) & 63];
        out += ALPHABET[n & 63];
    }
    const size_t rem = data.size() - i;
    if (rem == 1) {
        const uint32_t n = static_cast<uint32_t>( data[i] ) << 16;
        out += ALPHABET[(n >> 18) & 63];
        out += ALPHABET[(n >> 12) & 63];
        out += "==";
    } else if (rem == 2) {
        const uint32_t n = (static_cast<uint32_t>( data[i] ) << 16)
                         | (static_cast<uint32_t>( data[i + 1] ) << 8);
        out += ALPHABET[(n >> 18) & 63];
        out += ALPHABET[(n >> 12) & 63];
        out += ALPHABET[(n >> 6) & 63];
        out += '=';
    }
    return out;
}

void put_u32be( std::vector<std::byte>& out, uint32_t v ) {
    for (int i = 3; i >= 0; --i) {
        out.push_back( static_cast<std::byte>( (v >> (i * 8)) & 0xFF ) );
    }
}

} // anonymous namespace

/***************************/
/*      Encode action      */
/***************************/
std::string encode_action( const Map& fields ) {
    return base64_encode( encode_tagged_map( fields ) );
}

/***************************/
/*      Text action        */
/***************************/
Map text_action( const std::string& input_text,
                 const std::string& description,
                 bool is_enter ) {
    return Map{
        {"type",                  Value{"text"}},
        {"parentDescription",     Value{"System input control"}},
        {"isInputEnter",          Value{is_enter}},
        {"isCopyPaste",           Value{false}},
        {"inputText",             Value{input_text}},
        {"iconPath",              Value{"/static/icon/dark/Text.png"}},
        {"description",           Value{description}},
        {"AISoundControlKeyword", Value{""}},
    };
}

/***************************/
/*    Keyboard action      */
/***************************/
Map keyboard_action( int keycode, const std::string& key_string ) {
    // Vendor keyboard actions carry exactly 4 fields in this order
    // (confirmed byte-for-byte against the macOS app's embedded theme).
    return Map{
        {"type",        Value{"keyboard"}},
        {"keycode",     Value{static_cast<int32_t>( keycode )}},
        {"keyString",   Value{key_string}},
        {"description", Value{"Keyboard"}},
    };
}

/***************************/
/* Encoder keyboard action */
/***************************/
Map encoder_keyboard_action( int cw_keycode,
                             const std::string& cw_key_string,
                             int click_keycode,
                             const std::string& click_key_string,
                             int ccw_keycode,
                             const std::string& ccw_key_string,
                             const std::string& description ) {
    return Map{
        {"type",                    Value{"encoder_keyboard"}},
        {"parentDescription",       Value{"Encoder"}},
        {"iconPath",                Value{"/static/icon/dark/keyboard.png"}},
        {"encoder_right_keycode",   Value{static_cast<int32_t>( cw_keycode )}},
        {"encoder_right_keyString", Value{cw_key_string}},
        {"encoder_middle_keycode",  Value{static_cast<int32_t>( click_keycode )}},
        {"encoder_middle_keyString",Value{click_key_string}},
        {"encoder_left_keycode",    Value{static_cast<int32_t>( ccw_keycode )}},
        {"encoder_left_keyString",  Value{ccw_key_string}},
        {"description",             Value{description}},
        {"category",                Value{"encoder"}},
    };
}

/***************************/
/*        Key item         */
/***************************/
nlohmann::json key_item( int row, int col,
                         const Map& action,
                         const std::string& icon_asset_path,
                         const std::string& item_id,
                         const std::string& title ) {
    nlohmann::json item;
    item["type"]     = "115";
    item["id"]       = item_id;
    item["itemName"] = "control" + item_id;
    item["x"]        = std::to_string( col * KEY_SIZE );
    item["y"]        = std::to_string( KEY_GRID_Y + row * KEY_SIZE );
    item["z"]        = "1";
    item["rotate"]   = "0";
    item["scale"]    = "1";
    item["lock"]     = "1";

    item["row"]         = std::to_string( row );
    item["col"]         = std::to_string( col );
    item["path"]        = icon_asset_path;
    item["controlData"] = encode_action( action );
    item["maxWidth"]    = std::to_string( CANVAS_W );
    item["maxHeight"]   = std::to_string( CANVAS_H );
    item["opacity"]     = "100";
    item["paths"]       = "";
    item["scaledWidthTo"]  = std::to_string( KEY_SIZE );
    item["scaledHeightTo"] = std::to_string( KEY_SIZE );
    item["soundFile"]   = "";
    item["title"]       = title;
    // titleParam is a compact-JSON *string*, not a nested object —
    // the device fails to parse the item if it receives an object.
    item["titleParam"]  = nlohmann::ordered_json{
        {"FontFamily",      "Microsoft YaHei"},
        {"FontSize",        24},
        {"FontStyle",       ""},
        {"FontUnderline",   false},
        {"ShowImage",       true},
        {"ShowTitle",       !title.empty()},
        {"TitleAlignment",  "bottom"},
        {"TitleColor",      "#ffffff"},
    }.dump();
    return item;
}

/***************************/
/*      Encoder item       */
/***************************/
nlohmann::json encoder_item( int pseudo_row,
                             const Map& action,
                             const std::string& icon_asset_path,
                             const std::string& item_id ) {
    const int x = (pseudo_row == ENCODER_LEFT_ROW) ? ENCODER_LEFT_X
                                                   : ENCODER_RIGHT_X;
    auto item = key_item( pseudo_row, 0, action, icon_asset_path, item_id, "" );
    item["x"]       = std::to_string( x );
    item["y"]       = std::to_string( ENCODER_Y );
    item["opacity"] = "0";
    return item;
}

/***************************/
/*        Make page        */
/***************************/
nlohmann::json make_page( const nlohmann::json& items,
                          const std::string& page_name ) {
    nlohmann::json p;
    p["canvas"] = nlohmann::json{
        {"canvas_w",      std::to_string( CANVAS_W )},
        {"canvas_h",      std::to_string( CANVAS_H )},
        {"canvas_flip",   "0"},
        {"canvas_rotate", "0"},
        {"showUnit",      "1"},
    };
    p["encoder"]  = DEFAULT_ENCODER;
    p["items"]    = items;
    p["pageName"] = page_name;
    return p;
}

/***************************/
/*      Encode theme       */
/***************************/
std::vector<std::byte> encode_theme( const Theme_File& theme ) {
    // Tagged-map header (language, keyMacroValue, keyMacro=null)
    Map header;
    header["language"]      = Value{ static_cast<int32_t>( theme.language ) };
    header["keyMacroValue"] = Value{ KEY_MACRO_VALUE };
    header["keyMacro"]      = Value{ std::monostate{} };
    std::vector<std::byte> out = encode_tagged_map( header );

    // JSON layout blob (nlohmann::json sorts keys recursively,
    // matching the Python reference's sort_keys=True output)
    nlohmann::json layout;
    std::string current = theme.current_page;
    if (current.empty() && !theme.pages.empty()) {
        current = theme.pages[0].value( "pageName", "" );
    }
    layout["main"]  = nlohmann::json{
        {"currentPage", current},
        {"version",     theme.layout_version},
    };
    layout["pages"] = theme.pages;
    const std::string json_str = layout.dump( 4 );

    // 4 zero bytes + u32BE json-len+1 + json + 0x0A
    for (int i = 0; i < 4; ++i) {
        out.push_back( std::byte{ 0 } );
    }
    put_u32be( out, static_cast<uint32_t>( json_str.size() ) + 1 );
    for (char c : json_str) {
        out.push_back( static_cast<std::byte>( c ) );
    }
    out.push_back( std::byte{ 0x0A } );

    // Asset section
    put_u32be( out, static_cast<uint32_t>( theme.assets.size() ) );
    for (const auto& a : theme.assets) {
        const auto pb = utf8_to_utf16be( a.path );
        put_u32be( out, static_cast<uint32_t>( pb.size() ) );
        out.insert( out.end(), pb.begin(), pb.end() );
        put_u32be( out, static_cast<uint32_t>( a.data.size() ) );
        out.insert( out.end(), a.data.begin(), a.data.end() );
    }

    // Required trailer
    for (int i = 0; i < 4; ++i) {
        out.push_back( std::byte{ 0 } );
    }
    return out;
}

} // namespace ovb::mpad
