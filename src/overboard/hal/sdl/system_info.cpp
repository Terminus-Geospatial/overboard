/**
 * @file    system_info.cpp
 * @author  Marvin Smith
 * @date    2026-06-10
 *
 * @brief   SDL platform system information implementation
 */
#include "system_info.hpp"
#include "os_utils.hpp"

// C++ Standard Libraries
#include <sys/statvfs.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <vector>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::sdl {

/****************************/
/*     Constructor          */
/****************************/
SDL_System_Info::SDL_System_Info() {
    LOG_DEBUG("SDL_System_Info initialized (macOS=", std::to_string(is_macos),
              ", Linux=", std::to_string(is_linux), ")");
}

/****************************/
/*     Get System Info      */
/****************************/
System_Info SDL_System_Info::get_info() {
    std::lock_guard lk( m_cache_mtx );
    if (m_info_cached &&
        std::chrono::steady_clock::now() - m_info_cached_at < INFO_CACHE_TTL) {
        return m_info_cache;
    }
    m_info_cache     = collect_info();
    m_info_cached_at = std::chrono::steady_clock::now();
    m_info_cached    = true;
    return m_info_cache;
}

/****************************/
/*      Collect Info        */
/****************************/
System_Info SDL_System_Info::collect_info() {
    System_Info info;

    // CPU temperature
    info.cpu_temp_c = read_cpu_temp();

    // Battery - not available on typical dev machines
    // Could parse `pmset -g batt` on macOS, but it's slow and requires shelling out
    info.battery_voltage_v = std::nullopt;
    info.battery_percent   = std::nullopt;

    // Storage
    info.storage = read_storage();

    // USB - not meaningfully queryable in SDL context
    info.usb = std::nullopt;

    // Bluetooth — m_cache_mtx is already held by get_info(); reuse the
    // memoized capability probes rather than re-running system_profiler.
    if (!m_has_bt_cache) {
        m_has_bt_cache = ovb::hal::sdl::has_bluetooth();
    }
    if (*m_has_bt_cache) {
        System_Info::BT_Status bt;
        bt.powered = true;
        bt.connected = get_bluetooth_connected();
        info.bluetooth = bt;
    } else {
        info.bluetooth = std::nullopt;
    }

    // WiFi - same memoized capability probe
    if (!m_has_wifi_cache) {
        m_has_wifi_cache = ovb::hal::sdl::has_wifi();
    }
    if (*m_has_wifi_cache) {
        auto wifi_status = get_wifi_status();
        if (wifi_status) {
            System_Info::WiFi_Status wifi;
            wifi.connected = wifi_status->connected;
            wifi.ssid = wifi_status->ssid;
            wifi.signal_dbm = wifi_status->signal_strength;  // Convert percentage to dBm approximation
            info.wifi = wifi;
        } else {
            info.wifi = std::nullopt;
        }
    } else {
        info.wifi = std::nullopt;
    }

    return info;
}

/****************************/
/*     Capability Checks    */
/****************************/
bool SDL_System_Info::has_cpu_temp() const {
    // On Linux, check if thermal zone exists
    if (is_linux) {
        return access("/sys/class/thermal/thermal_zone0/temp", R_OK) == 0;
    }
    // On macOS, CPU temp is difficult to query reliably without SMC
    // We'll return false and rely on mock/powermetrics if needed
    return false;
}

/************************************************/
/*          Check if Battery is available       */
/************************************************/
bool SDL_System_Info::has_battery() const {
    // Most development machines don't expose battery in a standard way
    // macOS laptops have battery but querying requires IOKit
    return false;
}

/************************************************/
/*          Check if Storage is available       */
/************************************************/
bool SDL_System_Info::has_storage() const {
    return true;  // statvfs works everywhere
}

/************************************************/
/*          Check if USB is available           */
/************************************************/
bool SDL_System_Info::has_usb() const {
    return false;  // Not queryable in SDL context
}

/************************************************/
/*          Check if Bluetooth is available     */
/************************************************/
bool SDL_System_Info::has_bluetooth() const {
    std::lock_guard lk( m_cache_mtx );
    if (!m_has_bt_cache) {
        m_has_bt_cache = ovb::hal::sdl::has_bluetooth();
    }
    return *m_has_bt_cache;
}

/************************************************/
/*          Check if Wifi is available          */
/************************************************/
bool SDL_System_Info::has_wifi() const {
    std::lock_guard lk( m_cache_mtx );
    if (!m_has_wifi_cache) {
        m_has_wifi_cache = ovb::hal::sdl::has_wifi();
    }
    return *m_has_wifi_cache;
}

/*********************************************/
/*          Check if Wifi is Enabled         */
/*********************************************/
bool SDL_System_Info::wifi_enabled() const {
    return ovb::hal::sdl::wifi_enabled();
}

/*********************************************/
/*            Set Wifi Enabled               */
/*********************************************/
void SDL_System_Info::set_wifi_enabled(bool enable) {
    ovb::hal::sdl::set_wifi_enabled(enable);
}

/*********************************************/
/*          Scan WiFi Networks               */
/*********************************************/
std::vector<std::string> SDL_System_Info::scan_wifi_networks() {
    return ovb::hal::sdl::scan_wifi_networks();
}

/****************************/
/*     Read CPU Temp        */
/****************************/
std::optional<float> SDL_System_Info::read_cpu_temp() {
    if (is_linux) {
        // Try standard Linux thermal zone
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        if (temp_file.is_open()) {
            int temp_millidegrees;
            temp_file >> temp_millidegrees;
            if (temp_file.good()) {
                return static_cast<float>(temp_millidegrees) / 1000.0f;  // Convert to Celsius
            }
        }

        // Try alternative location (some systems)
        temp_file.open("/sys/class/hwmon/hwmon0/temp1_input");
        if (temp_file.is_open()) {
            int temp_millidegrees;
            temp_file >> temp_millidegrees;
            if (temp_file.good()) {
                return static_cast<float>(temp_millidegrees) / 1000.0f;
            }
        }
    }

    #if defined(__APPLE__)
    // On macOS, we could shell out to `powermetrics` but it's slow and requires sudo
    // For now, return nullopt - the UI can show "N/A" or use a mock value
    #endif

    return std::nullopt;
}

/****************************/
/*     Read Storage         */
/****************************/
std::optional<System_Info::Storage> SDL_System_Info::read_storage() {
    struct statvfs stat;

    // Get current working directory filesystem stats
    if (statvfs(".", &stat) != 0) {
        LOG_WARN("SDL_System_Info: failed to get storage stats: ", std::strerror(errno));
        return std::nullopt;
    }

    System_Info::Storage storage;
    storage.total_bytes = stat.f_blocks * stat.f_frsize;
    storage.free_bytes  = stat.f_bfree * stat.f_frsize;
    storage.used_bytes  = storage.total_bytes - storage.free_bytes;

    return storage;
}

} // namespace ovb::hal::sdl
