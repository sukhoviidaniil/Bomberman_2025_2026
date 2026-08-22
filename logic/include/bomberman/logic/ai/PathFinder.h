/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-01-01
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_PATHFINDER_H
#define BOMBERMAN_LOGIC_PATHFINDER_H

#include <functional>
#include <optional>
#include <vector>

#include "bomberman/logic/Direction.h"
#include "bomberman/logic/grid/TileGrid.h"
#include "bomberman/logic/grid/TilePos.h"

namespace bomberman::logic::ai {

    /**
     * @brief Answers "is this cell safe/reachable right now".
     *
     * A predicate rather than a flag on the tile: whether a cell may be
     * entered depends on things the grid does not know (a bomb standing
     * there, a blast about to cover it), and every bot behaviour the
     * assignment lists is really the same search with a different
     * predicate.
     */
    using CellPredicate = std::function<bool(const TilePos&)>;

    /**
     * @brief Breadth-first search over the arena.
     *
     * BFS and not the three-way pathfinder Pac-Man had. There, an
     * `Optimize` parameter chose between minimising and maximising the
     * distance - except two of the three implementations ignored the
     * parameter entirely and the third treated both values identically,
     * so "flee" behaved exactly like "chase". One search that does one
     * thing, plus a predicate, is both smaller and honest.
     *
     * The grid is unweighted, so BFS gives shortest paths and there is
     * nothing for A* to improve on at this size.
     *
     * TODO(daniil): A* over a weighted grid (danger as cost) is a listed
     *  bonus - "Smarter bots... by using a different search algorithm".
     *  Add it as a second implementation of the same interface rather
     *  than as a flag on this one.
     */
    class PathFinder {
    public:
        /**
         * @brief Shortest path from `from` to `to`, both inclusive.
         *
         * @return Empty if no path exists. The first element is `from`.
         */
        [[nodiscard]] static std::vector<TilePos> find_path(const TileGrid& grid, const TilePos& from,
                                                            const TilePos& to, const CellPredicate& passable);

        /**
         * @brief First step of the shortest path, as a direction.
         *
         * @return Direction::None when there is no path or it is already
         * reached - never a "best guess", because a bot that guesses is
         * indistinguishable from a bot that is broken.
         */
        [[nodiscard]] static Direction first_step(const TileGrid& grid, const TilePos& from, const TilePos& to,
                                                  const CellPredicate& passable);

        /**
         * @brief Nearest cell satisfying `goal`, searched breadth-first.
         *
         * This is the shape of every "run to safety" / "go get that
         * power-up" query: the bot does not know *which* cell it wants,
         * only what it must satisfy.
         *
         * @param max_depth Search cut-off in steps; 0 means unlimited.
         */
        [[nodiscard]] static std::optional<TilePos> find_nearest(const TileGrid& grid, const TilePos& from,
                                                                 const CellPredicate& passable,
                                                                 const CellPredicate& goal, int max_depth = 0);
    };
} // namespace bomberman::logic::ai

#endif // BOMBERMAN_LOGIC_PATHFINDER_H
