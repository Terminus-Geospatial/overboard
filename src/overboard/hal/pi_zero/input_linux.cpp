/**
 * @file    input_linux.cpp
 * @author  Marvin Smith
 * @date    2026-06-05
 *
 * @brief   Linux input event implementation
 */
#include <overboard/hal/pi_zero/input_linux.hpp>

// C++ Standard Libraries
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <unistd.h>

#include <array>
#include <cstring>

// Undefine Linux input.h macros that conflict with Input_Key enum names
// MUST be done BEFORE including input_key.hpp to prevent macro expansion
// in the enum definition (e.g., KEY_0 expanding to 11)
#undef KEY_A
#undef KEY_B
#undef KEY_C
#undef KEY_D
#undef KEY_E
#undef KEY_F
#undef KEY_G
#undef KEY_H
#undef KEY_I
#undef KEY_J
#undef KEY_K
#undef KEY_L
#undef KEY_M
#undef KEY_N
#undef KEY_O
#undef KEY_P
#undef KEY_Q
#undef KEY_R
#undef KEY_S
#undef KEY_T
#undef KEY_U
#undef KEY_V
#undef KEY_W
#undef KEY_X
#undef KEY_Y
#undef KEY_Z
#undef KEY_0
#undef KEY_1
#undef KEY_2
#undef KEY_3
#undef KEY_4
#undef KEY_5
#undef KEY_6
#undef KEY_7
#undef KEY_8
#undef KEY_9

// Overboard Libraries
#include <overboard/core/input_key.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::pi_zero {

/********************************************/
/*      Linux Input to Input Key Code       */
/********************************************/
static ovb::core::Input_Key linux_code_to_input_key(unsigned int code) {

    // Linux key codes from /usr/include/linux/input-event-codes.h
    // (Conflicting KEY_* macros undefined above to prevent expansion errors)
    switch (code) {
        // Escape and control (1, 14, 15, 28, 57)
        case 1:   return core::Input_Key::ESCAPE;     // KEY_ESC
        case 15:  return core::Input_Key::TAB;        // KEY_TAB
        case 14:  return core::Input_Key::BACKSPACE; // KEY_BACKSPACE
        case 28:  return core::Input_Key::RETURN;    // KEY_ENTER
        case 57:  return core::Input_Key::SPACE;     // KEY_SPACE

        // Modifiers (29, 42, 54, 56, 100, 125, 126)
        case 42:  return core::Input_Key::SHIFT;       // KEY_LEFTSHIFT
        case 54:  return core::Input_Key::SHIFT;       // KEY_RIGHTSHIFT
        case 29:  return core::Input_Key::CONTROL;     // KEY_LEFTCTRL
        case 97:  return core::Input_Key::CONTROL;     // KEY_RIGHTCTRL
        case 56:  return core::Input_Key::ALT;         // KEY_LEFTALT
        case 100: return core::Input_Key::ALT;         // KEY_RIGHTALT

        // Navigation (102-106, 110-111)
        case 105: return core::Input_Key::LEFT;        // KEY_LEFT
        case 106: return core::Input_Key::RIGHT;       // KEY_RIGHT
        case 103: return core::Input_Key::UP;          // KEY_UP
        case 108: return core::Input_Key::DOWN;        // KEY_DOWN
        case 102: return core::Input_Key::HOME;        // KEY_HOME
        case 107: return core::Input_Key::END;         // KEY_END
        case 104: return core::Input_Key::PAGE_UP;     // KEY_PAGEUP
        case 109: return core::Input_Key::PAGE_DOWN;   // KEY_PAGEDOWN
        case 110: return core::Input_Key::INSERT;      // KEY_INSERT
        case 111: return core::Input_Key::DELETE;      // KEY_DELETE

        // Function keys (59-69)
        case 59:  return core::Input_Key::F1;          // KEY_F1
        case 60:  return core::Input_Key::F2;          // KEY_F2
        case 61:  return core::Input_Key::F3;          // KEY_F3
        case 62:  return core::Input_Key::F4;          // KEY_F4
        case 63:  return core::Input_Key::F5;          // KEY_F5
        case 64:  return core::Input_Key::F6;          // KEY_F6
        case 65:  return core::Input_Key::F7;          // KEY_F7
        case 66:  return core::Input_Key::F8;          // KEY_F8
        case 67:  return core::Input_Key::F9;          // KEY_F9
        case 68:  return core::Input_Key::F10;         // KEY_F10
        case 87:  return core::Input_Key::F11;         // KEY_F11
        case 88:  return core::Input_Key::F12;         // KEY_F12
        case 183: return core::Input_Key::F13;         // KEY_F13
        case 184: return core::Input_Key::F14;         // KEY_F14
        case 185: return core::Input_Key::F15;         // KEY_F15
        case 186: return core::Input_Key::F16;         // KEY_F16
        case 187: return core::Input_Key::F17;         // KEY_F17
        case 188: return core::Input_Key::F18;         // KEY_F18
        case 189: return core::Input_Key::F19;         // KEY_F19
        case 190: return core::Input_Key::F20;         // KEY_F20
        case 191: return core::Input_Key::F21;         // KEY_F21
        case 192: return core::Input_Key::F22;         // KEY_F22
        case 193: return core::Input_Key::F23;         // KEY_F23
        case 194: return core::Input_Key::F24;         // KEY_F24

        // Digits (top row: 2-10, 11)
        case 11:  return core::Input_Key::KEY_0;
        case 2:   return core::Input_Key::KEY_1;
        case 3:   return core::Input_Key::KEY_2;
        case 4:   return core::Input_Key::KEY_3;
        case 5:   return core::Input_Key::KEY_4;
        case 6:   return core::Input_Key::KEY_5;
        case 7:   return core::Input_Key::KEY_6;
        case 8:   return core::Input_Key::KEY_7;
        case 9:   return core::Input_Key::KEY_8;
        case 10:  return core::Input_Key::KEY_9;

        // Letters (16-25, 30-38, 44-50)
        case 30:  return core::Input_Key::KEY_A;
        case 48:  return core::Input_Key::KEY_B;
        case 46:  return core::Input_Key::KEY_C;
        case 32:  return core::Input_Key::KEY_D;
        case 18:  return core::Input_Key::KEY_E;
        case 33:  return core::Input_Key::KEY_F;
        case 34:  return core::Input_Key::KEY_G;
        case 35:  return core::Input_Key::KEY_H;
        case 23:  return core::Input_Key::KEY_I;
        case 36:  return core::Input_Key::KEY_J;
        case 37:  return core::Input_Key::KEY_K;
        case 38:  return core::Input_Key::KEY_L;
        case 50:  return core::Input_Key::KEY_M;
        case 49:  return core::Input_Key::KEY_N;
        case 24:  return core::Input_Key::KEY_O;
        case 25:  return core::Input_Key::KEY_P;
        case 16:  return core::Input_Key::KEY_Q;
        case 19:  return core::Input_Key::KEY_R;
        case 31:  return core::Input_Key::KEY_S;
        case 20:  return core::Input_Key::KEY_T;
        case 22:  return core::Input_Key::KEY_U;
        case 47:  return core::Input_Key::KEY_V;
        case 17:  return core::Input_Key::KEY_W;
        case 45:  return core::Input_Key::KEY_X;
        case 21:  return core::Input_Key::KEY_Y;
        case 44:  return core::Input_Key::KEY_Z;

        // Numpad
        case 82:  return core::Input_Key::NUMPAD_0;       // KEY_KP0
        case 79:  return core::Input_Key::NUMPAD_1;       // KEY_KP1
        case 80:  return core::Input_Key::NUMPAD_2;       // KEY_KP2
        case 81:  return core::Input_Key::NUMPAD_3;       // KEY_KP3
        case 75:  return core::Input_Key::NUMPAD_4;       // KEY_KP4
        case 76:  return core::Input_Key::NUMPAD_5;       // KEY_KP5
        case 77:  return core::Input_Key::NUMPAD_6;       // KEY_KP6
        case 71:  return core::Input_Key::NUMPAD_7;       // KEY_KP7
        case 72:  return core::Input_Key::NUMPAD_8;       // KEY_KP8
        case 73:  return core::Input_Key::NUMPAD_9;       // KEY_KP9
        case 78:  return core::Input_Key::NUMPAD_ADD;     // KEY_KPPLUS
        case 74:  return core::Input_Key::NUMPAD_SUBTRACT; // KEY_KPMINUS
        case 55:  return core::Input_Key::NUMPAD_MULTIPLY; // KEY_KPASTERISK
        case 98:  return core::Input_Key::NUMPAD_DIVIDE;  // KEY_KPSLASH
        case 83:  return core::Input_Key::NUMPAD_DECIMAL; // KEY_KPDOT
        case 96:  return core::Input_Key::NUMPAD_ENTER;   // KEY_KPENTER

        // Symbols (12-13, 26-27, 39-43, 51-53)
        case 12:  return core::Input_Key::MINUS;        // KEY_MINUS
        case 13:  return core::Input_Key::EQUAL;        // KEY_EQUAL
        case 26:  return core::Input_Key::BRACKET_LEFT; // KEY_LEFTBRACE
        case 27:  return core::Input_Key::BRACKET_RIGHT; // KEY_RIGHTBRACE
        case 43:  return core::Input_Key::BACKSLASH;    // KEY_BACKSLASH
        case 39:  return core::Input_Key::SEMICOLON;    // KEY_SEMICOLON
        case 40:  return core::Input_Key::APOSTROPHE;   // KEY_APOSTROPHE
        case 41:  return core::Input_Key::GRAVE;        // KEY_GRAVE
        case 51:  return core::Input_Key::COMMA;        // KEY_COMMA
        case 52:  return core::Input_Key::PERIOD;       // KEY_DOT
        case 53:  return core::Input_Key::SLASH;        // KEY_SLASH

        default: return core::Input_Key::NONE;
    }
}

struct Linux_Input::Impl {
    std::vector<int>                fds;
    std::vector<std::string>        device_paths;
    std::atomic<bool>               quit_requested = false;
    std::array<pollfd, 16>          poll_fds;
    size_t                          num_poll_fds = 0;
};

/************************************/
/*          Constructor             */
/************************************/
Linux_Input::Linux_Input(const std::vector<std::string>& device_paths)
    : m_impl(std::make_unique<Impl>()) {
    m_impl->device_paths = device_paths;

    for (const auto& path : device_paths) {
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            LOG_DEBUG("Failed to open input device: ", path);
            continue;
        }

        // Grab the device exclusively
        if (ioctl(fd, EVIOCGRAB, 1) < 0) {
            LOG_DEBUG("Failed to grab input device: ", path);
        }

        m_impl->fds.push_back(fd);
        if (m_impl->num_poll_fds < m_impl->poll_fds.size()) {
            m_impl->poll_fds[m_impl->num_poll_fds].fd = fd;
            m_impl->poll_fds[m_impl->num_poll_fds].events = POLLIN;
            m_impl->num_poll_fds++;
        }
    }

    LOG_INFO("Linux_Input: Opened ", m_impl->fds.size(), " input devices");
}

/************************************/
/*          Destructor              */
/************************************/
Linux_Input::~Linux_Input() {
    for (int fd : m_impl->fds) {
        ioctl(fd, EVIOCGRAB, 0);  // Release grab
        close(fd);
    }
}

/************************************/
/*          Poll for events         */
/************************************/
bool Linux_Input::poll(Key_Event& out_event) {
    using ovb::core::Input_Key;
    struct input_event ev;

    for (int fd : m_impl->fds) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == sizeof(ev)) {
            if (ev.type == EV_KEY) {
                // Map Linux key code to Input_Key
                auto input_key = linux_code_to_input_key(ev.code);

                // Skip unmapped keys (modifiers, etc.)
                if (input_key == Input_Key::NONE) {
                    continue;
                }

                // Store Input_Key value directly - app layer will map to key_index
                out_event.kind = Key_Event_Kind::Action;
                out_event.key_index = static_cast<int>(input_key);
                out_event.type = (ev.value == 1) ? Key_Event_Type::Press : Key_Event_Type::Release;

                return true;
            }
        }
    }

    return false;
}

/************************************/
/*          Should quit             */
/************************************/
bool Linux_Input::should_quit() const {
    return m_impl->quit_requested.load();
}

/************************************/
/*          Pump events             */
/************************************/
void Linux_Input::pump() {
    if (m_impl->num_poll_fds == 0) {
        return;
    }

    int ret = ::poll(m_impl->poll_fds.data(), m_impl->num_poll_fds, 0);
    if (ret < 0) {
        LOG_DEBUG("poll() failed");
    }
}

} // namespace ovb::hal::pi_zero
