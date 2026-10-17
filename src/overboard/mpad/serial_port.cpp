/**
 * @file    serial_port.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   POSIX serial port implementation.
 */
#include <overboard/mpad/serial_port.hpp>

// C++ Standard Libraries
#include <cerrno>
#include <cstring>

// POSIX Libraries
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace ovb::mpad {

/***************************/
/*        Constructor      */
/***************************/
Serial_Port::Serial_Port( const std::filesystem::path& path, unsigned baud ) {
    m_fd = ::open( path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK );
    if (m_fd < 0) {
        return;
    }

    // Configure 8N1 with a 100ms read timeout (VTIME = 1 decisecond).
    // VMIN=0 makes read() return whatever is available immediately.
    struct termios tio {};
    if (tcgetattr( m_fd, &tio ) != 0) {
        ::close( m_fd );
        m_fd = -1;
        return;
    }
    cfmakeraw( &tio );
    tio.c_cflag |= CLOCAL | CREAD;
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 1;

    speed_t spd = B115200;
    switch (baud) {
        case 9600:   spd = B9600;   break;
        case 19200:  spd = B19200;  break;
        case 38400:  spd = B38400;  break;
        case 57600:  spd = B57600;  break;
        case 230400: spd = B230400; break;
        default:                    break;
    }
    cfsetispeed( &tio, spd );
    cfsetospeed( &tio, spd );

    if (tcsetattr( m_fd, TCSANOW, &tio ) != 0) {
        ::close( m_fd );
        m_fd = -1;
        return;
    }
    tcflush( m_fd, TCIOFLUSH );
}

/***************************/
/*        Destructor       */
/***************************/
Serial_Port::~Serial_Port() {
    close();
}

/***************************/
/*        Is open          */
/***************************/
bool Serial_Port::is_open() const {
    return m_fd >= 0;
}

/***************************/
/*          Read           */
/***************************/
int Serial_Port::read( std::span<std::byte> buf ) {
    if (m_fd < 0) {
        return -1;
    }
    const ssize_t n = ::read( m_fd, buf.data(), buf.size() );
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return 0;
    }
    return static_cast<int>( n );
}

/***************************/
/*         Write           */
/***************************/
bool Serial_Port::write( std::span<const std::byte> data ) {
    if (m_fd < 0) {
        return false;
    }
    size_t off = 0;
    while (off < data.size()) {
        const ssize_t n = ::write( m_fd, data.data() + off, data.size() - off );
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            return false;
        }
        off += static_cast<size_t>( n );
    }
    return true;
}

/***************************/
/*         Flush           */
/***************************/
void Serial_Port::flush() {
    if (m_fd >= 0) {
        tcdrain( m_fd );
    }
}

/***************************/
/*         Close           */
/***************************/
void Serial_Port::close() {
    if (m_fd >= 0) {
        ::close( m_fd );
        m_fd = -1;
    }
}

} // namespace ovb::mpad
