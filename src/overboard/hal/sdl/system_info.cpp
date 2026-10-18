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
#include <vector>

// Project Libraries
#include <overboard/hal/host_info.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::sdl {

/****************************/
/*     Constructor          */
/****************************/
SDL_System_Info::SDL_System_Info() {
    LOG_DEBUG("SDL_System_Info initialized (macOS=", std::to_string(is_macos), ")");
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
    info.cpu_temp_c = read_host_cpu_temp();

    // Battery - not available on typical dev machines
    // Could parse `pmset -g batt` on macOS, but it's slow and requires shelling out
    info.battery_voltage_v = std::nullopt;
    info.battery_percent   = std::nullopt;

    // Storage
    info.storage = read_storage_stats(".");

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
    return has_host_cpu_temp();
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

} // namespace ovb::hal::sdl
