/**
 * @file    config.cpp
 * @author  Marvin Smith
 * @date    2026-10-21
 *
 * @brief   Configuration for ovt_icon_render application
 */
#include "config.hpp"

// C++ Standard Libraries
#include <algorithm>
#include <charconv>
#include <iostream>
#include <unordered_map>
#include <vector>

// Third-Party Libraries
#include <lvgl/lvgl.h>

namespace ovb::tools::icon_render {

namespace {

/// Decode the first UTF-8 codepoint from an LVGL symbol string
uint32_t utf8_first_codepoint(const char* s) {
    const auto* p = reinterpret_cast<const unsigned char*>(s);
    if (p[0] < 0x80) return p[0];
    if ((p[0] & 0xE0) == 0xC0) return ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
    if ((p[0] & 0xF0) == 0xE0) return ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    return 0;
}

/// Named LVGL symbol glyphs. Extend as needed — keep alphabetical.
const std::unordered_map<std::string, const char*> SYMBOLS = {
    { "audio",      LV_SYMBOL_AUDIO      },
    { "backspace",  LV_SYMBOL_BACKSPACE  },
    { "battery",    LV_SYMBOL_BATTERY_FULL },
    { "bell",       LV_SYMBOL_BELL       },
    { "bluetooth",  LV_SYMBOL_BLUETOOTH  },
    { "bullet",     LV_SYMBOL_BULLET     },
    { "call",       LV_SYMBOL_CALL       },
    { "close",      LV_SYMBOL_CLOSE      },
    { "copy",       LV_SYMBOL_COPY       },
    { "cut",        LV_SYMBOL_CUT        },
    { "directory",  LV_SYMBOL_DIRECTORY  },
    { "download",   LV_SYMBOL_DOWNLOAD   },
    { "down",       LV_SYMBOL_DOWN       },
    { "drive",      LV_SYMBOL_DRIVE      },
    { "edit",       LV_SYMBOL_EDIT       },
    { "eject",      LV_SYMBOL_EJECT      },
    { "file",       LV_SYMBOL_FILE       },
    { "gps",        LV_SYMBOL_GPS        },
    { "home",       LV_SYMBOL_HOME       },
    { "image",      LV_SYMBOL_IMAGE      },
    { "keyboard",   LV_SYMBOL_KEYBOARD   },
    { "left",       LV_SYMBOL_LEFT       },
    { "list",       LV_SYMBOL_LIST       },
    { "loop",       LV_SYMBOL_LOOP       },
    { "minus",      LV_SYMBOL_MINUS      },
    { "mute",       LV_SYMBOL_MUTE       },
    { "next",       LV_SYMBOL_NEXT       },
    { "ok",         LV_SYMBOL_OK         },
    { "pause",      LV_SYMBOL_PAUSE      },
    { "play",       LV_SYMBOL_PLAY       },
    { "plus",       LV_SYMBOL_PLUS       },
    { "power",      LV_SYMBOL_POWER      },
    { "prev",       LV_SYMBOL_PREV       },
    { "refresh",    LV_SYMBOL_REFRESH    },
    { "right",      LV_SYMBOL_RIGHT      },
    { "save",       LV_SYMBOL_SAVE       },
    { "settings",   LV_SYMBOL_SETTINGS   },
    { "shuffle",    LV_SYMBOL_SHUFFLE    },
    { "stop",       LV_SYMBOL_STOP       },
    { "tint",       LV_SYMBOL_TINT       },
    { "trash",      LV_SYMBOL_TRASH      },
    { "up",         LV_SYMBOL_UP         },
    { "upload",     LV_SYMBOL_UPLOAD     },
    { "usb",        LV_SYMBOL_USB        },
    { "video",      LV_SYMBOL_VIDEO      },
    { "volume_max", LV_SYMBOL_VOLUME_MAX },
    { "volume_mid", LV_SYMBOL_VOLUME_MID },
    { "warning",    LV_SYMBOL_WARNING    },
    { "wifi",       LV_SYMBOL_WIFI       },
};

bool parse_hex_rgb(const std::string& s, uint32_t& out) {
    std::string v = (s.size() > 0 && s[0] == '#') ? s.substr(1) : s;
    uint32_t val = 0;
    const auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), val, 16);
    if (ec != std::errc() || ptr != v.data() + v.size() || v.size() != 6) {
        return false;
    }
    out = val;
    return true;
}

} // anonymous namespace

/***************************/
/*          Parse          */
/***************************/
std::optional<Config> Config::parse(int argc, char* argv[]) {
    Config cfg;
    std::string symbol_name;
    bool have_out = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        // Value-taking flags consume the next token via this lambda.
        auto next = [&]() -> std::string {
            return (i + 1 < argc) ? argv[++i] : "";
        };

        if (arg == "-h" || arg == "--help") {
            cfg.m_help_requested = true;
            return cfg;
        }

        // Named glyph from the LVGL symbol table (resolved after the loop
        // so --codepoint can still override it)
        if (arg == "--symbol") {
            symbol_name = next();
        }
        // Raw codepoint — decimal or 0x-hex (e.g. 0xF013 for settings)
        else if (arg == "--codepoint") {
            const std::string v = next();
            uint32_t cp = 0;
            const int base = (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) ? 16 : 10;
            const auto [ptr, ec] = std::from_chars(v.data() + (base == 16 ? 2 : 0),
                                                   v.data() + v.size(), cp, base);
            if (ec != std::errc() || ptr != v.data() + v.size()) {
                std::cerr << "invalid --codepoint\n";
                return std::nullopt;
            }
            cfg.m_codepoint = cp;
        }
        // Caption text rendered beneath the glyph (Montserrat 14)
        else if (arg == "--label") {
            cfg.m_label = next();
        }
        // Output PNG path (required)
        else if (arg == "--out" || arg == "-o") {
            cfg.m_output_path = next();
            have_out = true;
        }
        // Square icon edge in px — device keys are 128
        else if (arg == "--size") {
            const std::string v = next();
            int sz = 0;
            const auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), sz);
            if (ec != std::errc() || ptr != v.data() + v.size() || sz <= 0) {
                std::cerr << "invalid --size\n";
                return std::nullopt;
            }
            cfg.m_size = sz;
        }
        // Background fill, RRGGBB (with or without '#')
        else if (arg == "--bg") {
            if (!parse_hex_rgb(next(), cfg.m_bg_rgb)) {
                std::cerr << "invalid --bg (want RRGGBB)\n";
                return std::nullopt;
            }
        }
        // Glyph + caption color, RRGGBB
        else if (arg == "--fg") {
            if (!parse_hex_rgb(next(), cfg.m_fg_rgb)) {
                std::cerr << "invalid --fg (want RRGGBB)\n";
                return std::nullopt;
            }
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            return std::nullopt;
        }
    }

    // --symbol resolves after the loop so it composes with other flags
    if (!symbol_name.empty()) {
        const auto it = SYMBOLS.find(symbol_name);
        if (it == SYMBOLS.end()) {
            std::cerr << "unknown symbol: " << symbol_name << "\n";
            return std::nullopt;
        }
        cfg.m_codepoint = utf8_first_codepoint(it->second);
    }

    // Both a glyph and a destination are mandatory
    if (cfg.m_codepoint == 0 || !have_out) {
        return std::nullopt;
    }
    return cfg;
}

/***************************/
/*       Print usage       */
/***************************/
void Config::print_usage(const std::filesystem::path& program_path) {
    std::cout <<
        "ovt_icon_render - MK20 key icon generator\n\n"
        "Renders an LVGL symbol glyph (the same FontAwesome subset the app\n"
        "menu uses) onto a solid tile and writes a PNG suitable for\n"
        "embedding as a macropad key icon in a .Theme file. An optional\n"
        "caption is rendered beneath the glyph in Montserrat 14.\n\n"
        "Usage:\n"
        "  " << program_path.filename().string() << " --symbol <name> --out <png> [options]\n"
        "  " << program_path.filename().string() << " --codepoint <cp> --out <png> [options]\n\n"
        "Glyph selection (one required):\n"
        "  --symbol <name>      Named LVGL symbol (see list below)\n"
        "  --codepoint <cp>     Raw codepoint, decimal or 0x-hex (e.g. 0xF013)\n\n"
        "Required:\n"
        "  -o, --out <png>      Output PNG path\n\n"
        "Options:\n"
        "  --label <text>       Caption rendered under the glyph (Montserrat 14)\n"
        "  --size <px>          Square icon edge length (default: 128 = key size)\n"
        "  --bg <RRGGBB>        Background color (default: 2B2B3B dark slate)\n"
        "  --fg <RRGGBB>        Glyph + caption color (default: FFFFFF)\n"
        "  -h, --help           Show this help\n\n"
        "Examples:\n"
        "  " << program_path.filename().string() << " --symbol settings --label Set -o icons/settings.png\n"
        "  " << program_path.filename().string() << " --codepoint 0xF287 --size 128 --bg 1A1A2E -o usb.png\n\n"
        "Available symbols:";
    std::vector<std::string> names;
    names.reserve(SYMBOLS.size());
    for (const auto& [name, sym] : SYMBOLS) {
        names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    int col = 0;
    for (const auto& name : names) {
        if (col++ % 6 == 0) {
            std::cout << "\n  ";
        }
        std::cout << name;
        const int pad = 14 - static_cast<int>(name.size());
        std::cout << std::string(pad > 0 ? pad : 1, ' ');
    }
    std::cout << "\n\n";
}

} // namespace ovb::tools::icon_render
