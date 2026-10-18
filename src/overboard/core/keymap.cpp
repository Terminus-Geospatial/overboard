/**
 * @file   keymap.cpp
 * @author Marvin
 * @date   2025-10-18
 *
 * @brief  Keymap implementation
 */

// Project Libraries
#include <overboard/core/keymap.hpp>

namespace ovb::core {

/***************************/
/*       Constructor       */
/***************************/
Keymap::Keymap(const std::array<Layer, LAYER_COUNT>& layers) {
    m_layers = layers;
}

/***************************/
/*         Getters         */
/***************************/
const Layer& Keymap::get_layer(std::size_t index) const {
    return m_layers[index];
}

Action_Code Keymap::get_action(std::size_t layer, std::size_t key_index) const {
    return m_layers[layer].keys[key_index];
}

} // namespace ovb::core
