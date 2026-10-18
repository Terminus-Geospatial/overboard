/**
 * @file    main.cpp
 * @author  Marvin Smith
 * @date    2026-10-21
 *
 * @brief   Render an LVGL symbol glyph (+ optional caption) to a PNG
 *          suitable for embedding as an MK20 key icon.
 *
 * The actual rasterization lives in mpad/key_icon — shared with the
 * runtime key-cap baking in Mk20_Keymap.
 */
#include "config.hpp"

// C++ Standard Libraries
#include <fstream>
#include <iostream>

// Project Libraries
#include <overboard/mpad/key_icon.hpp>

namespace ovb::tools::icon_render {
namespace {

int run(const Config& cfg) {
    auto png = mpad::render_key_icon( cfg.label(),
                                      static_cast<char32_t>( cfg.codepoint() ),
                                      cfg.size(), cfg.bg_rgb(), cfg.fg_rgb() );
    if (png.empty()) {
        std::cerr << "icon render failed\n";
        return 1;
    }

    std::ofstream out( cfg.output_path(), std::ios::binary );
    if (!out) {
        std::cerr << "failed to open " << cfg.output_path() << "\n";
        return 1;
    }
    out.write( reinterpret_cast<const char*>( png.data() ),
               static_cast<std::streamsize>( png.size() ) );
    out.close();
    std::cout << "wrote " << cfg.output_path() << " (" << cfg.size() << "x"
              << cfg.size() << ")\n";
    return 0;
}

} // anonymous namespace
} // namespace ovb::tools::icon_render

/***************************/
/*           Main          */
/***************************/
int main(int argc, char* argv[]) {
    const auto cfg = ovb::tools::icon_render::Config::parse(argc, argv);
    if (!cfg) {
        ovb::tools::icon_render::Config::print_usage(argv[0]);
        return 1;
    }
    if (cfg->help_requested()) {
        ovb::tools::icon_render::Config::print_usage(argv[0]);
        return 0;
    }
    return ovb::tools::icon_render::run(*cfg);
}
