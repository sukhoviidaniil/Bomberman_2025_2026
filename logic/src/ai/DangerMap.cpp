/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/ai/DangerMap.h"

#include <algorithm>

namespace bomberman::logic::ai {

    void DangerMap::rebuild(const TileGrid &grid,
                            const std::vector<std::shared_ptr<Bomb>> &bombs,
                            const std::vector<std::shared_ptr<Explosion>> &explosions) {
        rows_ = static_cast<int>(grid.rows());
        columns_ = static_cast<int>(grid.columns());
        seconds_.assign(static_cast<std::size_t>(rows_ * columns_), never);

        for (const auto& bomb : bombs) {
            if (bomb == nullptr || bomb->expired()) {
                continue;
            }
            // A bomb already set off by a chain reaction is about to burn,
            // not in two seconds' time.
            const float fuse = bomb->detonated() ? 0.f : std::max(0.f, bomb->fuse_remaining());
            add_bomb(grid, bomb->cell(), bomb->radius(), fuse);
        }

        // Fire already on the ground is danger in zero seconds. No ray
        // tracing here: an Explosion is one tile, and the cross it came
        // from is already gone.
        for (const auto& explosion : explosions) {
            if (explosion != nullptr && !explosion->expired()) {
                mark(explosion->cell(), 0.f);
            }
        }
    }

    DangerMap DangerMap::with_bomb(const TileGrid &grid, const TilePos &cell,
                                   const unsigned int radius, const float fuse_seconds) const {
        DangerMap copy = *this;
        copy.add_bomb(grid, cell, radius, fuse_seconds);
        return copy;
    }

    void DangerMap::add_bomb(const TileGrid &grid, const TilePos &cell,
                             const unsigned int radius, const float fuse_seconds) {
        mark(cell, fuse_seconds);

        // The same rules as World::spread_blast, deliberately: a bot that
        // models the blast differently from the game is worse than a bot
        // with no model at all.
        for (std::size_t i = 0; i < direction_count; ++i) {
            const Direction dir = by_index(i);

            for (unsigned int step = 1; step <= radius; ++step) {
                const TilePos target{
                    cell.row + static_cast<int>(to_vector(dir).y) * static_cast<int>(step),
                    cell.col + static_cast<int>(to_vector(dir).x) * static_cast<int>(step)
                };

                const Tile tile = grid.get_tile(target);
                if (tile == Tile::Indestructible) {
                    break; // the blast stops dead
                }

                mark(target, fuse_seconds);

                if (tile == Tile::Destructible) {
                    break; // absorbed: the block burns, the cell behind it does not
                }
            }
        }
    }

    void DangerMap::mark(const TilePos &cell, const float seconds) {
        if (!contains(cell)) {
            return;
        }
        float& current = seconds_[index(cell)];
        current = std::min(current, seconds);
    }

    float DangerMap::seconds_until_blast(const TilePos &cell) const {
        if (!contains(cell)) {
            return never;
        }
        return seconds_[index(cell)];
    }

    bool DangerMap::safe(const TilePos &cell) const {
        return seconds_until_blast(cell) == never;
    }

    bool DangerMap::safe_for(const TilePos &cell, const float seconds) const {
        return seconds_until_blast(cell) > seconds;
    }

    std::size_t DangerMap::index(const TilePos &cell) const {
        return static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(columns_)
             + static_cast<std::size_t>(cell.col);
    }

    bool DangerMap::contains(const TilePos &cell) const {
        return cell.row >= 0 && cell.col >= 0 && cell.row < rows_ && cell.col < columns_;
    }
}
