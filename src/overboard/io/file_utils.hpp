/**
 * @file    file_utils.hpp
 * @author  Marvin Smith
 * @date    2026-10-21
 *
 * @brief   Whole-file readers shared by config and asset loading.
 */
#pragma once

// C++ Standard Libraries
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace ovb::io {

/**
 * @brief Read an entire file as raw bytes
 * @param path File to read
 * @return Contents, or empty vector on open/read failure
 */
std::vector<std::byte> read_file_bytes( const std::filesystem::path& path );

/**
 * @brief Read an entire file as text
 * @param path File to read
 * @return Contents, or nullopt on open/read failure
 */
std::optional<std::string> read_file_text( const std::filesystem::path& path );

} // namespace ovb::io
