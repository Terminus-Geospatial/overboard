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
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/gui/i_keymap_view.hpp>
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
         * @param device       Shared mpad::Device (also feeds Mk20_Input)
         * @param layout       Grid layout (key positions -> index map)
         * @param layers       Layer manager for current key definitions
         * @param get_label_cb Optional custom-label callback from active panel
         * @param device_root  Theme install root (default /data/theme/MK20/ovb)
         */
        Mk20_Keymap( std::shared_ptr<mpad::Device>      device,
                     const core::Grid_Layout&           layout,
                     const core::Layer_Manager&         layers,
                     std::function<std::string(int)>    get_label_cb = nullptr,
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

    private:

        /// Build + upload a theme for the given layer index (if new)
        void ensure_layer_theme( int layer_index );

        /// Build the Theme_File for a layer (or overlay)
        mpad::Theme_File build_theme( int layer_index,
            const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay = nullptr ) const;

        /// Resolve a key's label: overlay > panel custom > layer label
        std::string label_for( int key_index, int layer_index,
            const std::vector<gui::I_Panel::Overlay_Key_Desc>* overlay ) const;

        /// Device path for a layer's theme
        std::string layer_path( int layer_index ) const;

        std::shared_ptr<mpad::Device>      m_device;
        const core::Grid_Layout&           m_layout;
        const core::Layer_Manager&         m_layers;
        std::function<std::string(int)>    m_get_label_cb;
        std::string                        m_device_root;

        /// Layers whose themes have already been uploaded
        std::set<int>                      m_uploaded;

        /// Overlay stack (top is active)
        struct Overlay_Frame {
            std::string                                    title;
            std::vector<gui::I_Panel::Overlay_Key_Desc>    keys;
        };
        std::vector<Overlay_Frame>         m_overlays;
};

} // namespace ovb::hal::mk20
