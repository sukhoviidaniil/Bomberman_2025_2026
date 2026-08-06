/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/grid/TileGrid.h"

#include <algorithm>
#include <stdexcept>

#include "sif/internal/Random.h"

namespace bomberman::logic {
    namespace {
        /// The world spans [-1, 1] on both axes, i.e. two units per axis.
        constexpr float world_extent = 2.f;
    }

    TileGrid::TileGrid(const std::size_t rows, const std::size_t columns)
        : rows_(rows), columns_(columns), tiles_(rows * columns, Tile::Free) {
        if (rows == 0 || columns == 0) {
            throw std::invalid_argument("TileGrid: an arena needs at least one row and one column");
        }
        recompute_projection();
    }

    void TileGrid::recompute_projection() {
        // Square tiles: the longer axis decides the size, the shorter one
        // is centred inside [-1, 1].
        const auto longest = static_cast<float>(std::max(rows_, columns_));
        tile_size_ = world_extent / longest;

        origin_ = {
            -static_cast<float>(columns_) * tile_size_ * 0.5f,
            -static_cast<float>(rows_) * tile_size_ * 0.5f
        };
    }

    std::size_t TileGrid::rows() const { return rows_; }
    std::size_t TileGrid::columns() const { return columns_; }
    float TileGrid::tile_size() const { return tile_size_; }

    sif::intrnl::Rect TileGrid::bounds() const {
        return {
            origin_.x, origin_.y,
            static_cast<float>(columns_) * tile_size_,
            static_cast<float>(rows_) * tile_size_
        };
    }

    bool TileGrid::contains(const TilePos &pos) const {
        return pos.row >= 0 && pos.col >= 0
            && static_cast<std::size_t>(pos.row) < rows_
            && static_cast<std::size_t>(pos.col) < columns_;
    }

    Tile TileGrid::get_tile(const TilePos &pos) const {
        if (!contains(pos)) {
            // The arena is implicitly walled in. Reporting Free here (or
            // clamping, as the Pac-Man grid did) would let a blast or a
            // character escape through a corner.
            return Tile::Indestructible;
        }
        return tiles_[static_cast<std::size_t>(pos.row) * columns_ + static_cast<std::size_t>(pos.col)];
    }

    void TileGrid::set_tile(const TilePos &pos, const Tile tile) {
        if (!contains(pos)) {
            throw std::out_of_range("TileGrid::set_tile - cell outside the arena");
        }
        tiles_[static_cast<std::size_t>(pos.row) * columns_ + static_cast<std::size_t>(pos.col)] = tile;
    }

    TilePos TileGrid::clamp(TilePos pos) const {
        pos.row = std::clamp(pos.row, 0, static_cast<int>(rows_) - 1);
        pos.col = std::clamp(pos.col, 0, static_cast<int>(columns_) - 1);
        return pos;
    }

    std::optional<TilePos> TileGrid::get_TilePos(const sif::math::Point2 &pos) const {
        if (tile_size_ <= 0.f) {
            return std::nullopt;
        }

        const TilePos cell{
            static_cast<int>(std::floor((pos.y - origin_.y) / tile_size_)),
            static_cast<int>(std::floor((pos.x - origin_.x) / tile_size_))
        };

        if (!contains(cell)) {
            return std::nullopt;
        }
        return cell;
    }

    TilePos TileGrid::neighbour(const TilePos &pos, const Direction dir) const {
        switch (dir) {
            case Direction::Up:    return {pos.row - 1, pos.col};
            case Direction::Down:  return {pos.row + 1, pos.col};
            case Direction::Left:  return {pos.row, pos.col - 1};
            case Direction::Right: return {pos.row, pos.col + 1};
            default:               return pos;
        }
    }

    sif::math::Point2 TileGrid::get_center(const TilePos &pos) const {
        return {
            origin_.x + (static_cast<float>(pos.col) + 0.5f) * tile_size_,
            origin_.y + (static_cast<float>(pos.row) + 0.5f) * tile_size_
        };
    }

    sif::intrnl::Rect TileGrid::get_rect(const TilePos &pos) const {
        return {
            origin_.x + static_cast<float>(pos.col) * tile_size_,
            origin_.y + static_cast<float>(pos.row) * tile_size_,
            tile_size_,
            tile_size_
        };
    }

    bool TileGrid::is_junction(const TilePos &pos, const Direction current) const {
        for (std::size_t i = 0; i < direction_count; ++i) {
            const Direction dir = by_index(i);
            if (dir == current || dir == opposite(current)) {
                continue;
            }
            if (walkable(get_tile(neighbour(pos, dir)))) {
                return true;
            }
        }
        return false;
    }

    std::vector<TilePos> TileGrid::spawn_cells() const {
        const int last_row = static_cast<int>(rows_) - 1;
        const int last_col = static_cast<int>(columns_) - 1;
        return {
            {0, 0},
            {0, last_col},
            {last_row, 0},
            {last_row, last_col}
        };
    }

    void TileGrid::generate_arena(const float destructible_chance) {
        const float chance = std::clamp(destructible_chance, 0.f, 1.f);

        // 1. The fixed lattice: a pillar wherever both coordinates are
        //    odd, which is what gives Bomberman its recognisable shape.
        for (int row = 0; row < static_cast<int>(rows_); ++row) {
            for (int col = 0; col < static_cast<int>(columns_); ++col) {
                const bool pillar = (row % 2 == 1) && (col % 2 == 1);
                set_tile({row, col}, pillar ? Tile::Indestructible : Tile::Free);
            }
        }

        // 2. Destructible fill, with a chance of leaving air instead.
        for (int row = 0; row < static_cast<int>(rows_); ++row) {
            for (int col = 0; col < static_cast<int>(columns_); ++col) {
                const TilePos cell{row, col};
                if (get_tile(cell) != Tile::Free) {
                    continue;
                }
                if (sif::intrnl::rand_chance(chance)) {
                    set_tile(cell, Tile::Destructible);
                }
            }
        }

        // 3. Clear each spawn and its two orthogonal neighbours, so every
        //    character can place a first bomb and still step out of the
        //    blast. Without this the game can be lost before it starts.
        for (const TilePos& spawn : spawn_cells()) {
            set_tile(spawn, Tile::Free);
            for (std::size_t i = 0; i < direction_count; ++i) {
                const TilePos next = neighbour(spawn, by_index(i));
                if (contains(next) && get_tile(next) == Tile::Destructible) {
                    set_tile(next, Tile::Free);
                }
            }
        }
    }
}
