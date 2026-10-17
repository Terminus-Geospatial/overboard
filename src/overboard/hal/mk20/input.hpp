/**
 * @file    input.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 macropad input — serial key events -> hal::Key_Event.
 *
 * The MK20 reports key presses as PROACTIVE_CMD serial events carrying
 * (row, col, pressed). Grid keys map to key_index = row*5 + col (0-19);
 * encoder clicks arrive at pseudo-rows 100 (left) and 103 (right) and
 * map to key_index 20 / 21. Rotation produces no serial events — use
 * encoder_keyboard bindings (evdev) for that.
 */
#pragma once

// C++ Standard Libraries
#include <deque>
#include <memory>
#include <mutex>

// Project Libraries
#include <overboard/hal/i_input.hpp>
#include <overboard/mpad/device.hpp>

namespace ovb::hal::mk20 {

/**
 * @brief I_Input over the MK20 serial channel.
 *
 * Owns a shared mpad::Device (the same connection can feed the
 * Mk20_Keymap presenter). The reader thread inside Device does the
 * pumping; poll() drains the event queue it fills.
 */
class Mk20_Input : public I_Input {
    public:

        /// @brief MK20 grid geometry — key_index = row*COLS + col
        static constexpr int COLS { 5 };
        static constexpr int ROWS { 4 };
        static constexpr int KEY_COUNT { COLS * ROWS };

        /// @brief Key indices for encoder pseudo-rows
        static constexpr int ENCODER_LEFT_INDEX  { KEY_COUNT };
        static constexpr int ENCODER_RIGHT_INDEX { KEY_COUNT + 1 };

        /**
         * @brief Wrap a live device connection
         * @param device Connected mpad::Device (shared with the presenter)
         */
        explicit Mk20_Input( std::shared_ptr<mpad::Device> device );

        /// @brief Pop the next queued event; false when empty
        bool poll( Key_Event& out_event ) override;

        /// @brief Device disconnect doesn't request app quit
        bool should_quit() const override;

        /// @brief No-op — the device reader thread pumps continuously
        void pump() override;

        /// @brief Underlying device (for the keymap presenter to share)
        std::shared_ptr<mpad::Device> device() const;

        /// @brief Map a device (row,col) to a layout key index; -1 if unknown
        static int key_index_for( int row, int col );

    private:

        std::shared_ptr<mpad::Device> m_device;
        std::mutex                    m_mtx;
        std::deque<Key_Event>         m_queue;
};

} // namespace ovb::hal::mk20
