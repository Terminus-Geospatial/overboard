/**
 * @file   keymap.hpp
 * @author Marvin Smith
 * @date   2025-10-18
 * @brief  Key mapping and layout definitions
 */
#pragma once

// C++ Standard Libraries
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Project Libraries
#include <overboard/core/action_code.hpp>

namespace ovb::core {

static constexpr int LAYER_COUNT = 5;  // Number of keyboard layers

/**
 * @brief Represents a keyboard layer with its name and action mappings
 */
struct Layer {
    std::string              name;
    std::vector<Action_Code> keys;
    std::vector<std::string> labels;  ///< Display label for each key cap (parallel to keys)
};


/**
 * @brief Manages keyboard layer definitions and key mappings.
 *
 * The Keymap class provides access to keyboard layers loaded from JSON,
 * each containing a mapping of key indices to key codes and labels.
 * It supports layer-based keyboard layouts where different functional
 * sets can be switched between.
 */
class Keymap {
    public:

        Keymap() = default;

        /**
         * @brief Constructor with custom layers
         * @param layers Array of layer definitions
         */
        explicit Keymap(const std::array<Layer, LAYER_COUNT>& layers);

        /**
         * @brief Returns the total number of layers in the keymap.
         * @return The layer count (always LAYER_COUNT).
         */
        constexpr int layer_count() const { return LAYER_COUNT; }

        /**
         * @brief Retrieves a layer by index.
         * @param index The layer index (0 to LAYER_COUNT-1).
         * @return Reference to the requested Layer.
         */
        const Layer& get_layer(std::size_t index) const;

        /**
         * @brief Retrieves an action code from a specific layer.
         * @param layer The layer index.
         * @param key_index The key index within that layer.
         * @return Action code for the specified key.
         */
        Action_Code get_action( std::size_t layer,
                               std::size_t key_index ) const;

    private:
        std::array<Layer, LAYER_COUNT> m_layers;  ///< Layer definitions
};

} // namespace ovb::core
