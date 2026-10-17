/**
 * @file    serial_port.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   Minimal POSIX serial port wrapper for USB CDC-ACM devices.
 *
 * Works on Linux and macOS via termios. The MK20's baud rate is
 * ignored by the USB CDC stack but is set anyway for hygiene.
 */
#pragma once

// C++ Standard Libraries
#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace ovb::mpad {

/**
 * @brief RAII wrapper around a POSIX serial file descriptor.
 *
 * Configured for 8N1 with a short read timeout (VTIME) so the
 * reader thread can poll without blocking forever.
 */
class Serial_Port {
    public:

        /**
         * @brief Open and configure the port
         * @param path Device node (e.g. /dev/ttyACM0, /dev/cu.usbmodem1101)
         * @param baud Baud rate constant (default 115200; ignored by CDC)
         */
        explicit Serial_Port( const std::filesystem::path& path,
                              unsigned baud = 115200 );

        /// Close the port
        ~Serial_Port();

        Serial_Port( const Serial_Port& )            = delete;
        Serial_Port& operator=( const Serial_Port& ) = delete;
        Serial_Port( Serial_Port&& )                 = delete;
        Serial_Port& operator=( Serial_Port&& )      = delete;

        /// @brief True while the port is open
        bool is_open() const;

        /**
         * @brief Read up to buf.size() bytes (up to the read timeout)
         * @param buf Destination buffer
         * @return Number of bytes read, -1 on error
         */
        int read( std::span<std::byte> buf );

        /**
         * @brief Write all bytes to the port
         * @param data Data to write
         * @return true on success
         */
        bool write( std::span<const std::byte> data );

        /// @brief Flush output buffers
        void flush();

        /// @brief Close the port (idempotent)
        void close();

    private:

        /// @brief File descriptor (-1 when closed)
        int m_fd = -1;
};

} // namespace ovb::mpad
