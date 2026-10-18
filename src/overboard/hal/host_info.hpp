/**
 * @file    host_info.hpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared POSIX host-system queries used by SDL and Pi Zero
 *          system-info implementations.
 *
 * All functions degrade gracefully on hosts without the relevant
 * sysfs/procfs nodes (e.g. macOS): they return std::nullopt/false.
 */
#pragma once

// C++ Standard Libraries
#include <filesystem>
#include <optional>

// Project Libraries
#include <overboard/hal/i_system_info.hpp>

namespace ovb::hal {

/**
 * @brief Check whether a Linux CPU-temperature sysfs node is readable
 *
 * Checks /sys/class/thermal/thermal_zone0/temp and the hwmon fallback.
 */
bool has_host_cpu_temp();

/**
 * @brief Read CPU temperature from Linux sysfs
 *
 * Tries thermal_zone0 then hwmon0/temp1_input; values are
 * millidegrees Celsius.
 *
 * @return Temperature in Celsius, or nullopt when unavailable
 */
std::optional<float> read_host_cpu_temp();

/**
 * @brief Read filesystem stats via statvfs
 *
 * @param root Path on the filesystem to query (e.g. "/", ".")
 * @return Storage totals in bytes, or nullopt on failure
 */
std::optional<System_Info::Storage> read_storage_stats(
    const std::filesystem::path& root );

} // namespace ovb::hal
