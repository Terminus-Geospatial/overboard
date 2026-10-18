/**
 * @file    host_info.cpp
 * @author  Marvin Smith
 * @date    2026-10-17
 *
 * @brief   Shared POSIX host-system query implementation.
 */
#include <overboard/hal/host_info.hpp>

// C++ Standard Libraries
#include <sys/statvfs.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstring>

// Project Libraries
#include <overboard/io/file_utils.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::hal {

namespace {
    /// @brief Sysfs paths that expose CPU temperature in millidegrees
    constexpr const char* CPU_TEMP_PATHS[] {
        "/sys/class/thermal/thermal_zone0/temp",
        "/sys/class/hwmon/hwmon0/temp1_input",
    };
}

/****************************/
/*     Has CPU Temp         */
/****************************/
bool has_host_cpu_temp() {
    for (const auto* path : CPU_TEMP_PATHS) {
        if (access(path, R_OK) == 0) {
            return true;
        }
    }
    return false;
}

/****************************/
/*     Read CPU Temp        */
/****************************/
std::optional<float> read_host_cpu_temp() {
    for (const auto* path : CPU_TEMP_PATHS) {
        const auto text = io::read_file_text(path);
        if (!text) {
            continue;
        }

        int temp_millidegrees = 0;
        const auto* end = text->data() + text->size();
        if (std::from_chars(text->data(), end, temp_millidegrees).ec == std::errc{}) {
            return static_cast<float>(temp_millidegrees) / 1000.0f;
        }
    }
    return std::nullopt;
}

/****************************/
/*     Read Storage         */
/****************************/
std::optional<System_Info::Storage> read_storage_stats(
    const std::filesystem::path& root ) {
    struct statvfs stat;
    if (statvfs(root.c_str(), &stat) != 0) {
        LOG_WARN("Failed to get storage stats for ", root.string(),
                 ": ", std::strerror(errno));
        return std::nullopt;
    }

    System_Info::Storage storage;
    storage.total_bytes = stat.f_blocks * stat.f_frsize;
    storage.free_bytes  = stat.f_bfree * stat.f_frsize;
    storage.used_bytes  = storage.total_bytes - storage.free_bytes;
    return storage;
}

} // namespace ovb::hal
