/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-04
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_TILEGRID_H
#define BOMBERMAN_LOGIC_TILEGRID_H

#include <cstddef>
#include <optional>
#include <vector>

#include "bomberman/logic/Direction.h"
#include "bomberman/logic/grid/Tile.h"
#include "bomberman/logic/grid/TilePos.h"

#include "sif/internal/Rect.h"
#include "sif/math/Point2.h"

namespace bomberman::logic {

    /**
     * @brief The arena: a rectangular grid of tiles, addressed either by
     * cell or by world position.
     *
     * @par Normalized world coordinates
     * The grid maps itself into the square [-1, 1] x [-1, 1] and nothing
     * outside this class ever needs to know how many pixels a tile is.
     * That is a requirement of the assignment ("the World width and
     * height bounded by [-1, 1]"), and it is also what makes the speed
     * values in the config resolution-independent: the Pac-Man grid used
     * a tile size of 1 and a world of 21x11, so every speed constant was
     * secretly tied to the tile size and re-tuning the map re-tuned the
     * whole game.
     *
     * Tiles stay square: the shorter axis is filled and the longer one is
     * centred, exactly like sif::rnd::AspectPolicy::Fit does for pixels.
     */
    class TileGrid {
    public:
        TileGrid() = default;

        /**
         * @brief Builds an empty (all Free) grid of the given size.
         *
         * @throws std::invalid_argument if rows or columns is 0.
         */
        TileGrid(std::size_t rows, std::size_t columns);

        // ===== Layout =====

        [[nodiscard]] std::size_t rows() const;
        [[nodiscard]] std::size_t columns() const;

        /// @brief Edge length of one tile, in world units.
        [[nodiscard]] float tile_size() const;

        /// @brief World-space rectangle covered by the whole arena.
        [[nodiscard]] sif::intrnl::Rect bounds() const;

        // ===== Tiles =====

        /// @brief True if the cell is inside the grid.
        [[nodiscard]] bool contains(const TilePos& pos) const;

        /**
         * @brief Tile at a cell.
         *
         * Out-of-range cells read as Indestructible: the arena behaves as
         * if it were walled in, so movement and blast code never needs a
         * separate bounds check before asking.
         */
        [[nodiscard]] Tile get_tile(const TilePos& pos) const;

        /// @throws std::out_of_range if the cell is outside the grid.
        void set_tile(const TilePos& pos, Tile tile);

        /// @brief Clamps a cell into the grid.
        [[nodiscard]] TilePos clamp(TilePos pos) const;

        // ===== Cells and world positions =====

        /// @brief Cell containing a world position, or nullopt if outside.
        [[nodiscard]] std::optional<TilePos> get_TilePos(const sif::math::Point2& pos) const;

        /// @brief Neighbouring cell in a direction (may be outside the grid).
        [[nodiscard]] TilePos neighbour(const TilePos& pos, Direction dir) const;

        /// @brief World-space centre of a cell.
        [[nodiscard]] sif::math::Point2 get_center(const TilePos& pos) const;

        /// @brief World-space rectangle of a cell.
        [[nodiscard]] sif::intrnl::Rect get_rect(const TilePos& pos) const;

        /**
         * @brief True where a character may change direction.
         *
         * A cell is a junction when at least one direction other than the
         * one travelled (and its reverse) leads somewhere walkable.
         */
        [[nodiscard]] bool is_junction(const TilePos& pos, Direction current) const;

        /**
         * @brief Fills the grid with the classic Bomberman layout.
         *
         * Indestructible pillars on every even row/column, the rest
         * seeded with destructible blocks, and the four corners plus
         * their neighbours left clear so nobody spawns entombed.
         *
         * @param destructible_chance Probability a free cell becomes a
         * destructible block, clamped to [0, 1].
         */
        void generate_arena(float destructible_chance = 0.75f);

        /// @brief Cells the four players start on (top-left first).
        [[nodiscard]] std::vector<TilePos> spawn_cells() const;

    private:
        /// @brief Recomputes tile_size_/origin_ after the size changes.
        void recompute_projection();

        std::size_t rows_ = 0;
        std::size_t columns_ = 0;

        float tile_size_ = 0.f;             ///< Edge length in world units
        sif::math::Point2 origin_{0.f, 0.f}; ///< World position of cell (0, 0)'s corner

        std::vector<Tile> tiles_; ///< row-major, rows_ * columns_ entries
    };
}

#endif //BOMBERMAN_LOGIC_TILEGRID_H
