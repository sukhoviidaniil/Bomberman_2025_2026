/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-31

 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_TILEPOS_H
#define BOMBERMAN_LOGIC_TILEPOS_H

#include <cstdlib>
#include <tuple>

namespace bomberman::logic {

    /**
     * @brief A cell of the arena, addressed by row and column.
     *
     * Deliberately *signed*. The Pac-Man version stored row/column as
     * size_t, so every "one tile up" computation (`pos.row - 1`, or a
     * bot aiming four tiles ahead) silently wrapped to ~1.8e19 on the
     * top row and then clamped to the opposite edge - a targeting bug
     * that looked like bad AI rather than like arithmetic. Out-of-range
     * coordinates are normal intermediate values here; TileGrid decides
     * what they mean.
     */
    struct TilePos {
        int row = 0;
        int col = 0;

        constexpr TilePos() = default;
        constexpr TilePos(const int row_, const int col_) : row(row_), col(col_) {}

        constexpr bool operator==(const TilePos& other) const noexcept { return row == other.row && col == other.col; }

        constexpr bool operator!=(const TilePos& other) const noexcept { return !(*this == other); }

        /// @brief Lexicographic ordering, so TilePos can be a map key.
        bool operator<(const TilePos& other) const noexcept {
            return std::tie(row, col) < std::tie(other.row, other.col);
        }

        constexpr TilePos operator+(const TilePos& other) const noexcept { return {row + other.row, col + other.col}; }
    };

    /// @brief Grid ("taxicab") distance between two cells.
    [[nodiscard]] inline int manhattan_distance(const TilePos& a, const TilePos& b) noexcept {
        return std::abs(a.row - b.row) + std::abs(a.col - b.col);
    }

    /// @brief True if the cells touch or coincide.
    [[nodiscard]] inline bool are_close(const TilePos& a, const TilePos& b) noexcept {
        return std::abs(a.row - b.row) <= 1 && std::abs(a.col - b.col) <= 1;
    }

    struct TilePosHash {
        std::size_t operator()(const TilePos& p) const noexcept {
            return static_cast<std::size_t>(p.row) * 73856093u ^ static_cast<std::size_t>(p.col) * 19349663u;
        }
    };
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_TILEPOS_H
