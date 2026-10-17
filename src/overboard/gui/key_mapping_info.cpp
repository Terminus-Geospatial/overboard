/**
 * @file    key_mapping_info.cpp
 * @author  Marvin Smith
 * @date    2026-06-03
 *
 * @brief   Key Mapping Info Panel implementation
 */
#include <overboard/gui/key_mapping_info.hpp>

// C++ Standard Libraries
#include <string>

// Project Libraries
#include <overboard/core/action_code.hpp>
#include <overboard/font/font_selector.hpp>
#include <overboard/gui/lvgl_theme.hpp>
#include <overboard/log/stdout_logger.hpp>

namespace ovb::gui {

/***************************/
/*        Constructor       */
/***************************/
Key_Mapping_Info::Key_Mapping_Info( lv_obj_t*                      parent,
                                   const ovb::core::Grid_Layout& layout,
                                   const core::Layer_Manager&    layers,
                                   int                           width,
                                   int                           height,
                                   std::function<std::string(int)> get_label_cb )
    : m_layout(layout), m_layers(layers), m_width(width), m_height(height), m_get_label_cb(std::move(get_label_cb))
{
    // Root container: full display, light background, no padding
    m_container = lv_obj_create(parent);
    lv_obj_set_size(m_container, width, height);
    lv_obj_align(m_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(m_container, lvgl_color(LVGL_COLOR_KBD_BG), LV_PART_MAIN);
    lv_obj_set_style_border_width(m_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(m_container, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(m_container, 0, LV_PART_MAIN);
    lv_obj_set_scrollable(m_container, false);

    // Header bar: layer name
    lv_obj_t* header = lv_obj_create(m_container);
    lv_obj_set_size(header, width, HEADER_H);
    lv_obj_align(header, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(header, lvgl_color(LVGL_COLOR_KBD_HEADER), LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(header, 0, LV_PART_MAIN);
    lv_obj_set_scrollable(header, false);

    m_header_label = lv_label_create(header);
    const std::string layer_text = "Layer: " + std::string(m_layers.current_layer().name);
    lv_label_set_text(m_header_label, layer_text.c_str());
    lv_obj_set_style_text_color(m_header_label, lvgl_color(LVGL_COLOR_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_align(m_header_label, LV_ALIGN_LEFT_MID, MARGIN_LEFT, 0);

    build_keys(m_container);
    LOG_DEBUG("Key_Mapping_Info: Initialized with " + std::to_string(m_key_labels.size()) + " keys");
}

/***************************/
/*        Destructor        */
/***************************/
Key_Mapping_Info::~Key_Mapping_Info() {
    // Remove all event callbacks to prevent use-after-free during LVGL cleanup
    for (lv_obj_t* rect : m_key_rects) {
        if (rect) {
            lv_obj_remove_event_cb(rect, on_btn_clicked);
        }
    }
}

/***************************/
/*        Build Keys        */
/***************************/
void Key_Mapping_Info::build_keys(lv_obj_t* parent) {
    const int key_count = m_layout.key_count();
    m_key_labels.assign(static_cast<std::size_t>(key_count), nullptr);
    m_key_rects.assign(static_cast<std::size_t>(key_count), nullptr);

    const int grid_x  = MARGIN_LEFT;
    const int grid_y  = HEADER_H + MARGIN_TOP;
    const int grid_w  = m_width  - MARGIN_LEFT - KEY_PAD;
    const int grid_h  = m_height - grid_y       - KEY_PAD;
    const int cell_w  = grid_w / m_layout.cols();
    const int cell_h  = grid_h / m_layout.rows();

    const auto& layer = m_layers.current_layer();
    LOG_TRACE("Key_Mapping_Info::build_keys: layer.keys.size=" + std::to_string(layer.keys.size()));

    int keys_created = 0;

    for (int i = 0; i < key_count; ++i) {
        auto pos_opt = m_layout.get_key_position(i);
        if (!pos_opt) {
            continue;
        }

        if (m_layout.get_cell_type(pos_opt->col, pos_opt->row) != ovb::core::Cell_Type::KEY_START) {
            continue;
        }

        auto rect_opt = m_layout.get_key_rect(
            i,
            ovb::core::Point<int>{grid_x, grid_y},
            ovb::core::Point<int>{cell_w,  cell_h},
            KEY_PAD);
        if (!rect_opt) {
            continue;
        }

        const auto& r = *rect_opt;

        // Key widget with optional click handling
        lv_obj_t* rect = lv_obj_create(parent);
        if (!rect) {
            continue;
        }
        keys_created++;

        lv_obj_set_pos(rect, r.x, r.y);
        lv_obj_set_size(rect, r.w, r.h);
        lv_obj_set_style_bg_color(rect, lvgl_color(LVGL_COLOR_KBD_BUTTON), LV_PART_MAIN);
        lv_obj_set_style_border_color(rect, lvgl_color(LVGL_COLOR_BORDER_DARK), LV_PART_MAIN);
        lv_obj_set_style_border_width(rect, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(rect, 4, LV_PART_MAIN);
        lv_obj_set_style_pad_all(rect, 2, LV_PART_MAIN);
        lv_obj_set_style_bg_color(rect, lvgl_color(LVGL_COLOR_BORDER_DARK), static_cast<lv_style_selector_t>(static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));
        lv_obj_set_style_border_width(rect, 2, static_cast<lv_style_selector_t>(static_cast<uint32_t>(LV_PART_MAIN) | static_cast<uint32_t>(LV_STATE_PRESSED)));

        // Store key index for click handling
        lv_obj_set_user_data(rect, reinterpret_cast<void*>(static_cast<intptr_t>(i)));

        // Label
        lv_obj_t* lbl = lv_label_create(rect);
        if (!lbl) continue;

        auto action_code = layer.keys[static_cast<std::size_t>(i)];
        // Get display text: custom panel label > JSON label > action code display
        std::string text = get_key_label(i, action_code);

        // Set label text - empty string for unassigned keys
        lv_label_set_text(lbl, text.c_str());

        // Hide label if no text to display
        if (text.empty()) {
            lv_obj_set_hidden(lbl, true);
        } else {
            lv_obj_set_hidden(lbl, false);
        }

        lv_obj_set_style_text_color(lbl, lvgl_color(LVGL_COLOR_TEXT_PRIMARY), LV_PART_MAIN);

        // Use custom font for math symbols
        if (font::requires_custom_font(action_code)) {
            lv_obj_set_style_text_font(lbl, &lv_font_superscript_bold, LV_PART_MAIN);
        }

        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        m_key_rects[static_cast<std::size_t>(i)]  = rect;
        m_key_labels[static_cast<std::size_t>(i)] = lbl;
    }
    LOG_TRACE("Key_Mapping_Info::build_keys: created=" + std::to_string(keys_created) +
              "/" + std::to_string(key_count) + " keys");
}

/***************************/
/*       Layer Update      */
/***************************/
void Key_Mapping_Info::update_layer() {
    // If overlay is active, don't overwrite with layer data
    if (!m_overlay_stack.empty()) return;

    const auto& layer = m_layers.current_layer();
    const std::size_t key_count = m_key_labels.size();

    // Update header label
    const std::string layer_text = "Layer: " + std::string(layer.name);
    lv_label_set_text(m_header_label, layer_text.c_str());

    for (std::size_t i = 0; i < key_count; ++i) {
        if (!m_key_labels[i]) continue;
        // Get display text: custom panel label > JSON label > action code display
        std::string text = get_key_label(static_cast<int>(i), layer.keys[i]);
        lv_label_set_text(m_key_labels[i], text.c_str());

        // Use custom font for math symbols
        if (font::requires_custom_font(layer.keys[i])) {
            lv_obj_set_style_text_font(m_key_labels[i], &lv_font_superscript_bold, LV_PART_MAIN);
        } else {
            lv_obj_set_style_text_font(m_key_labels[i], LVGL_FONT_SMALL, LV_PART_MAIN);
        }

        // Show/hide label based on whether there's text
        if (text.empty()) {
            lv_obj_set_hidden(m_key_labels[i], true);
        } else {
            lv_obj_set_hidden(m_key_labels[i], false);
        }
    }
    LOG_DEBUG("Key_Mapping_Info::update_layer: complete");
}

/********************************/
/*        Push Overlay          */
/********************************/
void Key_Mapping_Info::push_overlay(const std::string& title,
                                    const std::vector<I_Panel::Overlay_Key_Desc>& keys) {
    m_overlay_stack.push_back({title, keys});
    apply_top_overlay();
}

/********************************/
/*         Pop Overlay          */
/********************************/
void Key_Mapping_Info::pop_overlay() {
    if (m_overlay_stack.empty()) return;

    m_overlay_stack.pop_back();

    if (m_overlay_stack.empty()) {
        // Restore normal layer display
        update_layer();
    } else {
        // Show previous overlay frame
        apply_top_overlay();
    }
}

/********************************/
/*    Is Overlay Active         */
/********************************/
bool Key_Mapping_Info::is_overlay_active() const {
    return !m_overlay_stack.empty();
}

/********************************/
/*     Apply Top Overlay        */
/********************************/
void Key_Mapping_Info::apply_top_overlay() {
    if (m_overlay_stack.empty()) return;

    const auto& frame = m_overlay_stack.back();
    const std::size_t key_count = m_key_labels.size();

    // Update header to show overlay title
    lv_label_set_text(m_header_label, frame.title.c_str());

    // Reset all keys to their regular layer labels first (passthrough)
    const auto& layer = m_layers.current_layer();
    for (std::size_t i = 0; i < key_count; ++i) {
        if (!m_key_labels[i]) continue;
        const std::string text = get_key_label(static_cast<int>(i), layer.keys[i]);
        lv_label_set_text(m_key_labels[i], text.c_str());
        lv_obj_set_style_text_font(m_key_labels[i], LVGL_FONT_SMALL, LV_PART_MAIN);
        if (text.empty()) {
            lv_obj_set_hidden(m_key_labels[i], true);
        } else {
            lv_obj_set_hidden(m_key_labels[i], false);
        }
    }

    // Set overlay key labels (overrides passthrough for matched keys)
    for (const auto& key : frame.keys) {
        if (key.key_index >= 0 && key.key_index < static_cast<int>(key_count)) {
            lv_obj_t* label = m_key_labels[static_cast<size_t>(key.key_index)];
            if (label) {
                lv_label_set_text(label, key.label.c_str());
                lv_obj_set_hidden(label, false);

                // Use custom font for math symbols
                if (font::requires_custom_font(key.label)) {
                    lv_obj_set_style_text_font(label, &lv_font_superscript_bold, LV_PART_MAIN);
                } else {
                    lv_obj_set_style_text_font(label, LVGL_FONT_SMALL, LV_PART_MAIN);
                }
            }
        }
    }
}

/***************************/
/*      Get Key Label      */
/***************************/
std::string Key_Mapping_Info::get_key_label(int key_index, core::Action_Code action_code) const {
    // First: check if panel provides a custom label
    if (m_get_label_cb) {
        std::string custom = m_get_label_cb(key_index);
        if (!custom.empty()) {
            return custom;
        }
    }
    // Second: use JSON label from keyboard.json
    const auto& layer = m_layers.current_layer();
    std::string text = layer.labels[static_cast<std::size_t>(key_index)];
    if (!text.empty()) {
        return text;
    }
    // Third: fall back to action code display string
    return core::action_code_to_display(action_code);
}

/********************************/
/*      Find Key By Label       */
/********************************/
int Key_Mapping_Info::find_key_by_label(const std::string& label) const {
    if (label.empty()) return -1;
    const auto& layer = m_layers.current_layer();
    const auto  count = static_cast<int>(m_key_labels.size());
    for (int i = 0; i < count; ++i) {
        if (!m_key_labels[static_cast<std::size_t>(i)]) continue;
        std::string key_label = get_key_label(i, layer.keys[static_cast<std::size_t>(i)]);
        if (key_label == label) return i;
    }
    return -1;
}

/********************************/
/*         Flash Key            */
/********************************/
void Key_Mapping_Info::flash_key(int key_index) {
    auto idx = static_cast<std::size_t>(key_index);
    if (idx >= m_key_rects.size() || !m_key_rects[idx]) return;

    lv_obj_t* rect = m_key_rects[idx];
    lv_obj_add_state(rect, LV_STATE_PRESSED);

    lv_timer_create([](lv_timer_t* timer) {
        auto* obj = static_cast<lv_obj_t*>(lv_timer_get_user_data(timer));
        lv_obj_clear_state(obj, LV_STATE_PRESSED);
        lv_timer_delete(timer);
    }, FLASH_MS, rect);
}

/********************************/
/*      Set Click Callback      */
/********************************/
void Key_Mapping_Info::set_click_callback(Click_Callback cb) {
    m_click_cb = std::move(cb);
    for (lv_obj_t* rect : m_key_rects) {
        if (!rect) continue;
        lv_obj_set_clickable(rect, true);
        lv_obj_add_event_cb(rect, on_btn_clicked, LV_EVENT_CLICKED, this);
    }
}

/********************************/
/*      On Button Clicked       */
/********************************/
void Key_Mapping_Info::on_btn_clicked(lv_event_t* e) {
    auto* self = static_cast<Key_Mapping_Info*>(lv_event_get_user_data(e));
    if (!self || !self->m_click_cb) return;
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    const auto key_index = static_cast<int>(
        reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn)));
    self->m_click_cb(key_index);
}

} // namespace ovb::gui
