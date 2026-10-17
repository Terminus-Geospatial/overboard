/**
 * @file    device.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 device connection — reader thread, command dispatch,
 *          theme upload, telemetry push, key-event callbacks.
 *
 * Ported from the Python Macropad client (terminus.mpad.client).
 */
#pragma once

// C++ Standard Libraries
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <vector>

// Third-Party Libraries
#include <nlohmann/json.hpp>

// Project Libraries
#include <overboard/mpad/protocol.hpp>
#include <overboard/mpad/serial_port.hpp>

namespace ovb::mpad {

/// Info returned by FIND_DEVICE
struct Device_Info {
    std::string version;
    std::string screen_model;
    std::string device_name;
    int         screen_width  { 0 };
    int         screen_height { 0 };
    int         device_volume { 0 };
    int         device_backlight { 0 };
    std::map<std::string, std::string> raw;
};

/// A key or encoder-click event reported by the device
struct Pad_Key_Event {
    int  row     { -1 };
    int  col     { -1 };
    bool pressed { false };

    static constexpr int ENCODER_LEFT  { 100 };
    static constexpr int ENCODER_RIGHT { 103 };

    /// Encoder events arrive as row==col >= 100
    bool is_encoder() const { return row == col && row >= ENCODER_LEFT; }
};

/**
 * @brief Connection to an MK20 over its USB CDC-ACM serial interface.
 *
 * Opens the port, starts a reader thread, and provides request/reply
 * commands plus unsolicited key-event dispatch.
 */
class Device {
    public:

        using Key_Handler   = std::function<void(const Pad_Key_Event&)>;
        using Json_Handler  = std::function<void(const nlohmann::json&)>;
        using Frame_Handler = std::function<void(const Frame&)>;

        /**
         * @brief Open port, start reader thread, send abort, find device
         * @param port_path Serial device node
         * @param timeout_s Command reply timeout in seconds (default 5)
         */
        explicit Device( const std::string& port_path, double timeout_s = 5.0 );

        /// Close port, join reader thread
        ~Device();

        Device( const Device& )            = delete;
        Device& operator=( const Device& ) = delete;

        /// @brief True if the serial port opened successfully
        bool is_open() const;

        /// @brief Latest FIND_DEVICE info (populated at construction)
        const std::optional<Device_Info>& info() const;

        // -- commands ----------------------------------------------------

        /// FIND_DEVICE — also serves as ping/sleep check (backlight==0)
        std::optional<Device_Info> find_device();

        /// Activate an installed theme (fires the SEND_JSON telemetry contract)
        void reload_theme( const std::string& device_path );

        /// SET_DEVICE_BL — ASCII "0".."100"; cannot wake a sleeping screen
        void set_backlight( int level );

        /// GET_DEVICE_THEME — paths plus storage stats
        std::optional<std::map<std::string, std::string>> get_themes();

        /// SEND_SYSTEM_DATA — push telemetry to widgets bound by the theme
        void push_system_data( const std::map<std::string, std::string>& values );

        /// SET_DEVICE_DELETE_THEME — returns true on res==1
        bool delete_theme( const std::string& device_path );

        /// SEND_JSON — arbitrary JSON payload (e.g. currentPage switch)
        void send_json( const nlohmann::json& payload );

        /// Generic request/reply; nullopt on timeout or bad reply
        std::optional<std::vector<std::byte>> call( uint32_t command,
                                                  std::span<const std::byte> payload = {} );

        /// Research tool for unobserved command IDs (same as call)
        std::optional<std::vector<std::byte>> probe_command(
            uint32_t command, std::span<const std::byte> payload = {} );

        // -- upload ------------------------------------------------------

        /// FILE_START → bulk chunks → FILE_END + abort. true on res==1.
        bool upload_file( std::span<const std::byte> data,
                          const std::string& device_path,
                          size_t chunk_size = 0 );

        /// Upload a .Theme blob and (optionally) activate it
        bool upload_theme( std::span<const std::byte> theme,
                           const std::string& device_path,
                           bool activate = true );

        // -- events ------------------------------------------------------

        /// Register for PROACTIVE_CMD key/encoder events (reader thread)
        void on_key( Key_Handler handler );

        /// Register for unsolicited SEND_JSON replies (reader thread)
        void on_json( Json_Handler handler );

        /// Register for every reply frame, including unconsumed ones
        void on_frame( Frame_Handler handler );

        /// Telemetry contract captured from SEND_JSON after theme activate
        const std::optional<nlohmann::json>& system_data_request() const;

        /// Close port and stop reader thread
        void close();

    private:

        /// Serial port
        Serial_Port m_port;

        /// Command reply timeout
        double m_timeout_s;

        /// Incremental frame parser
        Frame_Parser m_parser;

        /// Write serialization
        std::mutex m_write_mtx;

        /// Theme-mutating ops must never overlap (protocol requirement)
        std::mutex m_op_mtx;

        /// Pending reply waiters per command id
        struct Waiter {
            std::mutex mtx;
            std::condition_variable cv;
            std::optional<std::vector<std::byte>> reply;
        };
        std::mutex m_pending_mtx;
        std::map<uint32_t, std::deque<std::shared_ptr<Waiter>>> m_pending;

        /// Registered observers
        std::vector<Key_Handler>   m_key_handlers;
        std::vector<Json_Handler>  m_json_handlers;
        std::vector<Frame_Handler> m_frame_handlers;

        /// Telemetry contract from the active theme
        std::optional<nlohmann::json> m_system_data_request;

        /// Cached FIND_DEVICE info
        std::optional<Device_Info> m_info;

        /// Reader thread
        std::thread       m_reader;
        std::atomic<bool> m_running { false };

        void read_loop();
        void dispatch( const Frame& frame );
        void handle_event( std::span<const std::byte> payload );
        void send( std::span<const std::byte> data );
};

} // namespace ovb::mpad
