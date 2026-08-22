/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-01-01
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/ai/PathFinder.h"

#include <algorithm>
#include <queue>
#include <unordered_map>

namespace bomberman::logic::ai {
    namespace {
        /// Shared breadth-first sweep; both public queries are views on it.
        std::unordered_map<TilePos, TilePos, TilePosHash> sweep(const TileGrid& grid, const TilePos& from,
                                                                const CellPredicate& passable,
                                                                const CellPredicate& stop, int max_depth,
                                                                std::optional<TilePos>& reached) {

            std::unordered_map<TilePos, TilePos, TilePosHash> came_from;
            std::unordered_map<TilePos, int, TilePosHash> depth;

            std::queue<TilePos> frontier;
            frontier.push(from);
            came_from.emplace(from, from);
            depth.emplace(from, 0);

            while (!frontier.empty()) {
                const TilePos current = frontier.front();
                frontier.pop();

                if (stop && stop(current)) {
                    reached = current;
                    return came_from;
                }

                if (max_depth > 0 && depth[current] >= max_depth) {
                    continue;
                }

                for (std::size_t i = 0; i < direction_count; ++i) {
                    const TilePos next = grid.neighbour(current, by_index(i));
                    if (!grid.contains(next) || came_from.contains(next)) {
                        continue;
                    }
                    if (!passable(next)) {
                        continue;
                    }
                    came_from.emplace(next, current);
                    depth.emplace(next, depth[current] + 1);
                    frontier.push(next);
                }
            }

            return came_from;
        }

        std::vector<TilePos> rebuild(const std::unordered_map<TilePos, TilePos, TilePosHash>& came_from,
                                     const TilePos& from, const TilePos& to) {

            if (!came_from.contains(to)) {
                return {};
            }

            std::vector<TilePos> path;
            for (TilePos at = to; at != from; at = came_from.at(at)) {
                path.push_back(at);
            }
            path.push_back(from);
            std::reverse(path.begin(), path.end());
            return path;
        }
    } // namespace

    std::vector<TilePos> PathFinder::find_path(const TileGrid& grid, const TilePos& from, const TilePos& to,
                                               const CellPredicate& passable) {

        if (from == to) {
            return {from};
        }

        std::optional<TilePos> reached;
        const auto came_from = sweep(grid, from, passable, [&to](const TilePos& c) { return c == to; }, 0, reached);

        return rebuild(came_from, from, to);
    }

    Direction PathFinder::first_step(const TileGrid& grid, const TilePos& from, const TilePos& to,
                                     const CellPredicate& passable) {

        const std::vector<TilePos> path = find_path(grid, from, to, passable);
        if (path.size() < 2) {
            return Direction::None;
        }

        const TilePos step = path[1];
        for (std::size_t i = 0; i < direction_count; ++i) {
            const Direction dir = by_index(i);
            if (grid.neighbour(from, dir) == step) {
                return dir;
            }
        }
        return Direction::None;
    }

    std::optional<TilePos> PathFinder::find_nearest(const TileGrid& grid, const TilePos& from,
                                                    const CellPredicate& passable, const CellPredicate& goal,
                                                    const int max_depth) {

        std::optional<TilePos> reached;
        (void)sweep(grid, from, passable, goal, max_depth, reached);
        return reached;
    }
} // namespace bomberman::logic::ai
