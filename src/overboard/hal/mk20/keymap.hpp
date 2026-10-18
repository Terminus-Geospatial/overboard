/**
 * @file    keymap.hpp
 * @author  Marvin Smith
 * @date    2026-10-16
 *
 * @brief   MK20 keymap presenter — pushes per-layer themes to the
 *          physical macropad instead of drawing them on screen.
 *
 * Strategy: lazily build + upload one theme per layer on first visit,
 * then layer changes are instant via SET_DEVICE_RELOAD (verified
 * ~0ms host-side on hardware).
 */
#pragma once

// C++ Standard Libraries
#include <chrono>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/core/screen.hpp>
#include <overboard/gui/i_keymap_view.hpp>
#include <overboard/hal/i_system_info.hpp>
#include <overboard/mpad/device.hpp>
#include <overboard/mpad/theme.hpp>

namespace ovb::hal::mk20 {

/**
 * @brief Physical keymap presenter for the Waveshare MK20.
 *
 * Implements I_Keymap_View: update_layer() switches the active theme
 * on the device, overlay push/pop rebuild and swap a temp theme.
 */
class Mk20_Keymap : public gui::I_Keymap_View {
    public:

        /**
         * @brief Create presenter over a live device connection
         * @param device            Shared mpad::Device (also feeds Mk20_Input)
         * @param layout            Grid layout (key positions -> index map)
         * @param layers            Layer manager for current key definitions
         * @param screens           Per-panel screen definitions (may be empty)
         * @param get_label_cb      Optional custom-label callback from active panel
         * @param get_panel_name_cb Active panel name — selects the screen
         * @param system_info       Optional host info for strip telemetry (wifi)
         * @param device_root       Theme install root (default /data/theme/MK20/ovb)
         */
        Mk20_Keymap( std::shared_ptr<mpad::Device>      device,
                     const core::Grid_Layout&           layout,
                     const core::Layer_Manager&         layers,
                     const core::Screen_Set&            screens,
                     std::function<std::string(int)>    get_label_cb = nullptr,
                     std::function<std::string()>       get_panel_name_cb = nullptr,
                     hal::I_System_Info*                system_info = nullptr,
                     std::string device_root = "/data/theme/MK20/ovb" );

        /// @brief Upload or switch to the active layer's theme
        void update_layer() override;

        /// @brief Push overlay — rebuilds current layer with overlay labels
        void push_overlay(
            const std::string& title,
            const std::vector<gui::I_Panel::Overlay_Key_Desc>& keys ) override;

        /// @brief Pop overlay — restores the base layer theme
        void pop_overlay() override;

        /// @brief Brief visual feedback for a physical press (v1: no-op;
        ///        serial feedback would lag the device — kept for parity)
        void flash_key( int key_index ) override;

        /// @brief True if the device connection is live
        bool is_connected() const;

        /**
         * @brief Periodic upkeep — re-pushes strip telemetry (~1 s) while
         *        a data-bound screen is active. Call once per frame.
         */
        void tick();

    private:

        /// @brief Screen for the active panel, or nullptr when the panel
        ///        falls back to layer themes
        const core::Screen* active_screen() const;

        /// Build + upload a screen's theme on first visit
        void ensure_screen_theme( const core::Screen& screen );

        /// Build the Theme_File for a screen (labels + dial functions + strip)
        mpad::Theme_File build_screen_theme( const core::Screen& screen,
            const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay = nullptr ) const;

        /// Push the standard strip telemetry channels (clock/date/app/wifi/ver)
        void push_screen_data( const core::Screen& screen );

        /// Device path for a screen's theme
        std::string screen_path( const core::Screen& screen ) const;

        /// Build + upload a theme for the given layer index (if new)
        void ensure_layer_theme( int layer_index );

        /**
         * @brief Build the Theme_File for a layer (or overlay)
         * @param layer_index The layer index
         * @param overlay Optional overlay keys to override labels
         * @return The Theme_File
         */
        mpad::Theme_File build_theme( int layer_index,
                                      const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay = nullptr ) const;

        /**
         * @brief Resolve a key's label: overlay > panel custom > screen/layer label
         * @param key_index The key index
         * @param layer_index The layer index
         * @param screen The screen
         * @param overlay Optional overlay keys to override labels
         * @return The label
         */
        std::string label_for( int key_index, int layer_index,
            const core::Screen* screen,
            const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const;

        /**
         * @brief Device path for a layer's theme
         * @param layer_index The layer index
         * @return The device path
         */
        std::string layer_path( int layer_index ) const;

        /**
         * @brief Build the blank clear theme — an opaque tile covering the
         * full canvas so stale key art gets overwritten
         */
        mpad::Theme_File build_blank_theme() const;

        /**
         * @brief Upload the blank theme once per session
         */
        void ensure_blank_theme();

        /**
         * @brief Device path for the blank theme
         * @return The device path
         */
        std::string blank_path() const;

        /**
         * @brief Activate a theme via a blank-theme reset — the firmware
         * leaves stale key icons on a plain RELOAD
         * @param device_path The device path
         */
        void activate_theme( const std::string& device_path );

        /// @brief The MPAD device
        std::shared_ptr<mpad::Device>      m_device;
        /// @brief The grid layout
        const core::Grid_Layout&           m_layout;
        /// @brief The layer manager
        const core::Layer_Manager&         m_layers;
        /// @brief The screen set
        const core::Screen_Set&            m_screens;
        /// @brief Callback to get a label for a key
        std::function<std::string(int)>    m_get_label_cb;
        /// @brief Callback to get the panel name
        std::function<std::string()>       m_get_panel_name_cb;
        /// @brief System info
        hal::I_System_Info*                m_system_info;
        /// @brief The device root
        std::string                        m_device_root;

        /// @brief Layers whose themes have already been uploaded
        std::set<int>                      m_uploaded;

        /// @brief Screen names whose themes have already been uploaded
        std::set<std::string>              m_uploaded_screens;

        /// @brief True once the blank clear theme is on the device
        bool                               m_blank_uploaded { false };

        /// @brief Last strip-telemetry push (tick throttles to ~1 s)
        std::chrono::steady_clock::time_point m_last_strip_push{};

        /// @brief Overlay stack (top is active)
        struct Overlay_Frame {
            std::string                                    title;
            std::vector<gui::I_Panel::Overlay_Key_Desc>    keys;
        };
        /// @brief Overlay stack
        std::vector<Overlay_Frame>         m_overlays;
};

} // namespace ovb::hal::mk20
