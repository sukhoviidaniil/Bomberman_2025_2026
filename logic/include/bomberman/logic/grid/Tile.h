/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-28
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_TILE_H
#define BOMBERMAN_LOGIC_TILE_H

namespace bomberman::logic {

    /**
     * @brief What occupies one cell of the arena.
     *
     * Only the *terrain* lives here. Bombs, power-ups and characters are
     * entities in the World: they move, they are drawn by their own view,
     * and several of them can share a cell. Baking them into the tile
     * enum (as the Pac-Man grid did with GhostSpawn) means the grid has
     * to be rewritten every time an entity is added.
     */
    enum class Tile {
        Free,           ///< Walkable, nothing here.
        Indestructible, ///< The fixed pillars; blocks movement and blasts.
        Destructible    ///< Blocked until a blast turns it into Free.
    };

    /// @brief Can a character walk into this tile?
    [[nodiscard]] constexpr bool walkable(const Tile t) {
        return t == Tile::Free;
    }

    /// @brief Does a blast continue past this tile?
    ///
    /// Indestructible stops it outright; Destructible absorbs it (the
    /// assignment: "can only go through one destructible block at a time
    /// in each direction"), so it is destroyed but not passed through.
    [[nodiscard]] constexpr bool blocks_blast(const Tile t) {
        return t != Tile::Free;
    }
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_TILE_H
