/**
 * @file    system_info.hpp
 * @author  Marvin Smith
 * @date    2026-06-10
 *
 * @brief   SDL platform system information implementation
 *
 * For SDL (simulator), we use host OS APIs:
 * - macOS: IOKit for CPU temp, host_statistics for memory
 * - Linux: /sys/class/thermal for temp, statfs for storage
 * Battery is typically unavailable/mocked on development machines.
 */
#pragma once

// C++ Standard Libraries
#include <chrono>
#include <mutex>
#include <optional>

// Project Libraries
#include <overboard/hal/i_system_info.hpp>

namespace ovb::hal::sdl {

/**
 * @brief SDL platform system info implementation
 *
 * Uses host OS APIs to gather system information.
 * Some sensors (battery) may be unavailable on typical dev machines.
 */
class SDL_System_Info : public I_System_Info {
    public:
        /**
         * @brief Constructor
         */
        SDL_System_Info();

        /**
         * @brief Get current system information
         */
        System_Info get_info() override;

        /**
         * @brief Check if CPU temperature is available
         */
        bool has_cpu_temp() const override;

        /**
         * @brief Check if battery info is available
         */
        bool has_battery() const override;

        /**
         * @brief Check if storage stats are available
         */
        bool has_storage() const override;

        /**
         * @brief Check if USB status is available
         */
        bool has_usb() const override;

        /**
         * @brief Check if Bluetooth status is available
         */
        bool has_bluetooth() const override;

        /**
         * @brief Check if WiFi status is available
         */
        bool has_wifi() const override;

        /**
         * @brief Check if WiFi is currently powered on
         */
        bool wifi_enabled() const override;

        /**
         * @brief Enable or disable WiFi
         */
        void set_wifi_enabled(bool enable) override;

        /**
         * @brief Scan for available WiFi networks
         */
        std::vector<std::string> scan_wifi_networks() override;

    private:
        /**
         * @brief Collect a fresh system-info snapshot (expensive: shells
         *        out to networksetup/airport/system_profiler on macOS)
         */
        System_Info collect_info();

        /**
         * @brief Read CPU temperature (platform-specific)
         */
        std::optional<float> read_cpu_temp();

        /**
         * @brief Read storage stats for the application directory
         */
        std::optional<System_Info::Storage> read_storage();

        /// Info snapshots are expensive to collect (~400-500ms of
        /// subprocesses on macOS) — serve this TTL from cache.
        static constexpr std::chrono::seconds INFO_CACHE_TTL { 30 };

        mutable std::mutex                               m_cache_mtx;
        mutable System_Info                              m_info_cache;
        mutable std::chrono::steady_clock::time_point    m_info_cached_at {};
        mutable bool                                     m_info_cached { false };
        // Adapter presence is session-static; memoize to skip the
        // system_profiler/networksetup probes after the first call.
        mutable std::optional<bool>                      m_has_wifi_cache;
        mutable std::optional<bool>                      m_has_bt_cache;

        /**
         * @brief Platform detection (set at compile time via defines)
         */
        #if defined(__APPLE__)
            static constexpr bool is_macos { true };
            static constexpr bool is_linux { false };
        #elif defined(__linux__)
            static constexpr bool is_macos { false };
            static constexpr bool is_linux { true };
        #else
            static constexpr bool is_macos { false };
            static constexpr bool is_linux { false };
        #endif
};

} // namespace ovb::hal::sdl
