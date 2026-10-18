/**
 * @file    file_utils.cpp
 * @author  Marvin Smith
 * @date    2026-10-21
 *
 * @brief   Whole-file readers shared by config and asset loading.
 */
#include <overboard/io/file_utils.hpp>

// C++ Standard Libraries
#include <fstream>

namespace ovb::io {

/***************************/
/*     Read file bytes     */
/***************************/
std::vector<std::byte> read_file_bytes( const std::filesystem::path& path ) {
    std::ifstream f( path, std::ios::binary | std::ios::ate );
    if (!f) {
        return {};
    }
    const auto size = f.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<std::byte> out( static_cast<size_t>( size ) );
    f.seekg( 0 );
    f.read( reinterpret_cast<char*>( out.data() ), size );
    return out;
}

/***************************/
/*     Read file text      */
/***************************/
std::optional<std::string> read_file_text( const std::filesystem::path& path ) {
    std::ifstream f( path );
    if (!f.is_open()) {
        return std::nullopt;
    }
    return std::string{ std::istreambuf_iterator<char>( f ),
                        std::istreambuf_iterator<char>() };
}

} // namespace ovb::io
