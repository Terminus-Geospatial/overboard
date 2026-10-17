/**
 * @file    input.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 serial input implementation.
 */
#include <overboard/hal/mk20/input.hpp>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::mk20 {

/***************************/
/*        Constructor      */
/***************************/
Mk20_Input::Mk20_Input( std::shared_ptr<mpad::Device> device )
    : m_device( std::move( device ) ) {
    m_device->on_key( [this]( const mpad::Pad_Key_Event& evt ) {
        const int idx = key_index_for( evt.row, evt.col );
        if (idx < 0) {
            LOG_TRACE( "Mk20_Input: unmapped event row=", evt.row,
                       " col=", evt.col );
            return;
        }
        std::lock_guard lk( m_mtx );
        m_queue.push_back( Key_Event{
            .kind      = Key_Event_Kind::Action,
            .key_index = idx,
            .codepoint = 0,
            .type      = evt.pressed ? Key_Event_Type::Press
                                     : Key_Event_Type::Release,
            .input_key = core::Input_Key::NONE,
        } );
    } );
}

/***************************/
/*          Poll           */
/***************************/
bool Mk20_Input::poll( Key_Event& out_event ) {
    std::lock_guard lk( m_mtx );
    if (m_queue.empty()) {
        return false;
    }
    out_event = m_queue.front();
    m_queue.pop_front();
    return true;
}

/***************************/
/*      Should quit        */
/***************************/
bool Mk20_Input::should_quit() const {
    return false;
}

/***************************/
/*          Pump           */
/***************************/
void Mk20_Input::pump() {
    // Reader thread inside mpad::Device pumps continuously.
}

/***************************/
/*         Device          */
/***************************/
std::shared_ptr<mpad::Device> Mk20_Input::device() const {
    return m_device;
}

/***************************/
/*     Key index for       */
/***************************/
int Mk20_Input::key_index_for( int row, int col ) {
    // Encoder pseudo-rows: row == col >= 100
    if (row == mpad::Pad_Key_Event::ENCODER_LEFT && col == row) {
        return ENCODER_LEFT_INDEX;
    }
    if (row == mpad::Pad_Key_Event::ENCODER_RIGHT && col == row) {
        return ENCODER_RIGHT_INDEX;
    }
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
        return row * COLS + col;
    }
    return -1;
}

} // namespace ovb::hal::mk20
