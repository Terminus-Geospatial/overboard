/**
 * @file    keymap.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 keymap presenter implementation.
 */
#include <overboard/hal/mk20/keymap.hpp>

// C++ Standard Libraries
#include <algorithm>
#include <array>
#include <ctime>
#include <map>
#include <thread>

// Project Libraries
#include <overboard/hal/mk20/input.hpp>
#include <overboard/io/file_utils.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/mpad/key_icon.hpp>
#include <overboard/mpad/theme.hpp>
#include <overboard/version.hpp>

namespace ovb::hal::mk20 {

namespace {

/// Dark-slate 128x128 solid PNG — shared icon for encoder and
/// background items. Rendered once at startup (the previously
/// embedded byte blob was a truncated PNG — its zlib stream
/// never terminated).
const std::vector<std::byte>& key_icon() {
    static const std::vector<std::byte> icon = mpad::render_key_icon( "" );
    return icon;
}

/**
 * @brief Icon bytes for a key — a configured PNG wins, else the
 * label is baked into a tile. Unbound keys get a plain dark tile.
 */
std::vector<std::byte> bake_key_icon( const std::string& label,
                                      const std::string& image_path ) {
    if (!image_path.empty()) {
        auto bytes = io::read_file_bytes( image_path );
        if (!bytes.empty()) {
            return bytes;
        }
        LOG_WARN( "Mk20_Keymap: icon unreadable: ", image_path );
    }
    auto png = mpad::render_key_icon( label );
    return png.empty() ? key_icon() : std::move( png );
}

/// Shared asset path inside each generated theme
constexpr const char* ICON_PATH = "/static/icon/ovb_key.png";

} // anonymous namespace

/***************************/
/*        Constructor      */
/***************************/
Mk20_Keymap::Mk20_Keymap( std::shared_ptr<mpad::Device>   device,
                          const core::Grid_Layout&        layout,
                          const core::Layer_Manager&      layers,
                          const core::Screen_Set&         screens,
                          std::function<std::string(int)> get_label_cb,
                          std::function<std::string()>    get_panel_name_cb,
                          hal::I_System_Info*             system_info,
                          std::string                     device_root )
    : m_device( std::move( device ) ),
      m_layout( layout ),
      m_layers( layers ),
      m_screens( screens ),
      m_get_label_cb( std::move( get_label_cb ) ),
      m_get_panel_name_cb( std::move( get_panel_name_cb ) ),
      m_system_info( system_info ),
      m_device_root( std::move( device_root ) ) {
}

/***************************/
/*      Update layer       */
/***************************/
void Mk20_Keymap::update_layer() {
    if (!m_overlays.empty()) {
        return;
    }
    if (const auto* screen = active_screen()) {
        ensure_screen_theme( *screen );
        activate_theme( screen_path( *screen ) );
        push_screen_data( *screen );
        return;
    }
    const int idx = m_layers.active_layer();
    ensure_layer_theme( idx );
    activate_theme( layer_path( idx ) );
}

/***************************/
/*       Push overlay      */
/***************************/
void Mk20_Keymap::push_overlay(
    const std::string& title,
    const std::vector<gui::I_Panel::Overlay_Key_Desc>& keys ) {
    m_overlays.push_back( Overlay_Frame{ title, keys } );
    // Overlays are transient — build + upload under a scratch name
    auto theme  = build_theme( m_layers.active_layer(), &m_overlays.back().keys );
    auto data   = mpad::encode_theme( theme );
    const auto path = m_device_root + "/ovb-overlay/ovb-overlay.Theme";
    m_device->upload_theme( data, path, false );
    activate_theme( path );
}

/***************************/
/*       Pop overlay       */
/***************************/
void Mk20_Keymap::pop_overlay() {
    if (!m_overlays.empty()) {
        m_overlays.pop_back();
    }
    if (m_overlays.empty()) {
        // Restore the base layer theme (already uploaded — instant)
        activate_theme( layer_path( m_layers.active_layer() ) );
    } else {
        // Re-render the new top overlay
        auto theme = build_theme( m_layers.active_layer(),
                                  &m_overlays.back().keys );
        auto data  = mpad::encode_theme( theme );
        const auto path = m_device_root + "/ovb-overlay/ovb-overlay.Theme";
        m_device->upload_theme( data, path, false );
        activate_theme( path );
    }
}

/***************************/
/*       Flash key         */
/***************************/
void Mk20_Keymap::flash_key( [[maybe_unused]] int key_index ) {
    // v1: no-op. Serial feedback would lag the physical device; the
    // MK20 key already shows its own press animation.
}

/***************************/
/*      Is connected       */
/***************************/
bool Mk20_Keymap::is_connected() const {
    return m_device && m_device->is_open();
}

/***************************/
/*          Tick           */
/***************************/
void Mk20_Keymap::tick() {
    const auto* screen = active_screen();
    if (!screen || !screen->has_data_strip()) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now - m_last_strip_push >= std::chrono::seconds(1)) {
        m_last_strip_push = now;
        push_screen_data( *screen );
    }
}

/***************************/
/*     Active screen       */
/***************************/
const core::Screen* Mk20_Keymap::active_screen() const {
    if (!m_get_panel_name_cb) {
        return nullptr;
    }
    return m_screens.find( m_get_panel_name_cb() );
}

/***************************/
/*   Ensure screen theme   */
/***************************/
void Mk20_Keymap::ensure_screen_theme( const core::Screen& screen ) {
    if (m_uploaded_screens.contains( screen.name )) {
        return;
    }
    auto theme = build_screen_theme( screen );
    auto data  = mpad::encode_theme( theme );
    LOG_INFO( "Mk20_Keymap: uploading screen '", screen.name,
              "' theme (", data.size(), " bytes)" );
    if (!m_device->upload_theme( data, screen_path( screen ), false )) {
        LOG_ERROR( "Mk20_Keymap: upload failed for screen '", screen.name, "'" );
        return;
    }
    m_uploaded_screens.insert( screen.name );
}

/***************************/
/*       Screen path       */
/***************************/
std::string Mk20_Keymap::screen_path( const core::Screen& screen ) const {
    std::string name = "ovb-s-";
    for (const char c : screen.name) {
        name += static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
    }
    return mpad::Theme_File::device_path( name );
}

/***************************/
/*    Build screen theme   */
/***************************/
mpad::Theme_File Mk20_Keymap::build_screen_theme(
    const core::Screen& screen,
    const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const {
    nlohmann::json items = nlohmann::json::array();

    // Per-screen asset dir — image keys embed PNGs under the theme
    // name; overlays share one scratch dir so their baked icons don't
    // collide with the base theme's assets at the same path.
    std::string asset_dir = "/image/ovb-overlay/";
    if (!overlay) {
        asset_dir = "/image/ovb-s-";
        for (const char c : screen.name) {
            asset_dir += static_cast<char>( std::tolower( static_cast<unsigned char>( c ) ) );
        }
        asset_dir += '/';
    }
    std::map<std::string, std::vector<std::byte>> image_assets;

    // Opaque backgrounds — the firmware doesn't repaint untouched
    // pixels on theme switch, so stale art persists without them.
    items.push_back( mpad::dynamic_image_item(
        asset_dir + "bg.png", "bg_grid", 0, mpad::KEY_GRID_Y,
        mpad::CANVAS_W, mpad::CANVAS_H - mpad::KEY_GRID_Y,
        -2, "main" ) );
    items.push_back( mpad::dynamic_image_item(
        asset_dir + "bg.png", "bg_strip", mpad::STRIP_X, 0,
        mpad::STRIP_W, mpad::STRIP_H, 1 ) );
    image_assets.emplace( asset_dir + "bg.png", key_icon() );

    // Grid keys — labels come from the screen's key table (overlay wins)
    const int key_count = std::min( Mk20_Input::KEY_COUNT,
                                    m_layout.key_count() );
    for (int i = 0; i < key_count; ++i) {
        const int row = i / Mk20_Input::COLS;
        const int col = i % Mk20_Input::COLS;
        const std::string label = label_for( i, m_layers.active_layer(), &screen, overlay );

        // Every key gets a baked, per-theme-unique icon path — the
        // firmware's repaint heuristic keys off the icon path, and
        // device-rendered titles leave stale bitmaps behind.
        const auto* skey = screen.key_at( i );
        const std::string icon = asset_dir + "k" + std::to_string( i ) + ".png";
        if (!image_assets.contains( icon )) {
            image_assets.emplace( icon,
                bake_key_icon( label, skey ? skey->image : "" ) );
        }

        auto act = mpad::text_action(
            "ovb.k" + std::to_string( i ), label );
        items.push_back( mpad::key_item(
            row, col, act, icon,
            "k" + std::to_string( i ), "" ) );
    }

    // Left dial is always host-routed (encoder_keyboard F13/17/14 →
    // key_index 20-22). Right dial may switch to a device-native
    // function (system volume / device brightness) per screen config.
    items.push_back( mpad::encoder_item(
        mpad::ENCODER_LEFT_ROW,
        mpad::encoder_keyboard_action(
            104, "",   // CW  -> F13
            108, "",   // click -> F17
            105, "",   // CCW -> F14
            "Encoder L" ),
        ICON_PATH, "enc_l" ) );
    if (screen.right_dial_function.empty()) {
        items.push_back( mpad::encoder_item(
            mpad::ENCODER_RIGHT_ROW,
            mpad::encoder_keyboard_action(
                106, "",   // CW  -> F15
                109, "",   // click -> F18
                107, "",   // CCW -> F16
                "Encoder R" ),
            ICON_PATH, "enc_r" ) );
    } else {
        items.push_back( mpad::encoder_item(
            mpad::ENCODER_RIGHT_ROW,
            mpad::encoder_function_action( screen.right_dial_function,
                                           "Encoder R" ),
            ICON_PATH, "enc_r" ) );
    }

    // Status strip widgets (428x142 area above the key grid)
    int strip_idx = 0;
    for (const auto& item : screen.strip) {
        // Font must fit the box — the firmware's empty-font default is
        // large enough to overflow into the key grid below.
        const int font_pt = (item.font_size > 0)
            ? item.font_size
            : std::max( 6, item.h * 3 / 5 );
        items.push_back( mpad::text_item(
            "strip" + std::to_string( strip_idx++ ),
            item.x, item.y, item.w, item.h,
            item.text, item.system_data, font_pt ) );
    }

    mpad::Theme_File theme;
    theme.pages.push_back( mpad::make_page( items, "ovb_screen_" + screen.name ) );
    theme.assets.push_back( mpad::Theme_Asset{ ICON_PATH, key_icon() } );
    for (auto& [asset_path, bytes] : image_assets) {
        theme.assets.push_back( mpad::Theme_Asset{ std::move( asset_path ),
                                                   std::move( bytes ) } );
    }
    return theme;
}

/***************************/
/*    Push screen data     */
/***************************/
void Mk20_Keymap::push_screen_data( const core::Screen& screen ) {
    std::map<std::string, std::string> values;

    const std::time_t t = std::time( nullptr );
    std::tm tm_buf{};
    if (localtime_r( &t, &tm_buf )) {
        char clock_buf[8];
        std::strftime( clock_buf, sizeof clock_buf, "%H:%M", &tm_buf );
        values["clock"] = clock_buf;
        char date_buf[12];
        std::strftime( date_buf, sizeof date_buf, "%a %b %d", &tm_buf );
        values["date"] = date_buf;
    }
    values["app"] = screen.name;
    values["ver"] = PROJECT_VERSION_DISPLAY;
    if (m_system_info && m_system_info->has_wifi()) {
        const auto info = m_system_info->get_info();
        values["wifi"] = (info.wifi && info.wifi->connected)
            ? info.wifi->ssid : "offline";
    }

    m_device->push_system_data( values );
    m_last_strip_push = std::chrono::steady_clock::now();
}

/***************************/
/*   Ensure layer theme    */
/***************************/
void Mk20_Keymap::ensure_layer_theme( int layer_index ) {
    if (m_uploaded.contains( layer_index )) {
        return;
    }
    auto theme = build_theme( layer_index );
    auto data  = mpad::encode_theme( theme );
    LOG_INFO( "Mk20_Keymap: uploading layer ", layer_index,
              " theme (", data.size(), " bytes)" );
    // upload + activate=false — activation is a separate reload_theme
    // so first-visit and revisit paths share the same switch cost.
    if (!m_device->upload_theme( data, layer_path( layer_index ), false )) {
        LOG_ERROR( "Mk20_Keymap: upload failed for layer ", layer_index );
        return;
    }
    m_uploaded.insert( layer_index );
}

/***************************/
/*       Layer path        */
/***************************/
std::string Mk20_Keymap::layer_path( int layer_index ) const {
    const std::string name = "ovb-l" + std::to_string( layer_index );
    return mpad::Theme_File::device_path( name );
}

/***************************/
/*    Build blank theme    */
/***************************/
mpad::Theme_File Mk20_Keymap::build_blank_theme() const {
    nlohmann::json items = nlohmann::json::array();
    // One opaque tile across the whole canvas at high z — overwrites
    // every key cell and the strip, clearing art the firmware leaves
    // behind when a key's icon/label changes between themes.
    items.push_back( mpad::dynamic_image_item(
        "/image/ovb-blank/bg.png", "clr_all", 0, 0,
        mpad::CANVAS_W, mpad::CANVAS_H, 10 ) );
    mpad::Theme_File theme;
    theme.pages.push_back( mpad::make_page( items, "ovb_blank" ) );
    theme.assets.push_back(
        mpad::Theme_Asset{ "/image/ovb-blank/bg.png", key_icon() } );
    return theme;
}

/***************************/
/*   Ensure blank theme    */
/***************************/
void Mk20_Keymap::ensure_blank_theme() {
    if (m_blank_uploaded) {
        return;
    }
    auto data = mpad::encode_theme( build_blank_theme() );
    if (!m_device->upload_theme( data, blank_path(), false )) {
        LOG_ERROR( "Mk20_Keymap: blank theme upload failed" );
        return;
    }
    m_blank_uploaded = true;
}

/***************************/
/*       Blank path        */
/***************************/
std::string Mk20_Keymap::blank_path() const {
    return mpad::Theme_File::device_path( "ovb-blank" );
}

/***************************/
/*     Activate theme      */
/***************************/
void Mk20_Keymap::activate_theme( const std::string& device_path ) {
    ensure_blank_theme();
    // RELOAD takes ~0.5s to acknowledge on hardware — block on the
    // clear theme's reply so its render lands before the real theme
    // replaces it, then give the paint a moment to flush.
    m_device->call(
        static_cast<uint32_t>( mpad::Command_Id::Set_Device_Reload ),
        mpad::as_bytes( blank_path() ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    m_device->reload_theme( device_path );
}

/***************************/
/*      Build theme        */
/***************************/
mpad::Theme_File Mk20_Keymap::build_theme(
    int layer_index,
    const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const {
    // A screen's key table owns its labels entirely — when the active
    // panel has one, the theme comes from the screen (overlay applied)
    if (const auto* screen = active_screen()) {
        return build_screen_theme( *screen, overlay );
    }

    nlohmann::json items = nlohmann::json::array();

    // Per-layer asset dir — unique icon paths force repaints;
    // overlays share one scratch dir (see build_screen_theme)
    const std::string asset_dir = overlay ? "/image/ovb-overlay/"
        : "/image/ovb-l" + std::to_string( layer_index ) + '/';
    std::map<std::string, std::vector<std::byte>> image_assets;

    // Opaque backgrounds — clear remnants from the previous theme
    items.push_back( mpad::dynamic_image_item(
        asset_dir + "bg.png", "bg_grid", 0, mpad::KEY_GRID_Y,
        mpad::CANVAS_W, mpad::CANVAS_H - mpad::KEY_GRID_Y,
        -2, "main" ) );
    items.push_back( mpad::dynamic_image_item(
        asset_dir + "bg.png", "bg_strip", mpad::STRIP_X, 0,
        mpad::STRIP_W, mpad::STRIP_H, 1 ) );
    image_assets.emplace( asset_dir + "bg.png", key_icon() );

    // Grid keys — text actions report (row,col) serially. Bounded by
    // both the device geometry and the keys this layout defines.
    const int key_count = std::min( Mk20_Input::KEY_COUNT,
                                    m_layout.key_count() );
    for (int i = 0; i < key_count; ++i) {
        const int row = i / Mk20_Input::COLS;
        const int col = i % Mk20_Input::COLS;
        const std::string label = label_for( i, layer_index, nullptr, overlay );
        const std::string icon = asset_dir + "k" + std::to_string( i ) + ".png";
        image_assets.emplace( icon, bake_key_icon( label, "" ) );
        auto act = mpad::text_action(
            "ovb.k" + std::to_string( i ), label );
        items.push_back( mpad::key_item(
            row, col, act, icon,
            "k" + std::to_string( i ), "" ) );
    }

    // Encoder items — encoder_keyboard maps rotation + click to
    // HID F-keys (evdev on the host maps them to Input_Keys):
    //   L: CW=F13 click=F17 CCW=F14 | R: CW=F15 click=F18 CCW=F16
    items.push_back( mpad::encoder_item(
        mpad::ENCODER_LEFT_ROW,
        mpad::encoder_keyboard_action(
            104, "",   // CW  -> F13
            108, "",   // click -> F17
            105, "",   // CCW -> F14
            "Encoder L" ),
        ICON_PATH, "enc_l" ) );
    items.push_back( mpad::encoder_item(
        mpad::ENCODER_RIGHT_ROW,
        mpad::encoder_keyboard_action(
            106, "",   // CW  -> F15
            109, "",   // click -> F18
            107, "",   // CCW -> F16
            "Encoder R" ),
        ICON_PATH, "enc_r" ) );

    mpad::Theme_File theme;
    theme.pages.push_back( mpad::make_page( items,
        "ovb_layer_" + std::to_string( layer_index ) ) );
    theme.assets.push_back( mpad::Theme_Asset{ ICON_PATH, key_icon() } );
    for (auto& [asset_path, bytes] : image_assets) {
        theme.assets.push_back( mpad::Theme_Asset{ std::move( asset_path ),
                                                   std::move( bytes ) } );
    }
    return theme;
}

/***************************/
/*       Label for         */
/***************************/
std::string Mk20_Keymap::label_for(
    int key_index, int layer_index,
    const core::Screen* screen,
    const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const {
    // Overlay wins if it covers this key
    if (overlay) {
        for (const auto& k : *overlay) {
            if (k.key_index == key_index) {
                return k.label;
            }
        }
    }
    // Panel custom label next
    if (m_get_label_cb) {
        auto custom = m_get_label_cb( key_index );
        if (!custom.empty()) {
            return custom;
        }
    }
    // Screen label — unbound keys stay blank (dim)
    if (screen) {
        if (const auto* key = screen->key_at( key_index )) {
            return key->label;
        }
        return "";
    }
    // Layer label
    if (layer_index == m_layers.active_layer()) {
        return m_layers.label_at( key_index );
    }
    return "";
}

} // namespace ovb::hal::mk20
