/**
 * @file    stdout_logger.cpp
 * @author  Marvin Smith
 * @date    2026-05-22
 *
 * @brief   Stdout logger implementation
 */
#include <overboard/log/stdout_logger.hpp>

// C++ Standard Libraries
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>

// Third-Party Libraries
#ifdef TARGET_RP2350
#include <pico/stdlib.h>
#include <hardware/uart.h>
#include <hardware/gpio.h>
#endif

namespace ovb::log {

// Static unique_ptr for singleton instance
std::unique_ptr<Stdout_Logger> Stdout_Logger::s_instance;

Stdout_Logger::Stdout_Logger(Log_Level min_level, bool enable_uart)
    : m_min_level(min_level) {

    // Initialize UART if requested and on RP2350
    if (enable_uart) {
#ifdef TARGET_RP2350
        uart_init(uart0, 115200);
        gpio_set_function(0, GPIO_FUNC_UART);  // UART TX
        gpio_set_function(1, GPIO_FUNC_UART);  // UART RX
#endif
    }
}

Stdout_Logger& Stdout_Logger::instance() {
    if (!s_instance) {
        s_instance = std::make_unique<Stdout_Logger>(Log_Level::Debug);
    }
    return *s_instance;
}

void Stdout_Logger::initialize(Log_Level level) {
    s_instance = std::make_unique<Stdout_Logger>(level);
}

uint64_t Stdout_Logger::now_us() const {
    using namespace std::chrono;
    return static_cast<uint64_t>(
        duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

const char* Stdout_Logger::level_tag(Log_Level level) {
    switch (level) {
        case Log_Level::Trace: return "TRACE";
        case Log_Level::Debug: return "DEBUG";
        case Log_Level::Info:  return "INFO";
        case Log_Level::Warn:  return "WARN";
        case Log_Level::Error: return "ERROR";
        default:               return "???";
    }
}

void Stdout_Logger::log(Log_Level level, std::string_view message) {
    if (level < m_min_level) return;

    // Serialize output — the mpad reader thread and other workers log too,
    // and stdout is fully buffered when piped (systemd journal), so flush
    // every line to keep journald output timely and lines intact.
    static std::mutex s_log_mtx;
    std::lock_guard lk(s_log_mtx);

    const uint64_t us = now_us();
    const uint64_t secs  = us / 1'000'000;
    const uint64_t frac  = (us % 1'000'000) / 1'000;

    // Format message
    char formatted[512];
    snprintf(formatted, sizeof(formatted),
        "[%4llu.%03llus] [%s] %.*s\n",
        static_cast<unsigned long long>(secs),
        static_cast<unsigned long long>(frac),
        level_tag(level),
        static_cast<int>(message.size()),
        message.data());

    // Output to stdout
    std::printf("%s", formatted);
    std::fflush(stdout);

    // Output to UART if available
#ifdef TARGET_RP2350
    uart_puts(uart0, formatted);
#endif
}

/***************************/
/*    Log with Location    */
/***************************/
void Stdout_Logger::log(Log_Level level, std::source_location loc, std::string_view message) {
    if (level < m_min_level) return;

    static std::mutex s_log_mtx;
    std::lock_guard lk(s_log_mtx);

    const uint64_t us = now_us();
    const uint64_t secs  = us / 1'000'000;
    const uint64_t frac  = (us % 1'000'000) / 1'000;

    // Format message
    char formatted[512];
    snprintf(formatted, sizeof(formatted),
        "[%4llu.%03llus] [%s] %s:%d: %.*s\n",
        static_cast<unsigned long long>(secs),
        static_cast<unsigned long long>(frac),
        level_tag(level),
        loc.file_name(),
        static_cast<int>(loc.line()),
        static_cast<int>(message.size()),
        message.data());

    // Output to stdout
    std::printf("%s", formatted);
    std::fflush(stdout);

    // Output to UART if available
#ifdef TARGET_RP2350
    uart_puts(uart0, formatted);
#endif
}

} // namespace ovb::log
