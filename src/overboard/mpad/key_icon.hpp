/**
 * @file    key_icon.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Rasterize MK20 key-cap icons to PNG bytes at runtime.
 *
 * The firmware's title renderer is unreliable across theme switches
 * (stale bitmaps under repainted text), so key caps are baked as
 * opaque PNGs — glyph and/or caption text from the compiled
 * Montserrat faces composited over a solid background. Matches how
 * the Python reference builds vendor themes.
 */
#pragma once

// C++ Standard Libraries
#include <cstdint>
#include <string>
#include <vector>

namespace ovb::mpad {

/// Default key-cap background — dark slate, matches ovt_icon_render
inline constexpr uint32_t KEY_ICON_BG { 0x2B2B3B };

/// Default key-cap foreground
inline constexpr uint32_t KEY_ICON_FG { 0xffffff };

/**
 * @brief Render a key-cap icon to PNG bytes.
 *
 * @param label   Caption text (UTF-8; ASCII-safe). Alone: centered and
 *                auto-sized to fit. With `glyph`: small bottom caption.
 * @param glyph   Unicode codepoint for a large centered glyph
 *                (LVGL symbol range), or 0 for text-only.
 * @param size    Canvas edge in pixels (keys are 128x128).
 * @param bg_rgb  Background color (0xRRGGBB).
 * @param fg_rgb  Foreground color (0xRRGGBB).
 * @return PNG file bytes, or an empty vector on failure.
 */
std::vector<std::byte> render_key_icon( const std::string& label,
                                        char32_t glyph = 0,
                                        int size = 128,
                                        uint32_t bg_rgb = KEY_ICON_BG,
                                        uint32_t fg_rgb = KEY_ICON_FG );

} // namespace ovb::mpad
