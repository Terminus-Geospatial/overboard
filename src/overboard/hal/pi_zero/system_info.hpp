/**
 * @file    system_info.hpp
 * @author  Marvin Smith
 * @date    2026-06-10
 *
 * @brief   Pi Zero platform system information implementation
 *
 * Pi Zero uses standard Linux interfaces:
 * - /sys/class/thermal/thermal_zone0/temp for CPU temp
 * - /sys/class/power_supply/ for battery (if LiPo HAT present)
 * - Standard statvfs for storage
 * - /sys/kernel/debug/usb/devices or similar for USB
 * - hciconfig or bluetoothctl for BT
 */
#pragma once

// C++ Standard Libraries
#include <string>
#include <vector>

// Project Libraries
#include <overboard/hal/i_system_info.hpp>

namespace ovb::hal::pi_zero {

/**
 * @brief Pi Zero system info implementation
 *
 * Uses standard Linux sysfs interfaces for hardware queries.
 * Some features depend on HATs or external hardware.
 */
class PiZero_System_Info : public I_System_Info {
    public:
        /**
         * @brief Constructor
         */
        PiZero_System_Info();

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

        /**
         * @brief Initialize - probe available sensors
         */
        bool init();

    private:
        /**
         * @brief Read from power_supply sysfs (if battery HAT attached)
         */
        struct Battery_Info {
            float voltage_v;
            int   percent;
        };
        std::optional<Battery_Info> read_battery();

        /**
         * @brief Read USB gadget mode status
         */
        std::optional<System_Info::USB_Status> read_usb();

        /**
         * @brief Read Bluetooth status via hciconfig or bluetoothctl
         */
        std::optional<System_Info::BT_Status> read_bluetooth();

        /**
         * @brief Read WiFi status via sysfs operstate and iwgetid
         */
        std::optional<System_Info::WiFi_Status> read_wifi();

        /// @brief Whether the system info has been initialized
        bool m_initialized { false };
        /// @brief Whether a battery is detected
        bool m_has_battery { false };  // Detected at init
        /// @brief Whether Bluetooth is detected
        bool m_has_bt      { false };  // Detected at init
};

} // namespace ovb::hal::pi_zero
