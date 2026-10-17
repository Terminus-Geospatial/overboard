/**
 * @file    key_mapping_info.hpp
 * @author  Marvin Smith
 * @date    2026-06-03
 *
 * @brief   Key Mapping Info Panel
 *
 * Displays current layer key mappings as interactive buttons.
 * Used in the main window when the keyboard is in a separate window.
 */
#pragma once

// C++ Standard Libraries
#include <functional>
#include <string>
#include <vector>

// Third-Party Libraries
#include <lvgl.h>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>
#include <overboard/core/layer_manager.hpp>
#include <overboard/gui/i_keymap_view.hpp>
#include <overboard/gui/i_panel.hpp>

namespace ovb::gui {

/**
 * @brief Key mapping information panel with optional interactive buttons
 *
 * Renders current layer key mappings in a grid layout.
 * Can be made interactive by calling set_click_callback() after construction.
 * Designed for macropad users who need to see dynamic key mappings,
 * and for simulator/browser users who can click keys to interact.
 */
class Key_Mapping_Info : public I_Keymap_View {

    public:

        static constexpr int KEY_PAD       = 2;   ///< Gap between keys
        static constexpr int HEADER_H      = 20;  ///< Header height
        static constexpr int MARGIN_LEFT   = 8;   ///< Left margin
        static constexpr int MARGIN_TOP    = 6;   ///< Top margin (below header)
        static constexpr int FLASH_MS      = 120; ///< Key flash duration in milliseconds

        /// @brief Callback invoked when a key button is clicked; argument is the key index
        using Click_Callback = std::function<void(int key_index)>;

        /**
         * @brief Create key mapping info panel
         * @param parent           LVGL parent object
         * @param layout           Grid layout defining key positions
         * @param layers           Layer manager for current layer key definitions
         * @param width            Display width in pixels
         * @param height           Display height in pixels
         * @param get_label_cb     Optional callback to get custom label from active panel
         */
        Key_Mapping_Info( lv_obj_t*                      parent,
                         const ovb::core::Grid_Layout& layout,
                         const core::Layer_Manager&    layers,
                         int                           width,
                         int                           height,
                         std::function<std::string(int)> get_label_cb = nullptr );

        /**
         * @brief Destructor - removes event callbacks before LVGL cleanup
         */
        ~Key_Mapping_Info();

        /**
         * @brief Update display to match current layer
         */
        void update_layer() override;

        /**
         * @brief Register a callback invoked on every button click
         *
         * Must be called after construction. Retroactively enables
         * LV_OBJ_FLAG_CLICKABLE on all key widgets and attaches the handler.
         *
         * @param cb Callable receiving the logical key index
         */
        void set_click_callback(Click_Callback cb);

        /**
         * @brief Push an overlay frame onto the stack
         *
         * Keys not listed keep their current label (passthrough).
         * Multiple overlays can be stacked; pop removes the top one.
         *
         * @param title  Header text for this overlay (e.g., "Alg")
         * @param keys   Overlay keys to display on this frame
         */
        void push_overlay(const std::string& title,
                          const std::vector<I_Panel::Overlay_Key_Desc>& keys) override;

        /**
         * @brief Pop the top overlay frame from the stack
         *
         * Restores the previous overlay or normal layer display.
         */
        void pop_overlay() override;

        /**
         * @brief Check if any overlay is active
         */
        bool is_overlay_active() const;

        /**
         * @brief Briefly flash a key to show it was pressed
         *
         * Sets the key to its pressed visual state, then restores it
         * after a short LVGL timer. Used for physical keypress feedback.
         *
         * @param key_index Logical key index to flash
         */
        void flash_key(int key_index) override;

        /**
         * @brief Find a key index whose current display label matches the given string
         *
         * Searches the current layer labels and action code display strings.
         * Returns -1 if no match is found.
         *
         * @param label Display label to search for (e.g. "2", "+", "sin")
         * @return Key index, or -1 if not found
         */
        int find_key_by_label(const std::string& label) const;

        /**
         * @brief Get the container object for positioning
         * @return LVGL container object
         */
        lv_obj_t* container() const { return m_container; }

    private:

        /// @brief Grid layout defining key positions
        const core::Grid_Layout&   m_layout;

        /// @brief Layer manager for current layer key definitions
        const core::Layer_Manager& m_layers;

        /// @brief Display width in pixels
        int                        m_width;

        /// @brief Display height in pixels
        int                        m_height;

        /// @brief Container object for all elements
        lv_obj_t*              m_container = nullptr;

        /// @brief Header label showing current layer name
        lv_obj_t*              m_header_label = nullptr;

        /// @brief Vector of label objects, one per key
        std::vector<lv_obj_t*> m_key_labels;

        /// @brief Vector of rectangle objects, one per key
        std::vector<lv_obj_t*> m_key_rects;

        /// @brief Callback to get custom label from active panel
        std::function<std::string(int)> m_get_label_cb;

        /// @brief Single overlay frame
        struct Overlay_Frame {
            std::string title;
            std::vector<I_Panel::Overlay_Key_Desc> keys;
        };

        /// @brief Overlay stack (top frame is active)
        std::vector<Overlay_Frame> m_overlay_stack;

        /// @brief Click callback (null if not interactive)
        Click_Callback m_click_cb;

        /// @brief Apply the top overlay frame (or restore layer if empty)
        void apply_top_overlay();

        /// @brief Build all key display elements from current layout
        void build_keys(lv_obj_t* parent);

        /// @brief Get display text for a key, checking custom label callback first
        std::string get_key_label(int key_index, core::Action_Code action_code) const;

        /// @brief LVGL event handler forwarded to m_click_cb
        static void on_btn_clicked(lv_event_t* e);
};

} // namespace ovb::gui
