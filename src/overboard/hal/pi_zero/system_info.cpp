/**
 * @file    system_info.cpp
 * @author  Marvin Smith
 * @date    2026-06-10
 *
 * @brief   Pi Zero platform system information implementation
 */
#include "system_info.hpp"

// C++ Standard Libraries
#include <unistd.h>

#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/hal/host_info.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::pi_zero {

/****************************/
/*     Constructor          */
/****************************/
PiZero_System_Info::PiZero_System_Info() {
    LOG_DEBUG("PiZero_System_Info created (uninitialized)");
}

/****************************/
/*     Initialize           */
/****************************/
bool PiZero_System_Info::init() {
    // Probe for optional hardware
    m_has_battery = (access("/sys/class/power_supply/BAT0", F_OK) == 0) ||
                    (access("/sys/class/power_supply/battery", F_OK) == 0);

    m_has_bt = (access("/sys/class/bluetooth", F_OK) == 0) ||
               (system("which hciconfig > /dev/null 2>&1") == 0);

    LOG_DEBUG("PiZero_System_Info::init() - battery=", m_has_battery,
              ", bt=", m_has_bt);

    m_initialized = true;
    return true;
}

/****************************/
/*     Get System Info      */
/****************************/
System_Info PiZero_System_Info::get_info() {
    System_Info info;

    if (!m_initialized) {
        LOG_WARN("get_info() called before init()");
        return info;
    }

    info.cpu_temp_c = read_host_cpu_temp();

    auto batt = read_battery();
    if (batt) {
        info.battery_voltage_v = batt->voltage_v;
        info.battery_percent   = batt->percent;
    }

    info.storage    = read_storage_stats("/");
    info.usb        = read_usb();
    info.bluetooth  = read_bluetooth();
    info.wifi       = read_wifi();

    return info;
}

/****************************/
/*     Capability Checks    */
/****************************/
bool PiZero_System_Info::has_cpu_temp() const {
    return has_host_cpu_temp();
}

/************************************/
/*     Check if Battery Present     */
/************************************/
bool PiZero_System_Info::has_battery() const {
    return m_has_battery;
}

/************************************/
/*     Check if Storage Present     */
/************************************/
bool PiZero_System_Info::has_storage() const {
    return true;  // Always has SD card
}

/************************************/
/*      Check if USB Present        */
/************************************/
bool PiZero_System_Info::has_usb() const {
    return true;  // Always has USB (gadget mode)
}

/************************************/
/*     Check if Bluetooth Present   */
/************************************/
bool PiZero_System_Info::has_bluetooth() const {
    return m_has_bt;
}

/************************************/
/*       Check if WiFi Present      */
/************************************/
bool PiZero_System_Info::has_wifi() const {
    return access("/sys/class/net/wlan0", F_OK) == 0;
}

/************************************/
/*         WiFi Enabled             */
/************************************/
bool PiZero_System_Info::wifi_enabled() const {
    std::ifstream operstate("/sys/class/net/wlan0/operstate");
    if (!operstate.is_open()) {
        return false;
    }
    std::string state;
    operstate >> state;
    return (state == "up" || state == "dormant");
}

/************************************/
/*        Set WiFi Enabled          */
/************************************/
void PiZero_System_Info::set_wifi_enabled(bool enable) {
    const std::string cmd = std::string("ip link set wlan0 ")
                          + (enable ? "up" : "down") + " 2>/dev/null";
    FILE* p = popen(cmd.c_str(), "r");
    if (p) pclose(p);
}

/************************************/
/*       Scan WiFi Networks         */
/************************************/
std::vector<std::string> PiZero_System_Info::scan_wifi_networks() {
    std::vector<std::string> networks;
    FILE* pipe = popen("iwlist wlan0 scan 2>/dev/null | grep 'ESSID' | sed 's/.*ESSID:\"\\(.*\\)\"/\\1/'", "r");
    if (!pipe) {
        return networks;
    }
    std::array<char, 256> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        std::string ssid(buffer.data());
        while (!ssid.empty() && (ssid.back() == '\n' || ssid.back() == '\r')) {
            ssid.pop_back();
        }
        if (!ssid.empty()) {
            networks.push_back(ssid);
        }
    }
    pclose(pipe);
    return networks;
}

/****************************/
/*     Read Battery         */
/****************************/
std::optional<PiZero_System_Info::Battery_Info> PiZero_System_Info::read_battery() {
    if (!m_has_battery) {
        return std::nullopt;
    }

    // Try common battery paths
    const char* paths[] = {
        "/sys/class/power_supply/BAT0",
        "/sys/class/power_supply/battery"
    };

    for (const auto* path : paths) {
        if (access(path, F_OK) != 0) continue;

        Battery_Info info;

        // Read voltage (usually in microvolts)
        std::ifstream volt_file(std::string(path) + "/voltage_now");
        if (volt_file.is_open()) {
            int voltage_uv;
            volt_file >> voltage_uv;
            if (volt_file.good()) {
                info.voltage_v = static_cast<float>(voltage_uv) / 1000000.0f;
            }
        }

        // Read capacity percentage
        std::ifstream cap_file(std::string(path) + "/capacity");
        if (cap_file.is_open()) {
            cap_file >> info.percent;
        }

        return info;
    }

    return std::nullopt;
}

/****************************/
/*     Read USB Status      */
/****************************/
std::optional<System_Info::USB_Status> PiZero_System_Info::read_usb() {
    // Pi Zero is typically in USB gadget mode
    // Could check /sys/kernel/debug/usb/devices or dwc2 driver status
    // For now, report connected=true, host_mode=false (gadget)
    System_Info::USB_Status status;
    status.connected = true;   // TODO: Detect actual USB connection
    status.host_mode = false;  // Pi Zero is typically gadget mode
    return status;
}

/****************************/
/*       Read WiFi          */
/****************************/
std::optional<System_Info::WiFi_Status> PiZero_System_Info::read_wifi() {
    const char* const operstate_path = "/sys/class/net/wlan0/operstate";
    if (access(operstate_path, R_OK) != 0) {
        return std::nullopt;
    }

    System_Info::WiFi_Status status;
    status.signal_dbm = 0;

    // Check interface operational state
    std::ifstream operstate(operstate_path);
    std::string state;
    operstate >> state;
    status.connected = (state == "up");

    // Read SSID from /proc/net/wireless SSID column via iwgetid output file
    // Fallback: check /sys/class/net/wlan0/wireless/link for signal
    if (status.connected) {
        // Try to read SSID from sysfs phy80211 or /proc/net/wireless
        std::ifstream wireless("/proc/net/wireless");
        std::string line;
        std::getline(wireless, line); // header line 1
        std::getline(wireless, line); // header line 2
        if (std::getline(wireless, line)) {
            // Format: "wlan0: status link level noise ..."
            std::istringstream iss(line);
            std::string iface;
            int link_quality;
            int signal_dbm_raw;
            int noise;
            iss >> iface >> link_quality >> signal_dbm_raw >> noise;
            if (iss) {
                status.signal_dbm = signal_dbm_raw;
            }
        }
    }

    return status;
}

/****************************/
/*     Read Bluetooth       */
/****************************/
std::optional<System_Info::BT_Status> PiZero_System_Info::read_bluetooth() {
    if (!m_has_bt) {
        return std::nullopt;
    }

    // TODO: Use hciconfig or bluetoothctl to get actual status
    System_Info::BT_Status status;
    status.powered   = true;   // TODO: Detect actual power state
    status.connected = false;  // TODO: Check for active connections
    return status;
}

} // namespace ovb::hal::pi_zero
