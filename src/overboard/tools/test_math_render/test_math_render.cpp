/**
 * @file    test_math_render.cpp
 * @author  Marvin
 * @date    2026-05-25
 *
 * @brief   Tool to test math rendering to PNG
 *
 * This tool renders math expressions using the layout engine and outputs
 * PNG files for testing and verification.
 */

// C++ Standard Libraries
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/tools/test_math_render/config.hpp>
#include <overboard/hal/font_5x7.hpp>
#include <overboard/log/stdout_logger.hpp>
#include <overboard/math/layout/engine.hpp>
#include <overboard/font/font_metrics.hpp>
#include <overboard/math/parser.hpp>

// Third-Party Libraries
#include <png.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

using namespace ovb;

// Global font info
static stbtt_fontinfo g_font;
static std::vector<unsigned char> g_font_data;

/**
 * @brief Load font file
 */
bool load_font(const std::filesystem::path& font_path) {
    std::ifstream file(font_path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open font file: " << font_path << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    g_font_data.resize(size);
    if (!file.read(reinterpret_cast<char*>(g_font_data.data()), size)) {
        std::cerr << "Failed to read font file" << std::endl;
        return false;
    }

    int offset = stbtt_GetFontOffsetForIndex(g_font_data.data(), 0);
    if (!stbtt_InitFont(&g_font, g_font_data.data(), offset)) {
        std::cerr << "Failed to initialize font" << std::endl;
        return false;
    }

    return true;
}

/**
 * @brief Build font metrics from the loaded font at a given base pixel size
 */
font::Font_Metrics build_font_metrics(float base_px) {
    font::Font_Metrics m;
    m.base_px = base_px;

    float scale = stbtt_ScaleForPixelHeight(&g_font, base_px);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&g_font, &ascent, &descent, &line_gap);
    m.ascent   = static_cast<int>(ascent  *  scale);
    m.descent  = static_cast<int>(-descent * scale);  // descent is negative in stb
    m.line_gap = static_cast<int>(line_gap * scale);

    // Measure 'M' for em width
    int glyph_m = stbtt_FindGlyphIndex(&g_font, 'M');
    int adv_m, lsb_m;
    stbtt_GetGlyphHMetrics(&g_font, glyph_m, &adv_m, &lsb_m);
    m.em = static_cast<int>(adv_m * scale);

    // Measure 'x' bounding box for ex height
    int x0, y0, x1, y1;
    int glyph_x = stbtt_FindGlyphIndex(&g_font, 'x');
    stbtt_GetGlyphBitmapBox(&g_font, glyph_x, scale, scale, &x0, &y0, &x1, &y1);
    m.ex = y1 - y0;

    // Pre-measure advance widths for all printable ASCII (one-time at startup)
    for (int cp = font::Font_Metrics::ASCII_FIRST; cp <= font::Font_Metrics::ASCII_LAST; ++cp) {
        int glyph = stbtt_FindGlyphIndex(&g_font, cp);
        int adv, lsb;
        stbtt_GetGlyphHMetrics(&g_font, glyph, &adv, &lsb);
        m.advances[static_cast<size_t>(cp - font::Font_Metrics::ASCII_FIRST)] = static_cast<int>(adv * scale);
    }

    return m;
}

/**
 * @brief Render text using stb_truetype
 */
void render_text(const std::string& text, std::vector<uint8_t>& img, int img_width, int img_height, int x, int y, float font_size) {
    float scale = stbtt_ScaleForPixelHeight(&g_font, font_size);
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&g_font, &ascent, &descent, &line_gap);

    int baseline = y + static_cast<int>(ascent * scale);

    for (char c : text) {
        int glyph_index = stbtt_FindGlyphIndex(&g_font, c);

        int ix0, iy0, ix1, iy1;
        stbtt_GetGlyphBitmapBox(&g_font, glyph_index, scale, scale, &ix0, &iy0, &ix1, &iy1);

        int gw = ix1 - ix0;
        int gh = iy1 - iy0;

        std::vector<unsigned char> bitmap(gw * gh);
        stbtt_MakeGlyphBitmap(&g_font, bitmap.data(), gw, gh, gw, scale, scale, glyph_index);

        // Blit bitmap to image
        for (int py = 0; py < gh; ++py) {
            for (int px = 0; px < gw; ++px) {
                int img_x = x + ix0 + px;
                int img_y = baseline + iy0 + py;

                if (img_x >= 0 && img_x < img_width && img_y >= 0 && img_y < img_height) {
                    size_t idx = (static_cast<size_t>(img_y) * static_cast<size_t>(img_width) + static_cast<size_t>(img_x)) * 4;
                    unsigned char alpha = bitmap[py * gw + px];
                    img[idx + 0] = 0;    // R (black)
                    img[idx + 1] = 0;    // G (black)
                    img[idx + 2] = 0;    // B (black)
                    img[idx + 3] = alpha; // A
                }
            }
        }

        int advance, lsb;
        stbtt_GetGlyphHMetrics(&g_font, glyph_index, &advance, &lsb);
        x += static_cast<int>(advance * scale);
    }
}

/**
 * @brief Render text using 5x7 bitmap font
 */
void render_text_5x7(const std::string& text, std::vector<uint8_t>& img, int img_width, int img_height, int x, int y, int scale) {
    int char_width = 5 * scale;
    int char_height = 7 * scale;

    for (size_t i = 0; i < text.size(); ++i) {
        uint32_t codepoint;
        size_t pos = i;
        codepoint = hal::utf8_decode(text, pos);
        i = pos - 1;  // Update loop index

        const auto* glyph = hal::get_glyph(codepoint);
        if (!glyph) continue;

        // Draw 5x7 glyph scaled
        for (int row = 0; row < 7; ++row) {
            uint8_t glyph_row = (*glyph)[row];
            for (int col = 0; col < 5; ++col) {
                if (glyph_row & (0x10 >> col)) {  // MSB is leftmost
                    // Draw scaled pixel
                    for (int sy = 0; sy < scale; ++sy) {
                        for (int sx = 0; sx < scale; ++sx) {
                            int px = x + (static_cast<int>(i) * char_width) + (col * scale) + sx;
                            int py = y + (row * scale) + sy;
                            if (px >= 0 && px < img_width && py >= 0 && py < img_height) {
                                size_t idx = (static_cast<size_t>(py) * static_cast<size_t>(img_width) + static_cast<size_t>(px)) * 4;
                                img[idx + 0] = 0;    // R (black)
                                img[idx + 1] = 0;    // G (black)
                                img[idx + 2] = 0;    // B (black)
                                img[idx + 3] = 255;  // A
                            }
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief Recursively draw layout boxes to image buffer
 */
void draw_box_recursive( const math::layout::Layout_Box& box,
                         const font::Font_Metrics& metrics,
                         std::vector<uint8_t>& img,
                         int img_width,
                         int img_height,
                         bool use_5x7,
                         bool debug_boxes = false,
                         int depth = 0,
                         float scale = 1.0f ) {
    // Debug: print box info
    if (debug_boxes) {
        printf("[DEBUG] depth=%d, kind=%d, pos=(%d,%d), size=(%d,%d), baseline=%d, text='%s', children=%zu\n",
               depth, static_cast<int>(box.kind), box.position().x, box.position().y,
               box.width(), box.height(), box.baseline, box.text.c_str(), box.children.size());
    }

    // If this is an ATOM box with text, render the text
    if (box.kind == math::layout::Box_Kind::ATOM && !box.text.empty()) {
        if (use_5x7) {
            // 5x7 bitmap font rendering
            int char_width = 5 * static_cast<int>(box.scale);
            int total_width = static_cast<int>(box.text.size()) * char_width;
            int offset_x = box.position().x + (box.width() - total_width) / 2;
            int offset_y = box.position().y;
            render_text_5x7(box.text, img, img_width, img_height, offset_x, offset_y, static_cast<int>(box.scale));
        } else {
            // TTF font rendering
            float font_size = metrics.base_px * box.scale;
            float scale = stbtt_ScaleForPixelHeight(&g_font, font_size);

            // Measure total advance width of the text
            int total_width = 0;
            for (char c : box.text) {
                int glyph_index = stbtt_FindGlyphIndex(&g_font, c);
                int advance, lsb;
                stbtt_GetGlyphHMetrics(&g_font, glyph_index, &advance, &lsb);
                total_width += static_cast<int>(advance * scale);
            }

            // Center text horizontally in box
            int offset_x = box.position().x + (box.width() - total_width) / 2;

            // Place baseline at box.position().y + box.baseline
            // render_text(x, y) uses y as the top of the line and adds ascent internally
            // So: render_text y = baseline_y - (ascent * scale)
            // Since box.baseline == metrics.ascent * box.scale, those cancel cleanly
            int baseline_y = box.position().y + box.baseline;
            int ascent_raw, descent_raw, line_gap_raw;
            stbtt_GetFontVMetrics(&g_font, &ascent_raw, &descent_raw, &line_gap_raw);
            int offset_y = baseline_y - static_cast<int>(ascent_raw * scale);

            render_text(box.text, img, img_width, img_height, offset_x, offset_y, font_size);
        }
    }

    // If this is a FRACTION box, draw the horizontal divider bar
    if (box.kind == math::layout::Box_Kind::FRACTION && box.children.size() == 2) {
        const int pad = 2;
        const auto& num = box.children[0];
        int bar_y     = box.position().y + num.size.y + pad;
        int bar_thick = std::max(1, static_cast<int>(box.scale));
        int bar_x0    = box.position().x;
        int bar_x1    = box.position().x + box.width();

        for (int y = bar_y; y < bar_y + bar_thick; ++y) {
            for (int x = bar_x0; x < bar_x1; ++x) {
                if (x >= 0 && x < img_width && y >= 0 && y < img_height) {
                    size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(img_width) + static_cast<size_t>(x)) * 4;
                    img[idx + 0] = 0;
                    img[idx + 1] = 0;
                    img[idx + 2] = 0;
                    img[idx + 3] = 255;
                }
            }
        }
    }

    // If this is a SQRT box, draw the √ symbol and horizontal bar
    if (box.kind == math::layout::Box_Kind::SQRT) {
        int symbol_width = 2 * static_cast<int>(box.scale);  // Match layout engine reservation
        int top_pad = 2 * static_cast<int>(box.scale);      // Match layout engine top padding

        // Draw √ symbol manually with lines (using Bresenham-like approach)
        // Standard √ shape: vertical tick, diagonal, horizontal top bar
        int tick_x = box.position().x + 2 * static_cast<int>(box.scale);  // Moved right
        int tick_bottom = box.children[0].position().y + box.children[0].height();  // Go to base of variable (absolute position)
        int tick_top = box.position().y + top_pad;  // Start at top bar
        int diag_end_x = box.position().x + symbol_width + 1 * static_cast<int>(box.scale);  // Extend diagonal slightly
        int diag_end_y = box.position().y + top_pad;

        // Helper to draw a pixel
        auto draw_pixel = [&](int x, int y) {
            if (x >= 0 && x < img_width && y >= 0 && y < img_height) {
                size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(img_width) + static_cast<size_t>(x)) * 4;
                img[idx + 0] = 0;    // R (black)
                img[idx + 1] = 0;    // G (black)
                img[idx + 2] = 0;    // B (black)
                img[idx + 3] = 255;  // A
            }
        };

        // Draw line helper
        auto draw_line = [&](int x_start, int y_start, int x_end, int y_end) {
            int dx = abs(x_end - x_start);
            int dy = abs(y_end - y_start);
            int sx = x_start < x_end ? 1 : -1;
            int sy = y_start < y_end ? 1 : -1;
            int err = dx - dy;

            while (true) {
                draw_pixel(x_start, y_start);
                if (x_start == x_end && y_start == y_end) break;
                int e2 = 2 * err;
                if (e2 > -dy) { err -= dy; x_start += sx; }
                if (e2 < dx) { err += dx; y_start += sy; }
            }
        };

        // Draw vertical tick mark from top bar to base
        draw_line(tick_x, tick_bottom, tick_x, tick_top);

        // Draw diagonal line from top of tick to end
        draw_line(tick_x, tick_top, diag_end_x, diag_end_y);

        // Draw horizontal line from child position to end of box
        int line_y = box.position().y + top_pad;  // Top of the box + padding
        int line_start = box.children[0].position().x;  // Start at child position (where reserved space ends)
        int line_end = box.position().x + box.width() + 2;  // Extend past the argument

        for (int x = line_start; x < line_end; ++x) {
            draw_pixel(x, line_y);
        }

        // Draw tick in lower-left corner (up-2, left-1 for ~4 pixels)
        int tick_len = 4 * static_cast<int>(box.scale);
        int tick_start_x = tick_x;
        int tick_start_y = tick_bottom;
        int tick_end_x = tick_x - tick_len / 2;  // left-1 slope
        int tick_end_y = tick_bottom - tick_len;  // up-2 slope
        draw_line(tick_start_x, tick_start_y, tick_end_x, tick_end_y);
    }

    // Recursively draw children (they have their own absolute positions)
    for (const auto& child : box.children) {
        draw_box_recursive(child, metrics, img, img_width, img_height, use_5x7, debug_boxes, depth + 1, scale);
    }

    // Draw debug box if enabled (draw after children so it's on top)
    if (debug_boxes) {
        // Array of colors for different depths
        static const uint8_t colors[][3] = {
            {255, 0, 0},      // Red
            {0, 255, 0},      // Green
            {0, 0, 255},      // Blue
            {255, 255, 0},    // Yellow
            {255, 0, 255},    // Magenta
            {0, 255, 255},    // Cyan
        };
        int color_idx = depth % (sizeof(colors) / sizeof(colors[0]));
        const uint8_t* color = colors[color_idx];

        // Draw outline only (top, bottom, left, right)
        for (int x = 0; x < box.width(); ++x) {
            int img_x = box.position().x + x;
            int img_y_top = box.position().y;
            int img_y_bottom = box.position().y + box.height() - 1;

            if (img_x >= 0 && img_x < img_width) {
                if (img_y_top >= 0 && img_y_top < img_height) {
                    size_t idx = (static_cast<size_t>(img_y_top) * static_cast<size_t>(img_width) + static_cast<size_t>(img_x)) * 4;
                    img[idx + 0] = color[0];  // R
                    img[idx + 1] = color[1];  // G
                    img[idx + 2] = color[2];  // B
                    img[idx + 3] = 255;      // A
                }
                if (img_y_bottom >= 0 && img_y_bottom < img_height) {
                    size_t idx = (static_cast<size_t>(img_y_bottom) * static_cast<size_t>(img_width) + static_cast<size_t>(img_x)) * 4;
                    img[idx + 0] = color[0];  // R
                    img[idx + 1] = color[1];  // G
                    img[idx + 2] = color[2];  // B
                    img[idx + 3] = 255;      // A
                }
            }
        }

        for (int y = 0; y < box.height(); ++y) {
            int img_y = box.position().y + y;
            int img_x_left = box.position().x;
            int img_x_right = box.position().x + box.width() - 1;

            if (img_y >= 0 && img_y < img_height) {
                if (img_x_left >= 0 && img_x_left < img_width) {
                    size_t idx = (static_cast<size_t>(img_y) * static_cast<size_t>(img_width) + static_cast<size_t>(img_x_left)) * 4;
                    img[idx + 0] = color[0];  // R
                    img[idx + 1] = color[1];  // G
                    img[idx + 2] = color[2];  // B
                    img[idx + 3] = 255;      // A
                }
                if (img_x_right >= 0 && img_x_right < img_width) {
                    size_t idx = (static_cast<size_t>(img_y) * static_cast<size_t>(img_width) + static_cast<size_t>(img_x_right)) * 4;
                    img[idx + 0] = color[0];  // R
                    img[idx + 1] = color[1];  // G
                    img[idx + 2] = color[2];  // B
                    img[idx + 3] = 255;      // A
                }
            }
        }
    }
}

/**
 * @brief Render layout box to PNG
 */
void render_box_to_png( const math::layout::Layout_Box& box,
                        const font::Font_Metrics& metrics,
                        const std::filesystem::path& output_path,
                        int width  = 60,
                        int height = 60,
                        bool debug_boxes = false,
                        bool use_5x7 = false,
                        float scale = 1.0f) {

    // Create image buffer (RGBA for transparency)
    std::vector<uint8_t> img( static_cast<size_t>(width) * static_cast<size_t>(height) * 4, 0 ); // Transparent background

    // Simple rendering: draw text at measured positions
    // This is a placeholder - actual rendering would use a font library
    // For now, we'll draw the text content as colored pixels

    // Draw border
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * 4;
            if (x == 0 || x == width - 1 || y == 0 || y == height - 1) {
                img[idx + 0] = 100;  // R
                img[idx + 1] = 100;  // G
                img[idx + 2] = 100;  // B
                img[idx + 3] = 255;  // A
            }
        }
    }

    // Recursively draw all boxes (positions are absolute from prepare())
    draw_box_recursive(box, metrics, img, width, height, use_5x7, debug_boxes, 0, scale);

    // Write PNG using libpng
    FILE* fp = fopen(output_path.c_str(), "wb");
    if (!fp) {
        throw std::runtime_error("Failed to open file for writing: " + output_path.string());
    }

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        fclose(fp);
        throw std::runtime_error("Failed to create PNG write struct");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, nullptr);
        fclose(fp);
        throw std::runtime_error("Failed to create PNG info struct");
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        throw std::runtime_error("PNG write error");
    }

    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, static_cast<png_uint_32>(width), static_cast<png_uint_32>(height), 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    std::vector<png_bytep> row_pointers(static_cast<size_t>(height));
    for (int y = 0; y < height; ++y) {
        row_pointers[static_cast<size_t>(y)] = img.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * 4;
    }

    png_set_rows(png_ptr, info_ptr, row_pointers.data());
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

/**
 * @brief Render a math expression to PNG
 */
void render_expression( const std::string&           expr,
                        const std::filesystem::path& output_path,
                        int                          width  = 60,
                        int                          height = 60,
                        bool                         debug_boxes = false,
                        float                        scale = 1.0f,
                        bool                         use_5x7 = false ) {
    LOG_INFO("Rendering expression: " + expr);

    // Build font metrics
    font::Font_Metrics metrics;
    if (use_5x7) {
        metrics = font::Font_Metrics::make_5x7();
        if (!metrics.scale(scale)) {
            LOG_ERROR("Failed to scale font metrics");
            return;
        }
    } else {
        float base_px = 10.0f * scale;
        metrics = build_font_metrics(base_px);
    }

    // Parse the expression into an AST
    math::Parser parser(expr);
    math::ast::Node::ptr_t ast = parser.parse();

    if (!ast) {
        LOG_ERROR("Failed to parse expression");
        return;
    }

    // Build layout from AST using real font metrics
    math::layout::Layout_Engine engine(metrics, scale);
    math::layout::Layout_Box box = engine.build(ast.get());
    engine.prepare(box, core::Point<int>(width, height));

    // Render to PNG
    render_box_to_png(box, metrics, output_path, width, height, debug_boxes, use_5x7, scale);

    LOG_INFO("Saved to: " + output_path.string());
}

/**
 * @brief Main function
 */
int main(int argc, char* argv[], char* envp[]) {
    // Parse configuration
    ovb::Config config = ovb::Config::parse_command_line(argc, argv, envp);

    // Configure logging
    config.configure_logging();

    // Log configuration
    LOG_INFO(config.to_string());

    try {
        // Load font if not using 5x7
        if (!config.use_5x7()) {
            if (!load_font(config.font_path())) {
                LOG_ERROR("Failed to load font: " + config.font_path().string());
                return 1;
            }
        }

        // Render expression
        render_expression( config.expression(),
                           config.output_path(),
                           config.width(),
                           config.height(),
                           config.debug_boxes(),
                           config.scale(),
                           config.use_5x7() );
        return 0;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Error: ") + e.what());
        return 1;
    }
}
