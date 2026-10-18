/**
 * @file    key_icon.cpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Runtime key-cap PNG rasterizer — see key_icon.hpp.
 */
#include <overboard/mpad/key_icon.hpp>

// Third-Party Libraries
#include <lvgl/lvgl.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::mpad {

namespace {

/***************************/
/*   UTF-8 codepoint next  */
/***************************/
uint32_t utf8_next( const char*& p ) {
    const auto* s = reinterpret_cast<const unsigned char*>( p );
    uint32_t cp = *s;
    if ((cp & 0xE0) == 0xC0) {
        cp = ((cp & 0x1F) << 6) | (static_cast<uint32_t>( s[1] ) & 0x3F);
        s += 2;
    } else if ((cp & 0xF0) == 0xE0) {
        cp = ((cp & 0x0F) << 12) | ((static_cast<uint32_t>( s[1] ) & 0x3F) << 6)
           | (static_cast<uint32_t>( s[2] ) & 0x3F);
        s += 3;
    } else if ((cp & 0xF8) == 0xF0) {
        cp = ((cp & 0x07) << 18) | ((static_cast<uint32_t>( s[1] ) & 0x3F) << 12)
           | ((static_cast<uint32_t>( s[2] ) & 0x3F) << 6)
           | (static_cast<uint32_t>( s[3] ) & 0x3F);
        s += 4;
    } else {
        ++s;
    }
    p = reinterpret_cast<const char*>( s );
    return cp;
}

/***************************/
/*      Glyph bitmap       */
/***************************/
const lv_draw_buf_t* glyph_bitmap( const lv_font_t* font, uint32_t cp,
                                   lv_font_glyph_dsc_t& dsc,
                                   lv_draw_buf_t*& buf ) {
    if (!lv_font_get_glyph_dsc( font, &dsc, cp, 0 ) || dsc.box_w == 0) {
        return nullptr;
    }
    const uint32_t stride = lv_draw_buf_width_to_stride( dsc.box_w,
                                                         LV_COLOR_FORMAT_A8 );
    buf = lv_draw_buf_create( dsc.box_w, dsc.box_h, LV_COLOR_FORMAT_A8, stride );
    if (!buf) {
        return nullptr;
    }
    return static_cast<const lv_draw_buf_t*>(
        lv_font_get_glyph_bitmap( &dsc, buf ) );
}

/***************************/
/*       Blit glyph        */
/***************************/
void blit_glyph( std::vector<uint8_t>& px, int canvas_w, int canvas_h,
                 const lv_font_glyph_dsc_t& dsc, const uint8_t* alpha,
                 int stride, int x0, int baseline_y,
                 uint8_t fr, uint8_t fg, uint8_t fb ) {
    const int top = baseline_y - dsc.ofs_y - static_cast<int>( dsc.box_h );
    for (int y = 0; y < static_cast<int>( dsc.box_h ); ++y) {
        const int dy = top + y;
        if (dy < 0 || dy >= canvas_h) {
            continue;
        }
        for (int x = 0; x < static_cast<int>( dsc.box_w ); ++x) {
            const int dx = x0 + static_cast<int>( dsc.ofs_x ) + x;
            if (dx < 0 || dx >= canvas_w) {
                continue;
            }
            const uint8_t a = alpha[static_cast<size_t>( y ) * static_cast<size_t>( stride )
                                  + static_cast<size_t>( x )];
            if (a == 0) {
                continue;
            }
            auto* p = &px[(static_cast<size_t>( dy ) * static_cast<size_t>( canvas_w )
                         + static_cast<size_t>( dx )) * 4];
            const uint8_t ia = 255 - a;
            p[0] = static_cast<uint8_t>( (fr * a + p[0] * ia) / 255 );
            p[1] = static_cast<uint8_t>( (fg * a + p[1] * ia) / 255 );
            p[2] = static_cast<uint8_t>( (fb * a + p[2] * ia) / 255 );
        }
    }
}

/***************************/
/*    Measure text width   */
/***************************/
int measure_text( const lv_font_t* font, const std::string& text ) {
    int w = 0;
    const char* p = text.c_str();
    while (*p) {
        lv_font_glyph_dsc_t dsc{};
        if (lv_font_get_glyph_dsc( font, &dsc, utf8_next( p ), 0 )) {
            w += static_cast<int>( dsc.adv_w );
        }
    }
    return w;
}

/***************************/
/*        Draw text        */
/***************************/
void draw_text( std::vector<uint8_t>& px, int size,
                const lv_font_t* font, const std::string& text,
                int baseline_y, uint8_t fr, uint8_t fg, uint8_t fb ) {
    int pen_x = (size - measure_text( font, text )) / 2;
    const char* p = text.c_str();
    while (*p) {
        lv_font_glyph_dsc_t dsc{};
        lv_draw_buf_t* buf = nullptr;
        const lv_draw_buf_t* g = glyph_bitmap( font, utf8_next( p ), dsc, buf );
        if (g) {
            blit_glyph( px, size, size, dsc,
                        static_cast<const uint8_t*>( g->data ),
                        static_cast<int>( lv_draw_buf_width_to_stride(
                            dsc.box_w, LV_COLOR_FORMAT_A8 ) ),
                        pen_x, baseline_y, fr, fg, fb );
            lv_font_glyph_release_draw_data( &dsc );
            lv_draw_buf_destroy( buf );
        }
        pen_x += static_cast<int>( dsc.adv_w );
    }
}

/***************************/
/*   Encode PNG to memory  */
/***************************/
void stb_collect( void* ctx, void* data, int len ) {
    auto& out = *static_cast<std::vector<std::byte>*>( ctx );
    const auto* b = static_cast<const std::byte*>( data );
    out.insert( out.end(), b, b + len );
}

} // anonymous namespace

/***************************/
/*     Render key icon     */
/***************************/
std::vector<std::byte> render_key_icon( const std::string& label,
                                        char32_t glyph, int size,
                                        uint32_t bg_rgb, uint32_t fg_rgb ) {
    // LVGL must be initialized before draw-buf/font calls — the app
    // already does this via its display; harmless to repeat.
    lv_init();

    const uint8_t br = (bg_rgb >> 16) & 0xFF;
    const uint8_t bg = (bg_rgb >> 8) & 0xFF;
    const uint8_t bb = bg_rgb & 0xFF;
    const uint8_t fr = (fg_rgb >> 16) & 0xFF;
    const uint8_t fg = (fg_rgb >> 8) & 0xFF;
    const uint8_t fb = fg_rgb & 0xFF;
    const auto edge = static_cast<size_t>( size );
    std::vector<uint8_t> px( edge * edge * 4 );
    for (size_t i = 0; i + 3 < px.size(); i += 4) {
        px[i] = br; px[i + 1] = bg; px[i + 2] = bb; px[i + 3] = 0xFF;
    }

    // --- Large glyph (optional) --------------------------------------
    // Montserrat 48 carries the FontAwesome symbol range. With a
    // caption the glyph centers at 2/5 height; otherwise dead center.
    if (glyph != 0) {
        lv_font_glyph_dsc_t dsc{};
        lv_draw_buf_t* buf = nullptr;
        if (const lv_draw_buf_t* g = glyph_bitmap( &lv_font_montserrat_48,
                                                   glyph, dsc, buf )) {
            const int icon_cy = label.empty() ? size / 2 : size * 2 / 5;
            const int gx = size / 2 - static_cast<int>( dsc.adv_w ) / 2;
            const int baseline = icon_cy + static_cast<int>( dsc.box_h ) / 2
                               + dsc.ofs_y;
            blit_glyph( px, size, size, dsc,
                        static_cast<const uint8_t*>( g->data ),
                        static_cast<int>( lv_draw_buf_width_to_stride(
                            dsc.box_w, LV_COLOR_FORMAT_A8 ) ),
                        gx, baseline, fr, fg, fb );
            lv_font_glyph_release_draw_data( &dsc );
            lv_draw_buf_destroy( buf );
        } else {
            LOG_WARN( "mpad::render_key_icon: glyph 0x",
                      std::to_string( glyph ), " not in font" );
        }
    }

    // --- Text label ---------------------------------------------------
    if (!label.empty()) {
        if (glyph != 0) {
            // Caption band near the bottom edge
            draw_text( px, size, &lv_font_montserrat_14, label,
                       size - size / 8, fr, fg, fb );
        } else {
            // Text-only key: largest compiled face that fits
            const int margin = size / 8;
            const lv_font_t* fonts[] = { &lv_font_montserrat_48,
                                         &lv_font_montserrat_14,
                                         &lv_font_montserrat_12,
                                         &lv_font_montserrat_10 };
            const lv_font_t* font = fonts[0];
            for (const lv_font_t* f : fonts) {
                if (measure_text( f, label ) <= size - 2 * margin) {
                    font = f;
                    break;
                }
            }
            draw_text( px, size, font, label,
                       size / 2 + static_cast<int>( font->line_height ) / 2,
                       fr, fg, fb );
        }
    }

    std::vector<std::byte> out;
    if (!stbi_write_png_to_func( stb_collect, &out, size, size, 4,
                                 px.data(), size * 4 )) {
        return {};
    }
    return out;
}

} // namespace ovb::mpad
