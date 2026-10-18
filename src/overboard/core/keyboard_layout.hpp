/**
 * @file keyboard_layout.hpp
 * @brief Configurable grid-based keyboard layout
 *
 * Defines key positions, sizes, and visibility on a grid.
 * Supports asymmetric layouts with empty cells and multi-cell keys.
 */
#pragma once

// C++ Standard Libraries
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

// Project Libraries
#include <overboard/core/keymap.hpp>
#include <overboard/core/point.hpp>
#include <overboard/core/rect.hpp>

namespace ovb::core {

/**
 * @brief Cell state in the layout grid
 */
enum class Cell_Type : uint8_t {
    EMPTY,      ///< No key at this cell (invisible)
    KEY_START,  ///< Start of a key (visible, defines key origin)
    KEY_CONT    ///< Continuation of a multi-cell key (owned by KEY_START)
};

/**
 * @brief Definition of a key's position and size in grid cells
 */
struct Key_Position {
    int col;           ///< Starting column (0-based)
    int row;           ///< Starting row (0-based)
    int col_span;      ///< Number of columns this key occupies (>= 1)
    int row_span;      ///< Number of rows this key occupies (>= 1)
    float col_gap;     ///< Fractional column offset (gap) in key units
    float row_gap;     ///< Fractional row offset (gap) in key units
    float width;       ///< Actual width in key units (supports fractional like 1.25u)
    float height;      ///< Actual height in key units (supports fractional like 1.5u)

    Key_Position(int c=0, int r=0, int cs=1, int rs=1, float cg=0.0f, float rg=0.0f, float w=1.0f, float h=1.0f)
        : col(c), row(r), col_span(cs), row_span(rs), col_gap(cg), row_gap(rg), width(w), height(h) {}
};

/**
 * @brief Configurable keyboard grid layout
 *
 * Maps logical key indices to physical grid positions with support for:
 * - Irregular/asymmetric layouts (split sections, gaps)
 * - Multi-cell keys (tall/wide keys like + and Enter)
 * - Empty/invisible cells
 *
 * Example Womier SK30-style layout (simplified calculator subset):
 * @code
 *   // 5-column grid with asymmetric layout
 *   Grid_Layout layout(5, 6, {
 *       // Row 0: Top function row
 *       {0, 0, 1, 1}, {1, 0, 1, 1}, {2, 0, 1, 1},  // Esc, Copy, Backspace
 *       {3, 0, 2, 1},                               // Wide clear key
 *       // Row 1-2: Tall + key spans rows 1-2, col 4
 *       {0, 1, 1, 1}, {1, 1, 1, 1}, {2, 1, 1, 1},  // 7, 8, 9
 *       {0, 2, 1, 1}, {1, 2, 1, 1}, {2, 2, 1, 1},  // 4, 5, 6
 *       {4, 1, 1, 2},                               // Tall +
 *       // Row 3: More keys
 *       {0, 3, 1, 1}, {1, 3, 1, 1}, {2, 3, 1, 1},  // 1, 2, 3
 *       {4, 3, 1, 2},                               // Tall Enter
 *       // Row 4
 *       {0, 4, 2, 1}, {2, 4, 1, 1},                 // Wide 0, .
 *       // Row 5: Bottom row
 *       {0, 5, 1, 1}, {1, 5, 1, 1}, {2, 5, 1, 1},  // Layer, Fn, etc
 *   });
 * @endcode
 */
class Grid_Layout {
    public:
        /**
         * @brief Create empty layout (must call define_key() to populate)
         * @param cols Number of columns in the grid
         * @param rows Number of rows in the grid
         */
        Grid_Layout(int cols, int rows);

        /**
         * @brief Create layout with predefined key positions
         * @param cols Number of columns in the grid
         * @param rows Number of rows in the grid
         * @param key_positions Vector of key positions (index = key index)
         */
        Grid_Layout(int cols, int rows, std::vector<Key_Position> key_positions);

        /// @return Grid columns
        int cols() const { return m_cols; }

        /// @return Grid rows
        int rows() const { return m_rows; }

        /// @return Total cells in grid
        int total_cells() const { return m_cols * m_rows; }

        /// @return Number of defined keys
        int key_count() const { return static_cast<int>(m_key_positions.size()); }

        /**
         * @brief Define a key at the specified grid position
         * @param key_index Logical key index (maps to Layer.keys[key_index])
         * @param pos Grid position with column, row, and spans
         * @return true if successful, false if position conflicts with existing key
         */
        bool define_key(int key_index, Key_Position pos);

        /**
         * @brief Get position of a key by index
         * @param key_index Key index
         * @return Key position, or nullopt if not defined
         */
        std::optional<Key_Position> get_key_position(int key_index) const;

        /**
         * @brief Get cell type at grid coordinates
         * @param col Column (0 to cols-1)
         * @param row Row (0 to rows-1)
         * @return Cell type (EMPTY, KEY_START, or KEY_CONT)
         */
        Cell_Type get_cell_type(int col, int row) const;

        /**
         * @brief Get which key owns this cell
         * @param col Column
         * @param row Row
         * @return Key index that owns this cell, or -1 if empty
         */
        int get_cell_owner(int col, int row) const;

        /**
         * @brief Calculate pixel rectangle for a key
         * @param key_index Key index
         * @param grid_origin Top-left of grid in pixels
         * @param cell_size Size of one grid cell in pixels
         * @param padding Padding between keys in pixels
         * @return Rectangle as (x, y, w, h), or nullopt if key not defined
         */
        std::optional<core::Rect<int>> get_key_rect(
            int key_index,
            core::Point<int> grid_origin,
            core::Point<int> cell_size,
            int padding) const;

        /**
         * @brief Calculate pixel rectangle for a specific cell
         * @param col Column
         * @param row Row
         * @param grid_origin Top-left of grid in pixels
         * @param cell_size Size of one grid cell in pixels
         * @return Rectangle as (x, y, w, h)
         */
        core::Rect<int> get_cell_rect(
            int col, int row,
            core::Point<int> grid_origin,
            core::Point<int> cell_size) const;

    private:

        /// @brief Number of columns in the grid
        int m_cols;
        /// @brief Number of rows in the grid
        int m_rows;
        /// @brief Vector of key positions
        std::vector<Key_Position> m_key_positions;

        /// @brief Grid cell ownership: -1 = empty, >=0 = key index that owns this cell
        std::vector<int> m_cell_owners;

        void rebuild_cell_map();
        int cell_index(int col, int row) const { return row * m_cols + col; }
        bool is_valid_cell(int col, int row) const {
            return col >= 0 && col < m_cols && row >= 0 && row < m_rows;
        }
};

} // namespace ovb::core
