/**
 * @file    protocol.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 wire protocol — frame codec, CRC-32, and the tagged-value
 *          binary format used by command payloads and .Theme files.
 *
 * Frame layout (all header fields little-endian):
 *   0    FRAME_HEAD  22 bytes  ASCII "AA551234 FIXEDCMDHEAD " (trailing space)
 *   22   packetType  u32 LE    PACKET_REQUEST=0 / PACKET_REPLY=2
 *   26   commandId   u32 LE    see Command_Id
 *   30   payloadSize u32 LE
 *   34   crc32       u32 LE    CRC-32 of the payload only
 *   38   payload     payloadSize bytes
 *
 * Ported from the Python reference implementation (terminus.mpad.protocol).
 * All functions are non-throwing; decode failures return std::nullopt.
 */
#pragma once

// C++ Standard Libraries
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ovb::mpad {

// ---------------------------------------------------------------------------
// Framing constants
// ---------------------------------------------------------------------------

/// Full 22-byte frame header magic
inline constexpr std::string_view FRAME_HEAD { "AA551234 FIXEDCMDHEAD " };

/// Short magic used to resynchronize the stream
inline constexpr std::string_view MAGIC { "AA551234" };

/// Out-of-band abort message the device tolerates mid-stream
inline constexpr std::string_view ABORT_MSG {
    "AA551234 Abort file transfer 123455AA" };

/// Length of FRAME_HEAD
inline constexpr size_t HEADER_LEN { 22 };

/// Smallest valid frame: header + 4 u32 fields
inline constexpr size_t MIN_FRAME_SIZE { HEADER_LEN + 4 * 4 };

/// Payload chunk size for FILE transfers
inline constexpr size_t BULK_CHUNK_SIZE { 4096 };

/// Packet types
inline constexpr uint32_t PACKET_REQUEST { 0 };
inline constexpr uint32_t PACKET_REPLY   { 2 };

/// Wire command IDs (Host -> Device unless noted)
enum class Command_Id : uint32_t {
    Find_Device               = 0,
    Send_System_Data          = 1,
    Set_Device_Reload         = 2,
    Get_Device_Theme          = 3,
    Set_Device_Bl             = 4,
    Set_Device_Scan_State     = 5,
    File_Start                = 6,
    File_End                  = 7,
    Get_Device_Version        = 8,
    Set_Device_Canvasflip     = 9,
    Get_Device_Screenmessage  = 10,
    Set_Device_Delete_Theme   = 11,
    Send_Pixmap               = 12,
    Proactive_Cmd             = 13,   ///< Device -> Host events
    Request_Upload_Key        = 14,
    Send_Json                 = 15,
};

/// A decoded wire frame
struct Frame {
    uint32_t              command     { 0 };
    std::vector<std::byte> payload;
    uint32_t              packet_type { PACKET_REQUEST };
};

/// Incremental frame parser — feed() raw bytes, get complete frames.
/// Tolerates the out-of-band abort message and resynchronizes on bad
/// length or CRC rather than dropping a following valid frame.
class Frame_Parser {
    public:

        /// Feed raw bytes; returns any complete, CRC-valid frames
        std::vector<Frame> feed( std::span<const std::byte> data );

    private:

        std::vector<std::byte> m_buf;
};

// ---------------------------------------------------------------------------
// CRC and framing
// ---------------------------------------------------------------------------

/**
 * @brief CRC-32 (IEEE, reflected) of a byte span
 *
 * @param data The data to calculate the CRC for
 * @return uint32_t The CRC-32 of the data
 */
uint32_t crc32( std::span<const std::byte> data );

/// View a string's bytes as a std::byte span (no copy)
inline std::span<const std::byte> as_bytes( std::string_view s ) {
    return { reinterpret_cast<const std::byte*>( s.data() ), s.size() };
}

/**
 * @brief Encode a complete frame
 *
 * @param command The command to encode
 * @param payload The payload to encode
 * @param packet_type The packet type to encode
 * @return std::vector<std::byte> The encoded frame
 */
std::vector<std::byte> encode_frame(
    uint32_t command,
    std::span<const std::byte> payload = {},
    uint32_t packet_type = PACKET_REQUEST );

// ---------------------------------------------------------------------------
// Tagged-value binary format (key events, .Theme controlData/header)
// ---------------------------------------------------------------------------

/// Wire type tags
enum class Tagged_Type : uint32_t {
    Bool   = 1,
    Int32  = 2,
    Double = 6,
    Map    = 8,
    List   = 9,
    String = 10,
    Bytes  = 12,
};

struct Value;                              ///< Recursive tagged value
using Blob = std::vector<std::byte>;       ///< Raw byte blob
using Map  = std::map<std::string, Value>; ///< Keyed value map
using List = std::vector<Value>;           ///< Value array

/// Base type for Value — an alias so Value can name Map/List recursively
using Value_Base = std::variant<std::monostate, bool, int32_t, double,
                                std::string, Blob, Map, List>;

/**
 * @brief A tagged value — recursive variant matching Tagged_Type.
 *
 * std::monostate represents the wire "null" encoding. Derived from the
 * variant so std::get_if/std::holds_alternative deduce through the base.
 */
struct Value : Value_Base {
    using Value_Base::Value_Base;

    /// string literals must land on std::string, not bool
    Value( const char* s ) : Value_Base( std::string( s ) ) {}
};

/// Encode a tagged map (count + repeated key/value pairs)
std::vector<std::byte> encode_tagged_map( const Map& d );

/// Decode one tagged map starting at offset; nullopt on malformed input
std::optional<Map> decode_tagged_map( std::span<const std::byte> buf,
                                      size_t& offset );

/// Decode a top-level tagged array of maps (event payloads)
std::optional<std::vector<Map>> decode_tagged_array(
    std::span<const std::byte> buf );

// ---------------------------------------------------------------------------
// Simple string map (device info, theme list, telemetry payloads)
// ---------------------------------------------------------------------------

/// Encode map<string,string> — BE count + UTF-16BE key/value pairs
std::vector<std::byte> encode_string_map(
    const std::map<std::string, std::string>& d );

/// Decode a string map; nullopt on malformed input
std::optional<std::map<std::string, std::string>> decode_string_map(
    std::span<const std::byte> buf );

// ---------------------------------------------------------------------------
// String conversion
// ---------------------------------------------------------------------------

/// UTF-8 -> UTF-16BE bytes (no BOM); stops on truncated sequences
std::vector<std::byte> utf8_to_utf16be( std::string_view s );

/// UTF-16BE bytes -> UTF-8 (surrogate pairs joined)
std::string utf16be_to_utf8( std::span<const std::byte> data );

} // namespace ovb::mpad
