/**
 * @file    system_info.cpp
 * @author  Marvin Smith
 * @date    2026-06-10
 *
 * @brief   PicoCalc platform system information implementation
 */
#include "system_info.hpp"

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal::picocalc {

/****************************/
/*     Constructor          */
/****************************/
PicoCalc_System_Info::PicoCalc_System_Info() {
    LOG_DEBUG("PicoCalc_System_Info created (uninitialized)");
}

/****************************/
/*     Initialize           */
/****************************/
bool PicoCalc_System_Info::init() {
    // TODO: Initialize I2C for MAX17048
    // TODO: Check SD card mount status
    // TODO: Verify temperature sensor ADC channel

    LOG_DEBUG("PicoCalc_System_Info::init() - stub implementation");
    m_initialized = true;
    return true;
}

/****************************/
/*     Get System Info      */
/****************************/
System_Info PicoCalc_System_Info::get_info() {
    System_Info info;

    if (!m_initialized) {
        LOG_WARN("get_info() called before init()");
        return info;
    }

    info.cpu_temp_c      = read_cpu_temp();
    auto batt            = read_battery();
    if (batt) {
        info.battery_voltage_v = batt->voltage_v;
        info.battery_percent   = batt->percent;
    }
    info.storage         = read_storage();
    info.usb             = read_usb();
    // TODO: Bluetooth if module present
    info.bluetooth       = std::nullopt;

    return info;
}

/****************************/
/*     Capability Checks    */
/****************************/
bool PicoCalc_System_Info::has_cpu_temp() const {
    return m_initialized;  // RP2350 always has temp sensor
}

/***************************************************/
/*          Check if Battery is available          */
/***************************************************/
bool PicoCalc_System_Info::has_battery() const {
    return m_initialized;  // MAX17048 should be present on PicoCalc
}

/***************************************************/
/*          Check if Storage is available          */
/***************************************************/
bool PicoCalc_System_Info::has_storage() const {
    return m_initialized;  // SD card slot always present
}

/***************************************************/
/*          Check if USB is available              */
/***************************************************/
bool PicoCalc_System_Info::has_usb() const {
    return m_initialized;  // USB always present
}

/***************************************************/
/*          Check if Bluetooth is available        */
/***************************************************/
bool PicoCalc_System_Info::has_bluetooth() const {
    // TODO: Detect if BT module installed
    return false;
}

/*************************************************/
/*          Check if Wifi is available           */
/*************************************************/
bool PicoCalc_System_Info::has_wifi() const {
    return false;  // PicoCalc has no WiFi hardware
}

/****************************/
/*     Read CPU Temp        */
/****************************/
std::optional<float> PicoCalc_System_Info::read_cpu_temp() {
    // TODO: Read RP2350 internal temperature sensor via ADC
    // ADC channel 4 is the temperature sensor
    // Formula: T = 27 - (ADC_voltage - 0.706) / 0.001721
    LOG_DEBUG("read_cpu_temp() - stub");
    return std::nullopt;
}

/****************************/
/*     Read Battery         */
/****************************/
std::optional<PicoCalc_System_Info::Battery_Info> PicoCalc_System_Info::read_battery() {
    // TODO: Read MAX17048 via I2C
    // Register 0x02: VCELL (voltage)
    // Register 0x04: SOC (state of charge)
    LOG_DEBUG("read_battery() - stub");
    return std::nullopt;
}

/****************************/
/*     Read Storage         */
/****************************/
std::optional<System_Info::Storage> PicoCalc_System_Info::read_storage() {
    // TODO: Use f_stat or similar to get SD card FAT32 filesystem stats
    LOG_DEBUG("read_storage() - stub");
    return std::nullopt;
}

/****************************/
/*     Read USB Status      */
/****************************/
std::optional<System_Info::USB_Status> PicoCalc_System_Info::read_usb() {
    // TODO: Query TinyUSB for host/device mode and connection status
    LOG_DEBUG("read_usb() - stub");
    return std::nullopt;
}

} // namespace ovb::hal::picocalc
