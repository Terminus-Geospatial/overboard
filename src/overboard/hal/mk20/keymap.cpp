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

// Project Libraries
#include <overboard/hal/mk20/input.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/mpad/theme.hpp>

namespace ovb::hal::mk20 {

namespace {

/// Dark-slate 128x128 solid PNG — shared icon for all keys.
/// (Rendered text via `title`/`titleParam` sits on top.)
const std::vector<std::byte>& key_icon() {
    static const std::vector<std::byte> icon = {
        std::byte{0x89},std::byte{0x50},std::byte{0x4e},std::byte{0x47},std::byte{0x0d},std::byte{0x0a},std::byte{0x1a},std::byte{0x0a},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x0d},std::byte{0x49},std::byte{0x48},std::byte{0x44},std::byte{0x52},
        std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x80},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x80},std::byte{0x08},std::byte{0x02},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x4c},std::byte{0x5c},std::byte{0xf6},
        std::byte{0x9c},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0xf9},std::byte{0x49},std::byte{0x44},std::byte{0x41},std::byte{0x54},std::byte{0x78},std::byte{0x9c},std::byte{0xed},std::byte{0xd1},std::byte{0x41},std::byte{0x09},std::byte{0x00},
        std::byte{0x30},std::byte{0x0c},std::byte{0xc0},std::byte{0xc0},std::byte{0x4a},std::byte{0x1a},std::byte{0x7b},std::byte{0xce},std::byte{0xbf},std::byte{0xb0},std::byte{0xc9},std::byte{0xe8},std::byte{0x23},std::byte{0x07},std::byte{0x11},std::byte{0x10},
        std::byte{0xb8},std::byte{0x39},std::byte{0xf7},std::byte{0x69},std::byte{0xb1},std::byte{0x59},std::byte{0x3f},std::byte{0x88},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},
        std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},
        std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},
        std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},
        std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},
        std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},
        std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},
        std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},
        std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},
        std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},
        std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},
        std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},
        std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},
        std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x00},std::byte{0x00},std::byte{0x68},
        std::byte{0x07},std::byte{0x00},std::byte{0x40},std::byte{0x3b},std::byte{0x00},std::byte{0x00},std::byte{0xda},std::byte{0x01},std::byte{0x00},std::byte{0xd0},std::byte{0x0e},std::byte{0x00},std::byte{0x80},std::byte{0x76},std::byte{0x00},std::byte{0x00},
        std::byte{0xb4},std::byte{0x03},std::byte{0x00},std::byte{0xa0},std::byte{0x1d},std::byte{0x00},std::byte{0x00},std::byte{0xed},std::byte{0x3e},std::byte{0x3f},std::byte{0x86},std::byte{0x02},std::byte{0x4a},std::byte{0xf0},std::byte{0x82},std::byte{0x78},
        std::byte{0x42},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x00},std::byte{0x49},std::byte{0x45},std::byte{0x4e},std::byte{0x44},std::byte{0xae},std::byte{0x42},std::byte{0x60},std::byte{0x82},
    };
    return icon;
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
                          std::function<std::string(int)> get_label_cb,
                          std::string                     device_root )
    : m_device( std::move( device ) ),
      m_layout( layout ),
      m_layers( layers ),
      m_get_label_cb( std::move( get_label_cb ) ),
      m_device_root( std::move( device_root ) ) {
}

/***************************/
/*      Update layer       */
/***************************/
void Mk20_Keymap::update_layer() {
    if (!m_overlays.empty()) {
        return;
    }
    const int idx = m_layers.active_layer();
    ensure_layer_theme( idx );
    m_device->reload_theme( layer_path( idx ) );
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
    m_device->upload_theme( data, path, true );
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
        m_device->reload_theme( layer_path( m_layers.active_layer() ) );
    } else {
        // Re-render the new top overlay
        auto theme = build_theme( m_layers.active_layer(),
                                  &m_overlays.back().keys );
        auto data  = mpad::encode_theme( theme );
        m_device->upload_theme( data, m_device_root + "/ovb-overlay/ovb-overlay.Theme", true );
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
/*      Build theme        */
/***************************/
mpad::Theme_File Mk20_Keymap::build_theme(
    int layer_index,
    const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const {
    nlohmann::json items = nlohmann::json::array();

    // Grid keys — text actions report (row,col) serially. Bounded by
    // both the device geometry and the keys this layout defines.
    const int key_count = std::min( Mk20_Input::KEY_COUNT,
                                    m_layout.key_count() );
    for (int i = 0; i < key_count; ++i) {
        const int row = i / Mk20_Input::COLS;
        const int col = i % Mk20_Input::COLS;
        const std::string label = label_for( i, layer_index, overlay );
        auto act = mpad::text_action(
            "ovb.k" + std::to_string( i ), label );
        items.push_back( mpad::key_item(
            row, col, act, ICON_PATH,
            "k" + std::to_string( i ), label ) );
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
    return theme;
}

/***************************/
/*       Label for         */
/***************************/
std::string Mk20_Keymap::label_for(
    int key_index, int layer_index,
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
    // Layer label
    if (layer_index == m_layers.active_layer()) {
        return m_layers.label_at( key_index );
    }
    return "";
}

} // namespace ovb::hal::mk20
