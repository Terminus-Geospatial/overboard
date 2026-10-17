/**
 * @file    device.cpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 device connection implementation.
 */
#include <overboard/mpad/device.hpp>

// C++ Standard Libraries
#include <chrono>
#include <cstring>

// Project Libraries
#include <overboard/log/stdout_logger.hpp>

namespace ovb::mpad {

namespace {

/***************************/
/*  Parse Device_Info map  */
/***************************/
Device_Info info_from_map( const std::map<std::string, std::string>& m ) {
    auto get = [&m]( const char* k ) -> std::string {
        auto it = m.find( k );
        return it != m.end() ? it->second : "";
    };
    auto iget = [&get]( const char* k ) -> int {
        const auto s = get( k );
        return s.empty() ? 0 : std::atoi( s.c_str() );
    };
    Device_Info info;
    info.version          = get( "version" );
    info.screen_model     = get( "screen_model" );
    info.device_name      = get( "deviceName" );
    info.screen_width     = iget( "screen_width" );
    info.screen_height    = iget( "screen_height" );
    info.device_volume    = iget( "deviceVolume" );
    info.device_backlight = iget( "deviceBl" );
    info.raw              = m;
    return info;
}

} // anonymous namespace

/***************************/
/*        Constructor      */
/***************************/
Device::Device( const std::string& port_path, double timeout_s )
    : m_port( port_path ), m_timeout_s( timeout_s ) {
    if (!m_port.is_open()) {
        LOG_ERROR( "mpad::Device: failed to open ", port_path );
        return;
    }

    m_running = true;
    m_reader  = std::thread( &Device::read_loop, this );

    // Clear any stale file-receive mode left by an interrupted upload
    send( as_bytes( ABORT_MSG ) );
    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    m_info = find_device();
}

/***************************/
/*        Destructor       */
/***************************/
Device::~Device() {
    close();
}

/***************************/
/*         Is open         */
/***************************/
bool Device::is_open() const {
    return m_port.is_open();
}

/***************************/
/*          Info           */
/***************************/
const std::optional<Device_Info>& Device::info() const {
    return m_info;
}

/***************************/
/*       Find device       */
/***************************/
std::optional<Device_Info> Device::find_device() {
    auto reply = call( static_cast<uint32_t>( Command_Id::Find_Device ) );
    if (!reply) {
        return std::nullopt;
    }
    auto m = decode_string_map( *reply );
    if (!m) {
        return std::nullopt;
    }
    return info_from_map( *m );
}

/***************************/
/*      Reload theme       */
/***************************/
void Device::reload_theme( const std::string& device_path ) {
    std::lock_guard lk( m_op_mtx );
    send( encode_frame(
        static_cast<uint32_t>( Command_Id::Set_Device_Reload ),
        as_bytes( device_path ) ) );
}

/***************************/
/*      Set backlight      */
/***************************/
void Device::set_backlight( int level ) {
    const std::string s = std::to_string( level );
    send( encode_frame(
        static_cast<uint32_t>( Command_Id::Set_Device_Bl ),
        as_bytes( s ) ) );
}

/***************************/
/*       Get themes        */
/***************************/
std::optional<std::map<std::string, std::string>> Device::get_themes() {
    auto reply = call( static_cast<uint32_t>( Command_Id::Get_Device_Theme ) );
    if (!reply) {
        return std::nullopt;
    }
    return decode_string_map( *reply );
}

/***************************/
/*    Push system data     */
/***************************/
void Device::push_system_data(
    const std::map<std::string, std::string>& values ) {
    send( encode_frame(
        static_cast<uint32_t>( Command_Id::Send_System_Data ),
        encode_string_map( values ) ) );
}

/***************************/
/*      Delete theme       */
/***************************/
bool Device::delete_theme( const std::string& device_path ) {
    std::lock_guard lk( m_op_mtx );
    auto reply = call(
        static_cast<uint32_t>( Command_Id::Set_Device_Delete_Theme ),
        encode_string_map( { {device_path, ""} } ) );
    if (!reply) {
        return false;
    }
    auto m = decode_string_map( *reply );
    return m && m->at( "res" ) == "1";
}

/***************************/
/*        Send JSON        */
/***************************/
void Device::send_json( const nlohmann::json& payload ) {
    const std::string s = payload.dump();
    send( encode_frame(
        static_cast<uint32_t>( Command_Id::Send_Json ),
        as_bytes( s ) ) );
}

/***************************/
/*          Call           */
/***************************/
std::optional<std::vector<std::byte>> Device::call(
    uint32_t command, std::span<const std::byte> payload ) {
    auto waiter = std::make_shared<Waiter>();
    {
        std::lock_guard lk( m_pending_mtx );
        m_pending[command].push_back( waiter );
    }

    std::optional<std::vector<std::byte>> result;
    send( encode_frame( command, payload ) );
    {
        std::unique_lock lk( waiter->mtx );
        if (waiter->cv.wait_for(
                lk, std::chrono::duration<double>( m_timeout_s ),
                [&] { return waiter->reply.has_value(); } )) {
            result = std::move( *waiter->reply );
        } else {
            LOG_WARN( "mpad::Device: cmd ", command,
                      " timed out waiting for reply" );
        }
    }

    // Remove the waiter if still queued (timeout path)
    std::lock_guard lk( m_pending_mtx );
    auto& dq = m_pending[command];
    for (auto it = dq.begin(); it != dq.end(); ++it) {
        if (*it == waiter) {
            dq.erase( it );
            break;
        }
    }
    return result;
}

/***************************/
/*      Probe command      */
/***************************/
std::optional<std::vector<std::byte>> Device::probe_command(
    uint32_t command, std::span<const std::byte> payload ) {
    return call( command, payload );
}

/***************************/
/*       Upload file       */
/***************************/
bool Device::upload_file( std::span<const std::byte> data,
                          const std::string& device_path,
                          size_t chunk_size ) {
    const size_t cs = chunk_size ? chunk_size : BULK_CHUNK_SIZE;
    send( as_bytes( ABORT_MSG ) );

    // Wait for the FILE_START ack before sending bulk bytes
    if (!call( static_cast<uint32_t>( Command_Id::File_Start ),
               encode_string_map(
                   { {device_path, std::to_string( data.size() )} } ) )) {
        LOG_ERROR( "mpad::Device: FILE_START failed for ", device_path );
        return false;
    }

    {
        std::lock_guard lk( m_write_mtx );
        for (size_t off = 0; off < data.size(); off += cs) {
            m_port.write( data.subspan( off, std::min( cs, data.size() - off ) ) );
        }
        m_port.flush();
    }

    // FILE_END + abort back-to-back, THEN wait for the reply
    auto waiter = std::make_shared<Waiter>();
    {
        std::lock_guard lk( m_pending_mtx );
        m_pending[static_cast<uint32_t>( Command_Id::File_End )].push_back( waiter );
    }
    {
        std::lock_guard lk( m_write_mtx );
        m_port.write( encode_frame(
            static_cast<uint32_t>( Command_Id::File_End ),
            encode_string_map(
                { {device_path, std::to_string( crc32( data ) )} } ) ) );
        m_port.write( as_bytes( ABORT_MSG ) );
        m_port.flush();
    }

    bool ok = false;
    {
        std::unique_lock lk( waiter->mtx );
        if (!waiter->cv.wait_for(
                lk, std::chrono::duration<double>( m_timeout_s ),
                [&] { return waiter->reply.has_value(); } )) {
            LOG_ERROR( "mpad::Device: FILE_END timed out" );
        } else if (auto m = decode_string_map( *waiter->reply );
                   m && m->at( "res" ) == "1") {
            ok = true;
        } else {
            LOG_ERROR( "mpad::Device: FILE_END rejected" );
        }
    }
    return ok;
}

/***************************/
/*      Upload theme       */
/***************************/
bool Device::upload_theme( std::span<const std::byte> theme,
                           const std::string& device_path,
                           bool activate ) {
    std::lock_guard lk( m_op_mtx );
    if (!upload_file( theme, device_path )) {
        return false;
    }
    if (activate) {
        reload_theme( device_path );
    }
    return true;
}

/***************************/
/*         On key          */
/***************************/
void Device::on_key( Key_Handler handler ) {
    m_key_handlers.push_back( std::move( handler ) );
}

/***************************/
/*        On json          */
/***************************/
void Device::on_json( Json_Handler handler ) {
    m_json_handlers.push_back( std::move( handler ) );
}

/***************************/
/*        On frame         */
/***************************/
void Device::on_frame( Frame_Handler handler ) {
    m_frame_handlers.push_back( std::move( handler ) );
}

/***************************/
/*   System data request   */
/***************************/
const std::optional<nlohmann::json>& Device::system_data_request() const {
    return m_system_data_request;
}

/***************************/
/*          Close          */
/***************************/
void Device::close() {
    m_running = false;
    m_port.close();
    if (m_reader.joinable()) {
        m_reader.join();
    }
}

/***************************/
/*          Send           */
/***************************/
void Device::send( std::span<const std::byte> data ) {
    std::lock_guard lk( m_write_mtx );
    m_port.write( data );
    m_port.flush();
}

/***************************/
/*       Read loop         */
/***************************/
void Device::read_loop() {
    std::array<std::byte, 8192> buf{};
    while (m_running && m_port.is_open()) {
        const int n = m_port.read( buf );
        if (n < 0) {
            break;
        }
        if (n == 0) {
            continue;
        }
        for (const auto& frame : m_parser.feed(
                 std::span<const std::byte>(
                     buf.data(), static_cast<size_t>( n ) ) )) {
            dispatch( frame );
        }
    }
}

/***************************/
/*        Dispatch         */
/***************************/
void Device::dispatch( const Frame& frame ) {
    if (frame.packet_type != PACKET_REPLY) {
        return;
    }
    for (const auto& h : m_frame_handlers) {
        h( frame );
    }
    if (frame.command == static_cast<uint32_t>( Command_Id::Proactive_Cmd )) {
        handle_event( frame.payload );
        return;
    }
    if (frame.command == static_cast<uint32_t>( Command_Id::Send_Json )) {
        const char* first = reinterpret_cast<const char*>( frame.payload.data() );
        auto msg = nlohmann::json::parse( first, first + frame.payload.size(),
                                          nullptr, false );
        if (msg.is_discarded()) {
            return;
        }
        if (msg.contains( "deviceRequestSystemData" )) {
            m_system_data_request = msg;
        }
        for (const auto& h : m_json_handlers) {
            h( msg );
        }
        return;
    }

    // Resolve a pending waiter for this command
    std::shared_ptr<Waiter> waiter;
    {
        std::lock_guard lk( m_pending_mtx );
        auto it = m_pending.find( frame.command );
        if (it != m_pending.end() && !it->second.empty()) {
            waiter = it->second.front();
            it->second.pop_front();
        }
    }
    if (waiter) {
        std::lock_guard lk( waiter->mtx );
        waiter->reply = frame.payload;
        waiter->cv.notify_one();
    }
}

/***************************/
/*      Handle event       */
/***************************/
void Device::handle_event( std::span<const std::byte> payload ) {
    auto maps = decode_tagged_array( payload );
    if (!maps) {
        LOG_WARN( "mpad::Device: bad event payload (", payload.size(), " bytes)" );
        return;
    }

    const Map* state = nullptr;
    for (const auto& m : *maps) {
        auto it = m.find( "type" );
        if (it != m.end() && std::holds_alternative<std::string>( it->second ) &&
            std::get<std::string>( it->second ) == "keyState") {
            state = &m;
        }
    }
    if (!state) {
        return;
    }

    auto get_int = []( const Map& m, const char* k, int dflt ) -> int {
        auto it = m.find( k );
        if (it == m.end()) return dflt;
        if (const auto* p = std::get_if<int32_t>( &it->second )) return *p;
        if (const auto* p = std::get_if<double>( &it->second )) {
            return static_cast<int>( *p );
        }
        return dflt;
    };

    Pad_Key_Event evt;
    evt.row     = get_int( *state, "row", -1 );
    evt.col     = get_int( *state, "col", -1 );
    evt.pressed = get_int( *state, "pressed", 0 ) != 0;

    for (const auto& h : m_key_handlers) {
        h( evt );
    }
}

} // namespace ovb::mpad
