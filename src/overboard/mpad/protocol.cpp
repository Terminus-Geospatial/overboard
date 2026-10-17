/**
 * @file    protocol.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 wire protocol implementation.
 */
#include <overboard/mpad/protocol.hpp>

// C++ Standard Libraries
#include <algorithm>
#include <array>
#include <cstring>

namespace ovb::mpad {

namespace {

/***************************/
/*      CRC-32 table       */
/***************************/
const std::array<uint32_t, 256>& crc_table() {
    static const auto table = [] {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();
    return table;
}

/***************************/
/*   LE/BE pack helpers    */
/***************************/
void put_u32le( std::vector<std::byte>& out, uint32_t v ) {
    for (int i = 0; i < 4; ++i) {
        out.push_back( static_cast<std::byte>( (v >> (i * 8)) & 0xFF ) );
    }
}

void put_u32be( std::vector<std::byte>& out, uint32_t v ) {
    for (int i = 3; i >= 0; --i) {
        out.push_back( static_cast<std::byte>( (v >> (i * 8)) & 0xFF ) );
    }
}

uint32_t get_u32le( const std::byte* p ) {
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

uint32_t get_u32be( const std::byte* p ) {
    return (static_cast<uint32_t>(p[0]) << 24)
         | (static_cast<uint32_t>(p[1]) << 16)
         | (static_cast<uint32_t>(p[2]) << 8)
         | static_cast<uint32_t>(p[3]);
}

int32_t get_i32be( const std::byte* p ) {
    return static_cast<int32_t>( get_u32be( p ) );
}

void append_bytes( std::vector<std::byte>& out, std::span<const std::byte> b ) {
    out.insert( out.end(), b.begin(), b.end() );
}

/// Length-prefixed UTF-16BE string (u32 BE length + data)
void put_str( std::vector<std::byte>& out, std::string_view s ) {
    auto b = utf8_to_utf16be( s );
    put_u32be( out, static_cast<uint32_t>( b.size() ) );
    append_bytes( out, b );
}

/// Reads a length-prefixed UTF-16BE string. 0xFFFFFFFF = null.
std::optional<std::string> get_str( std::span<const std::byte> buf,
                                    size_t& offset ) {
    if ( offset + 4 > buf.size() ) {
        return std::nullopt;
    }
    const uint32_t n = get_u32be( buf.data() + offset );
    offset += 4;
    if ( n == 0xFFFFFFFF ) {
        return std::string {};
    }
    if ( offset + n > buf.size() ) {
        return std::nullopt;
    }
    std::string s = utf16be_to_utf8( buf.subspan( offset, n ) );
    offset += n;
    return s;
}

} // anonymous namespace

/***************************/
/*        CRC-32           */
/***************************/
uint32_t crc32( std::span<const std::byte> data ) {
    const auto& t = crc_table();
    uint32_t crc = 0xFFFFFFFF;
    for (const std::byte b : data) {
        crc = t[(crc ^ static_cast<uint8_t>( b )) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

/***************************/
/*      Encode frame       */
/***************************/
std::vector<std::byte> encode_frame( uint32_t command,
                                     std::span<const std::byte> payload,
                                     uint32_t packet_type ) {
    std::vector<std::byte> out;
    out.reserve( MIN_FRAME_SIZE + payload.size() );
    for (char c : FRAME_HEAD) {
        out.push_back( static_cast<std::byte>( c ) );
    }
    put_u32le( out, packet_type );
    put_u32le( out, command );
    put_u32le( out, static_cast<uint32_t>( payload.size() ) );
    put_u32le( out, crc32( payload ) );
    append_bytes( out, payload );
    return out;
}

/***************************/
/*      Frame parser       */
/***************************/
std::vector<Frame> Frame_Parser::feed( std::span<const std::byte> data ) {
    m_buf.insert( m_buf.end(), data.begin(), data.end() );
    std::vector<Frame> frames;

    while (true) {
        // Find next magic occurrence
        const auto* magic = reinterpret_cast<const std::byte*>( MAGIC.data() );
        auto it = std::search( m_buf.begin(), m_buf.end(),
                               magic, magic + MAGIC.size() );
        if ( it == m_buf.end() ) {
            // Keep a trailing partial magic across chunk boundaries
            const size_t keep = std::min( m_buf.size(), MAGIC.size() - 1 );
            m_buf.erase( m_buf.begin(), m_buf.end() - static_cast<long>( keep ) );
            break;
        }
        if ( it != m_buf.begin() ) {
            m_buf.erase( m_buf.begin(), it );
        }
        if ( m_buf.size() < MIN_FRAME_SIZE ) {
            break;
        }

        // Out-of-band abort message — skip it entirely
        if ( m_buf.size() >= ABORT_MSG.size() &&
             std::memcmp( m_buf.data(), ABORT_MSG.data(), ABORT_MSG.size() ) == 0 ) {
            m_buf.erase( m_buf.begin(), m_buf.begin() + ABORT_MSG.size() );
            continue;
        }

        // Not a real frame head — skip this magic occurrence
        if ( std::memcmp( m_buf.data(), FRAME_HEAD.data(), HEADER_LEN ) != 0 ) {
            m_buf.erase( m_buf.begin(), m_buf.begin() + MAGIC.size() );
            continue;
        }

        const uint32_t ptype = get_u32le( m_buf.data() + HEADER_LEN );
        const uint32_t cmd   = get_u32le( m_buf.data() + HEADER_LEN + 4 );
        const uint32_t size  = get_u32le( m_buf.data() + HEADER_LEN + 8 );
        const uint32_t crc   = get_u32le( m_buf.data() + HEADER_LEN + 12 );
        const size_t   end   = MIN_FRAME_SIZE + size;
        if ( m_buf.size() < end ) {
            break;
        }

        std::vector<std::byte> payload(
            m_buf.begin() + static_cast<long>( MIN_FRAME_SIZE ),
            m_buf.begin() + static_cast<long>( end ) );
        if ( crc32( payload ) == crc ) {
            frames.push_back( Frame{ cmd, std::move( payload ), ptype } );
        }
        m_buf.erase( m_buf.begin(), m_buf.begin() + static_cast<long>( end ) );
    }
    return frames;
}

/***************************/
/*   Encode string map     */
/***************************/
std::vector<std::byte> encode_string_map(
    const std::map<std::string, std::string>& d ) {
    std::vector<std::byte> out;
    put_u32be( out, static_cast<uint32_t>( d.size() ) );
    for (const auto& [k, v] : d) {
        put_str( out, k );
        put_str( out, v );
    }
    return out;
}

/***************************/
/*   Decode string map     */
/***************************/
std::optional<std::map<std::string, std::string>> decode_string_map(
    std::span<const std::byte> buf ) {
    if ( buf.size() < 4 ) {
        return std::nullopt;
    }
    const uint32_t count = get_u32be( buf.data() );
    if ( count > 10000 ) {
        return std::nullopt;
    }
    size_t off = 4;
    std::map<std::string, std::string> out;
    for (uint32_t i = 0; i < count; ++i) {
        auto k = get_str( buf, off );
        auto v = get_str( buf, off );
        if ( !k || !v ) {
            return std::nullopt;
        }
        out[std::move( *k )] = std::move( *v );
    }
    return out;
}

namespace {

/***************************/
/* Decode a tagged value   */
/***************************/
std::optional<Value> decode_tagged_value( std::span<const std::byte> buf,
                                          size_t& offset ) {
    if ( offset + 5 > buf.size() ) {
        return std::nullopt;
    }
    const uint32_t type_id = get_u32be( buf.data() + offset );
    const bool     is_null = buf[offset + 4] != std::byte{ 0 };
    offset += 5;

    switch (static_cast<Tagged_Type>( type_id )) {
    case Tagged_Type::Bool: {
        if ( offset + 1 > buf.size() ) return std::nullopt;
        const bool v = buf[offset] != std::byte{ 0 };
        offset += 1;
        if ( is_null ) return Value{ std::monostate{} };
        return Value{ v };
    }
    case Tagged_Type::Int32: {
        if ( offset + 4 > buf.size() ) return std::nullopt;
        const int32_t v = get_i32be( buf.data() + offset );
        offset += 4;
        if ( is_null ) return Value{ std::monostate{} };
        return Value{ v };
    }
    case Tagged_Type::Double: {
        if ( offset + 8 > buf.size() ) return std::nullopt;
        uint64_t bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits = (bits << 8) | static_cast<uint64_t>(
                buf[offset + static_cast<size_t>( i )] );
        }
        double v;
        std::memcpy( &v, &bits, 8 );
        offset += 8;
        if ( is_null ) return Value{ std::monostate{} };
        return Value{ v };
    }
    case Tagged_Type::Map: {
        auto m = decode_tagged_map( buf, offset );
        if ( !m ) return std::nullopt;
        return Value{ std::move( *m ) };
    }
    case Tagged_Type::List: {
        if ( offset + 4 > buf.size() ) return std::nullopt;
        const uint32_t count = get_u32be( buf.data() + offset );
        offset += 4;
        List items;
        items.reserve( count );
        for (uint32_t i = 0; i < count; ++i) {
            auto v = decode_tagged_value( buf, offset );
            if ( !v ) return std::nullopt;
            items.push_back( std::move( *v ) );
        }
        return Value{ std::move( items ) };
    }
    case Tagged_Type::String: {
        if ( offset + 4 > buf.size() ) return std::nullopt;
        const uint32_t n = get_u32be( buf.data() + offset );
        offset += 4;
        if ( n == 0xFFFFFFFF ) return Value{ std::monostate{} };
        if ( offset + n > buf.size() ) return std::nullopt;
        std::string s = utf16be_to_utf8( buf.subspan( offset, n ) );
        offset += n;
        if ( is_null ) return Value{ std::monostate{} };
        return Value{ std::move( s ) };
    }
    case Tagged_Type::Bytes: {
        if ( offset + 4 > buf.size() ) return std::nullopt;
        const uint32_t n = get_u32be( buf.data() + offset );
        offset += 4;
        if ( n == 0xFFFFFFFF ) return Value{ std::monostate{} };
        if ( offset + n > buf.size() ) return std::nullopt;
        Blob b( buf.begin() + static_cast<long>( offset ),
                buf.begin() + static_cast<long>( offset + n ) );
        offset += n;
        if ( is_null ) return Value{ std::monostate{} };
        return Value{ std::move( b ) };
    }
    }
    return std::nullopt;
}

void encode_tagged_value( std::vector<std::byte>& out, const Value& v );

void encode_tagged_map_into( std::vector<std::byte>& out, const Map& d ) {
    put_u32be( out, static_cast<uint32_t>( d.size() ) );
    for (const auto& [k, v] : d) {
        put_str( out, k );
        encode_tagged_value( out, v );
    }
}

/***************************/
/* Encode a tagged value   */
/***************************/
void encode_tagged_value( std::vector<std::byte>& out, const Value& v ) {
    // Every tagged value on the wire is:
    //   u32BE type_id | u8 is_null | <type-specific payload>
    // The is_null flag byte is 0 for all non-null values below.
    std::visit( [&out]( const auto& item ) {
        using T = std::decay_t<decltype( item )>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            // Null: encoded as a Bytes tag with a 0xFFFFFFFF (null) length.
            // The flag byte stays 0 — the length field carries nullness.
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Bytes ) );
            out.push_back( std::byte{ 0 } );
            put_u32be( out, 0xFFFFFFFF );
        } else if constexpr (std::is_same_v<T, bool>) {
            // Bool: single u8 payload, 0 or 1
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Bool ) );
            out.push_back( std::byte{ 0 } );
            out.push_back( static_cast<std::byte>( item ? 1 : 0 ) );
        } else if constexpr (std::is_same_v<T, int32_t>) {
            // Int32: u32BE bit pattern of the signed value
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Int32 ) );
            out.push_back( std::byte{ 0 } );
            put_u32be( out, static_cast<uint32_t>( item ) );
        } else if constexpr (std::is_same_v<T, double>) {
            // Double: IEEE-754 bits emitted big-endian, byte by byte
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Double ) );
            out.push_back( std::byte{ 0 } );
            uint64_t bits;
            std::memcpy( &bits, &item, 8 );
            for (int i = 7; i >= 0; --i) {
                out.push_back(
                    static_cast<std::byte>( (bits >> (i * 8)) & 0xFF ) );
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            // String: u32BE byte-length + UTF-16BE payload (no BOM)
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::String ) );
            out.push_back( std::byte{ 0 } );
            auto b = utf8_to_utf16be( item );
            put_u32be( out, static_cast<uint32_t>( b.size() ) );
            append_bytes( out, b );
        } else if constexpr (std::is_same_v<T, Blob>) {
            // Bytes: u32BE byte-length + raw payload
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Bytes ) );
            out.push_back( std::byte{ 0 } );
            put_u32be( out, static_cast<uint32_t>( item.size() ) );
            append_bytes( out, item );
        } else if constexpr (std::is_same_v<T, Map>) {
            // Map: recurse — u32BE entry count + tagged key/value pairs
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::Map ) );
            out.push_back( std::byte{ 0 } );
            encode_tagged_map_into( out, item );
        } else if constexpr (std::is_same_v<T, List>) {
            // List: u32BE element count + each element tagged-encoded
            put_u32be( out, static_cast<uint32_t>( Tagged_Type::List ) );
            out.push_back( std::byte{ 0 } );
            put_u32be( out, static_cast<uint32_t>( item.size() ) );
            for (const auto& elem : item) {
                encode_tagged_value( out, elem );
            }
        }
    }, v );
}

} // anonymous namespace

/***************************/
/*   Decode tagged map     */
/***************************/
std::optional<Map> decode_tagged_map( std::span<const std::byte> buf,
                                      size_t& offset ) {
    if ( offset + 4 > buf.size() ) {
        return std::nullopt;
    }
    const uint32_t count = get_u32be( buf.data() + offset );
    offset += 4;
    if ( count > 10000 ) {
        return std::nullopt;
    }
    Map out;
    for (uint32_t i = 0; i < count; ++i) {
        auto key = get_str( buf, offset );
        auto val = decode_tagged_value( buf, offset );
        if ( !key || !val ) {
            return std::nullopt;
        }
        out[std::move( *key )] = std::move( *val );
    }
    return out;
}

/***************************/
/*  Decode tagged array    */
/***************************/
std::optional<std::vector<Map>> decode_tagged_array(
    std::span<const std::byte> buf ) {
    if ( buf.size() < 4 ) {
        return std::nullopt;
    }
    const uint32_t count = get_u32be( buf.data() );
    if ( count > 10000 ) {
        return std::nullopt;
    }
    size_t off = 4;
    std::vector<Map> out;
    out.reserve( count );
    for (uint32_t i = 0; i < count; ++i) {
        auto m = decode_tagged_map( buf, off );
        if ( !m ) {
            return std::nullopt;
        }
        out.push_back( std::move( *m ) );
    }
    return out;
}

/***************************/
/*   Encode tagged map     */
/***************************/
std::vector<std::byte> encode_tagged_map( const Map& d ) {
    std::vector<std::byte> out;
    encode_tagged_map_into( out, d );
    return out;
}

/***************************/
/*   UTF-8 -> UTF-16BE     */
/***************************/
std::vector<std::byte> utf8_to_utf16be( std::string_view s ) {
    std::vector<std::byte> out;
    out.reserve( s.size() * 2 );

    auto emit_unit = [&out]( uint16_t u ) {
        out.push_back( static_cast<std::byte>( (u >> 8) & 0xFF ) );
        out.push_back( static_cast<std::byte>( u & 0xFF ) );
    };

    size_t i = 0;
    while (i < s.size()) {
        // Decode one UTF-8 codepoint (up to 4 bytes)
        const auto c = static_cast<unsigned char>( s[i] );
        char32_t cp;
        int len;
        if (c < 0x80)            { cp = c & 0x7F; len = 1; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; len = 2; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; len = 3; }
        else                       { cp = c & 0x07; len = 4; }
        if (i + static_cast<size_t>( len ) > s.size()) {
            break;  // truncated sequence — stop
        }
        for (int j = 1; j < len; ++j) {
            cp = (cp << 6) | (static_cast<unsigned char>(
                 s[i + static_cast<size_t>( j )] ) & 0x3F);
        }
        i += static_cast<size_t>( len );

        if (cp < 0x10000) {
            emit_unit( static_cast<uint16_t>( cp ) );
        } else {
            cp -= 0x10000;
            emit_unit( static_cast<uint16_t>( 0xD800 + (cp >> 10) ) );
            emit_unit( static_cast<uint16_t>( 0xDC00 + (cp & 0x3FF) ) );
        }
    }
    return out;
}

/***************************/
/*   UTF-16BE -> UTF-8     */
/***************************/
std::string utf16be_to_utf8( std::span<const std::byte> data ) {
    std::string out;
    for (size_t i = 0; i + 1 < data.size(); i += 2) {
        char32_t cp = (static_cast<uint32_t>( data[i] ) << 8)
                    | static_cast<uint32_t>( data[i + 1] );
        // Surrogate pair
        if (cp >= 0xD800 && cp < 0xDC00 && i + 3 < data.size()) {
            const char32_t lo = (static_cast<uint32_t>( data[i + 2] ) << 8)
                              | static_cast<uint32_t>( data[i + 3] );
            if (lo >= 0xDC00 && lo < 0xE000) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        }
        // Encode UTF-8
        if (cp < 0x80) {
            out.push_back( static_cast<char>( cp ) );
        } else if (cp < 0x800) {
            out.push_back( static_cast<char>( 0xC0 | (cp >> 6) ) );
            out.push_back( static_cast<char>( 0x80 | (cp & 0x3F) ) );
        } else if (cp < 0x10000) {
            out.push_back( static_cast<char>( 0xE0 | (cp >> 12) ) );
            out.push_back( static_cast<char>( 0x80 | ((cp >> 6) & 0x3F) ) );
            out.push_back( static_cast<char>( 0x80 | (cp & 0x3F) ) );
        } else {
            out.push_back( static_cast<char>( 0xF0 | (cp >> 18) ) );
            out.push_back( static_cast<char>( 0x80 | ((cp >> 12) & 0x3F) ) );
            out.push_back( static_cast<char>( 0x80 | ((cp >> 6) & 0x3F) ) );
            out.push_back( static_cast<char>( 0x80 | (cp & 0x3F) ) );
        }
    }
    return out;
}

} // namespace ovb::mpad
