/**
 * @file    input_key_mapping.cpp
 * @author  Marvin Smith
 * @date    2026-05-31
 *
 * @brief   SDL Scancode to Input_Key mapping implementation
 */
#include <overboard/hal/sdl/input_key_mapping.hpp>

// C++ Standard Libraries
#include <array>

// Third-Party Libraries
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <SDL2/SDL.h>
#pragma GCC diagnostic pop

namespace ovb::hal::sdl {

// Mapping table from SDL scancode to Input_Key
static constexpr std::array<core::Input_Key, SDL_NUM_SCANCODES> SCANCODES_TO_INPUT_KEYS = [] {
    std::array<core::Input_Key, SDL_NUM_SCANCODES> table{};
    table.fill(core::Input_Key::NONE);

    // Function keys
    table[SDL_SCANCODE_F1]  = core::Input_Key::F1;
    table[SDL_SCANCODE_F2]  = core::Input_Key::F2;
    table[SDL_SCANCODE_F3]  = core::Input_Key::F3;
    table[SDL_SCANCODE_F4]  = core::Input_Key::F4;
    table[SDL_SCANCODE_F5]  = core::Input_Key::F5;
    table[SDL_SCANCODE_F6]  = core::Input_Key::F6;
    table[SDL_SCANCODE_F7]  = core::Input_Key::F7;
    table[SDL_SCANCODE_F8]  = core::Input_Key::F8;
    table[SDL_SCANCODE_F9]  = core::Input_Key::F9;
    table[SDL_SCANCODE_F10] = core::Input_Key::F10;
    table[SDL_SCANCODE_F11] = core::Input_Key::F11;
    table[SDL_SCANCODE_F12] = core::Input_Key::F12;
    table[SDL_SCANCODE_F13] = core::Input_Key::F13;
    table[SDL_SCANCODE_F14] = core::Input_Key::F14;
    table[SDL_SCANCODE_F15] = core::Input_Key::F15;
    table[SDL_SCANCODE_F16] = core::Input_Key::F16;
    table[SDL_SCANCODE_F17] = core::Input_Key::F17;
    table[SDL_SCANCODE_F18] = core::Input_Key::F18;
    table[SDL_SCANCODE_F19] = core::Input_Key::F19;
    table[SDL_SCANCODE_F20] = core::Input_Key::F20;
    table[SDL_SCANCODE_F21] = core::Input_Key::F21;
    table[SDL_SCANCODE_F22] = core::Input_Key::F22;
    table[SDL_SCANCODE_F23] = core::Input_Key::F23;
    table[SDL_SCANCODE_F24] = core::Input_Key::F24;

    // Top row digits
    table[SDL_SCANCODE_0] = core::Input_Key::KEY_0;
    table[SDL_SCANCODE_1] = core::Input_Key::KEY_1;
    table[SDL_SCANCODE_2] = core::Input_Key::KEY_2;
    table[SDL_SCANCODE_3] = core::Input_Key::KEY_3;
    table[SDL_SCANCODE_4] = core::Input_Key::KEY_4;
    table[SDL_SCANCODE_5] = core::Input_Key::KEY_5;
    table[SDL_SCANCODE_6] = core::Input_Key::KEY_6;
    table[SDL_SCANCODE_7] = core::Input_Key::KEY_7;
    table[SDL_SCANCODE_8] = core::Input_Key::KEY_8;
    table[SDL_SCANCODE_9] = core::Input_Key::KEY_9;

    // Keypad digits
    table[SDL_SCANCODE_KP_0] = core::Input_Key::NUMPAD_0;
    table[SDL_SCANCODE_KP_1] = core::Input_Key::NUMPAD_1;
    table[SDL_SCANCODE_KP_2] = core::Input_Key::NUMPAD_2;
    table[SDL_SCANCODE_KP_3] = core::Input_Key::NUMPAD_3;
    table[SDL_SCANCODE_KP_4] = core::Input_Key::NUMPAD_4;
    table[SDL_SCANCODE_KP_5] = core::Input_Key::NUMPAD_5;
    table[SDL_SCANCODE_KP_6] = core::Input_Key::NUMPAD_6;
    table[SDL_SCANCODE_KP_7] = core::Input_Key::NUMPAD_7;
    table[SDL_SCANCODE_KP_8] = core::Input_Key::NUMPAD_8;
    table[SDL_SCANCODE_KP_9] = core::Input_Key::NUMPAD_9;

    // Keypad operators
    table[SDL_SCANCODE_KP_PLUS]     = core::Input_Key::NUMPAD_ADD;
    table[SDL_SCANCODE_KP_MINUS]    = core::Input_Key::NUMPAD_SUBTRACT;
    table[SDL_SCANCODE_KP_MULTIPLY] = core::Input_Key::NUMPAD_MULTIPLY;
    table[SDL_SCANCODE_KP_DIVIDE]   = core::Input_Key::NUMPAD_DIVIDE;
    table[SDL_SCANCODE_KP_PERIOD]   = core::Input_Key::NUMPAD_DECIMAL;
    table[SDL_SCANCODE_KP_ENTER]    = core::Input_Key::NUMPAD_ENTER;

    // Navigation
    table[SDL_SCANCODE_LEFT]     = core::Input_Key::LEFT;
    table[SDL_SCANCODE_RIGHT]    = core::Input_Key::RIGHT;
    table[SDL_SCANCODE_UP]       = core::Input_Key::UP;
    table[SDL_SCANCODE_DOWN]     = core::Input_Key::DOWN;
    table[SDL_SCANCODE_HOME]     = core::Input_Key::HOME;
    table[SDL_SCANCODE_END]      = core::Input_Key::END;
    table[SDL_SCANCODE_PAGEUP]   = core::Input_Key::PAGE_UP;
    table[SDL_SCANCODE_PAGEDOWN] = core::Input_Key::PAGE_DOWN;
    table[SDL_SCANCODE_INSERT]   = core::Input_Key::INSERT;
    table[SDL_SCANCODE_DELETE]   = core::Input_Key::DELETE;

    // Editing
    table[SDL_SCANCODE_RETURN]    = core::Input_Key::RETURN;
    table[SDL_SCANCODE_BACKSPACE] = core::Input_Key::BACKSPACE;
    table[SDL_SCANCODE_TAB]       = core::Input_Key::TAB;
    table[SDL_SCANCODE_ESCAPE]    = core::Input_Key::ESCAPE;
    table[SDL_SCANCODE_SPACE]     = core::Input_Key::SPACE;

    // Modifiers
    table[SDL_SCANCODE_LSHIFT] = core::Input_Key::SHIFT;
    table[SDL_SCANCODE_RSHIFT] = core::Input_Key::SHIFT;
    table[SDL_SCANCODE_LCTRL]  = core::Input_Key::CONTROL;
    table[SDL_SCANCODE_RCTRL]  = core::Input_Key::CONTROL;
    table[SDL_SCANCODE_LALT]   = core::Input_Key::ALT;
    table[SDL_SCANCODE_RALT]   = core::Input_Key::ALT;
    table[SDL_SCANCODE_LGUI]   = core::Input_Key::META;
    table[SDL_SCANCODE_RGUI]   = core::Input_Key::META;

    // Letters
    table[SDL_SCANCODE_A] = core::Input_Key::KEY_A;
    table[SDL_SCANCODE_B] = core::Input_Key::KEY_B;
    table[SDL_SCANCODE_C] = core::Input_Key::KEY_C;
    table[SDL_SCANCODE_D] = core::Input_Key::KEY_D;
    table[SDL_SCANCODE_E] = core::Input_Key::KEY_E;
    table[SDL_SCANCODE_F] = core::Input_Key::KEY_F;
    table[SDL_SCANCODE_G] = core::Input_Key::KEY_G;
    table[SDL_SCANCODE_H] = core::Input_Key::KEY_H;
    table[SDL_SCANCODE_I] = core::Input_Key::KEY_I;
    table[SDL_SCANCODE_J] = core::Input_Key::KEY_J;
    table[SDL_SCANCODE_K] = core::Input_Key::KEY_K;
    table[SDL_SCANCODE_L] = core::Input_Key::KEY_L;
    table[SDL_SCANCODE_M] = core::Input_Key::KEY_M;
    table[SDL_SCANCODE_N] = core::Input_Key::KEY_N;
    table[SDL_SCANCODE_O] = core::Input_Key::KEY_O;
    table[SDL_SCANCODE_P] = core::Input_Key::KEY_P;
    table[SDL_SCANCODE_Q] = core::Input_Key::KEY_Q;
    table[SDL_SCANCODE_R] = core::Input_Key::KEY_R;
    table[SDL_SCANCODE_S] = core::Input_Key::KEY_S;
    table[SDL_SCANCODE_T] = core::Input_Key::KEY_T;
    table[SDL_SCANCODE_U] = core::Input_Key::KEY_U;
    table[SDL_SCANCODE_V] = core::Input_Key::KEY_V;
    table[SDL_SCANCODE_W] = core::Input_Key::KEY_W;
    table[SDL_SCANCODE_X] = core::Input_Key::KEY_X;
    table[SDL_SCANCODE_Y] = core::Input_Key::KEY_Y;
    table[SDL_SCANCODE_Z] = core::Input_Key::KEY_Z;

    // Symbols
    table[SDL_SCANCODE_MINUS]        = core::Input_Key::MINUS;
    table[SDL_SCANCODE_EQUALS]       = core::Input_Key::EQUAL;
    table[SDL_SCANCODE_LEFTBRACKET]  = core::Input_Key::BRACKET_LEFT;
    table[SDL_SCANCODE_RIGHTBRACKET] = core::Input_Key::BRACKET_RIGHT;
    table[SDL_SCANCODE_BACKSLASH]    = core::Input_Key::BACKSLASH;
    table[SDL_SCANCODE_SEMICOLON]    = core::Input_Key::SEMICOLON;
    table[SDL_SCANCODE_APOSTROPHE]   = core::Input_Key::APOSTROPHE;
    table[SDL_SCANCODE_GRAVE]        = core::Input_Key::GRAVE;
    table[SDL_SCANCODE_COMMA]        = core::Input_Key::COMMA;
    table[SDL_SCANCODE_PERIOD]       = core::Input_Key::PERIOD;
    table[SDL_SCANCODE_SLASH]        = core::Input_Key::SLASH;

    return table;
}();

/***********************************************/
/*       Scancode to Input_Key lookup          */
/***********************************************/
core::Input_Key scancode_to_input_key(SDL_Scancode scancode) {
    if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) {
        return core::Input_Key::NONE;
    }
    return SCANCODES_TO_INPUT_KEYS[static_cast<std::size_t>(scancode)];
}

/***********************************************/
/*       Input_Key to scancode lookup          */
/***********************************************/
SDL_Scancode input_key_to_scancode(core::Input_Key key) {
    for (std::size_t sc = 0; sc < static_cast<std::size_t>(SDL_NUM_SCANCODES); ++sc) {
        if (SCANCODES_TO_INPUT_KEYS[sc] == key) {
            return static_cast<SDL_Scancode>(static_cast<int>(sc));
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

} // namespace ovb::hal::sdl
