// C++ Standard Libraries
#include <algorithm>
#include <cstring>

// Project Libraries
#include <overboard/core/keyboard_layout.hpp>

namespace ovb::core {

/****************************/
/*     Constructors         */
/****************************/

Grid_Layout::Grid_Layout(int cols, int rows)
    : m_cols(cols), m_rows(rows),
      m_cell_owners(static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), -1) {}

Grid_Layout::Grid_Layout(int cols, int rows, std::vector<Key_Position> key_positions)
    : m_cols(cols), m_rows(rows),
      m_key_positions(std::move(key_positions)),
      m_cell_owners(static_cast<std::size_t>(cols) * static_cast<std::size_t>(rows), -1) {
    rebuild_cell_map();
}

/****************************/
/*     Key Definition       */
/****************************/

bool Grid_Layout::define_key(int key_index, Key_Position pos) {
    // Validate position is within grid
    if (!is_valid_cell(pos.col, pos.row)) {
        return false;
    }
    if (pos.col + pos.col_span > m_cols || pos.row + pos.row_span > m_rows) {
        return false;
    }
    if (pos.col_span < 1 || pos.row_span < 1) {
        return false;
    }

    // Ensure key_index can be stored
    if (key_index < 0) {
        return false;
    }
    if (static_cast<std::size_t>(key_index) >= m_key_positions.size()) {
        m_key_positions.resize(static_cast<std::size_t>(key_index) + 1);
    }

    // Check for conflicts with existing keys
    for (int r = pos.row; r < pos.row + pos.row_span; ++r) {
        for (int c = pos.col; c < pos.col + pos.col_span; ++c) {
            int owner = m_cell_owners[static_cast<std::size_t>(cell_index(c, r))];
            if (owner >= 0 && owner != key_index) {
                return false; // Cell already occupied by another key
            }
        }
    }

    // Remove old cells owned by this key index
    for (auto& owner : m_cell_owners) {
        if (owner == key_index) {
            owner = -1;
        }
    }

    // Set new position
    m_key_positions[static_cast<std::size_t>(key_index)] = pos;

    // Mark cells as owned by this key
    for (int r = pos.row; r < pos.row + pos.row_span; ++r) {
        for (int c = pos.col; c < pos.col + pos.col_span; ++c) {
            m_cell_owners[static_cast<std::size_t>(cell_index(c, r))] = key_index;
        }
    }

    return true;
}

std::optional<Key_Position> Grid_Layout::get_key_position(int key_index) const {
    if (key_index < 0 || static_cast<std::size_t>(key_index) >= m_key_positions.size()) {
        return std::nullopt;
    }
    return m_key_positions[static_cast<std::size_t>(key_index)];
}

/****************************/
/*     Cell Queries         */
/****************************/
Cell_Type Grid_Layout::get_cell_type(int col, int row) const {
    if (!is_valid_cell(col, row)) {
        return Cell_Type::EMPTY;
    }

    int owner = m_cell_owners[static_cast<std::size_t>(cell_index(col, row))];
    if (owner < 0) {
        return Cell_Type::EMPTY;
    }

    // Check if this is the start cell for the key
    auto pos_opt = get_key_position(owner);
    if (!pos_opt) {
        return Cell_Type::EMPTY;
    }

    const auto& pos = *pos_opt;
    if (col == pos.col && row == pos.row) {
        return Cell_Type::KEY_START;
    }
    return Cell_Type::KEY_CONT;
}

int Grid_Layout::get_cell_owner(int col, int row) const {
    if (!is_valid_cell(col, row)) {
        return -1;
    }
    return m_cell_owners[static_cast<std::size_t>(cell_index(col, row))];
}

/****************************/
/*     Rectangle Calc       */
/****************************/
std::optional<core::Rect<int>> Grid_Layout::get_key_rect(
    int key_index,
    core::Point<int> grid_origin,
    core::Point<int> cell_size,
    int padding) const {

    auto pos_opt = get_key_position(key_index);
    if (!pos_opt) {
        return std::nullopt;
    }

    const auto& pos = *pos_opt;

    // Calculate total size using width/height (supports fractional sizes like 1.25u)
    int total_w = static_cast<int>(pos.width * static_cast<float>(cell_size.x));
    int total_h = static_cast<int>(pos.height * static_cast<float>(cell_size.y));

    // Position is based on start cell, with fractional gap offset
    int gap_x = static_cast<int>(pos.col_gap * static_cast<float>(cell_size.x));
    int gap_y = static_cast<int>(pos.row_gap * static_cast<float>(cell_size.y));

    int x = grid_origin.x + pos.col * cell_size.x + gap_x + padding / 2;
    int y = grid_origin.y + pos.row * cell_size.y + gap_y + padding / 2;

    // Adjust for padding (only apply padding between keys, not inside)
    int w = total_w - padding;
    int h = total_h - padding;

    return core::Rect<int>{x, y, w, h};
}

core::Rect<int> Grid_Layout::get_cell_rect(
    int col, int row,
    core::Point<int> grid_origin,
    core::Point<int> cell_size) const {

    int x = grid_origin.x + col * cell_size.x;
    int y = grid_origin.y + row * cell_size.y;
    return core::Rect<int>{x, y, cell_size.x, cell_size.y};
}

/****************************/
/*     Helpers              */
/****************************/

void Grid_Layout::rebuild_cell_map() {
    std::fill(m_cell_owners.begin(), m_cell_owners.end(), -1);

    for (size_t key_idx = 0; key_idx < m_key_positions.size(); ++key_idx) {
        const auto& pos = m_key_positions[key_idx];
        // Skip uninitialized positions
        if (pos.col_span == 0 && pos.row_span == 0) {
            continue;
        }

        for (int r = pos.row; r < pos.row + pos.row_span; ++r) {
            for (int c = pos.col; c < pos.col + pos.col_span; ++c) {
                if (is_valid_cell(c, r)) {
                    m_cell_owners[static_cast<std::size_t>(cell_index(c, r))] = static_cast<int>(key_idx);
                }
            }
        }
    }
}

} // namespace ovb::core
