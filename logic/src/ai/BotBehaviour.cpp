/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/ai/BotBehaviour.h"

#include <algorithm>
#include <cmath>

#include "bomberman/logic/ai/PathFinder.h"

namespace bomberman::logic::ai {
    namespace {
        /// How many steps ahead an escape search is willing to look. Beyond
        /// this a bomb is not "escapable in time" anyway, and the search
        /// stops being cheap enough to run per bot per decision.
        constexpr int escape_search_depth = 8;

        /// Safety margin on arrival times: models are never exact, and
        /// being a fifth of a second early is the difference between a bot
        /// that escapes and one that dies at the edge of the blast.
        constexpr float arrival_margin_seconds = 0.25f;
    }

    bool BotContext::passable(const TilePos &cell) const {
        if (!walkable(grid.get_tile(cell))) {
            return false;
        }
        // A bomb blocks, except the one the bot is standing on - it is
        // allowed to step off that one.
        if (has_bomb && has_bomb(cell) && !(cell == self_cell)) {
            return false;
        }
        return true;
    }

    bool BotContext::within(const TilePos &cell, const int radius) const {
        return manhattan_distance(self_cell, cell) <= radius;
    }

    const Character * BotContext::nearest_enemy() const {
        const Character* best = nullptr;
        int best_distance = 0;

        for (const auto& character : characters) {
            if (character == nullptr || !character->alive() || character.get() == &self) {
                continue;
            }
            const auto cell = grid.get_TilePos(character->position());
            if (!cell.has_value()) {
                continue;
            }
            const int distance = manhattan_distance(self_cell, *cell);
            if (best == nullptr || distance < best_distance) {
                best = character.get();
                best_distance = distance;
            }
        }
        return best;
    }

    bool BotContext::blocks_remain() const {
        for (int row = 0; row < static_cast<int>(grid.rows()); ++row) {
            for (int col = 0; col < static_cast<int>(grid.columns()); ++col) {
                if (grid.get_tile({row, col}) == Tile::Destructible) {
                    return true;
                }
            }
        }
        return false;
    }

    std::function<bool(const TilePos&)> passable_and_survivable(
        const BotContext &ctx, const TilePos &from, const DangerMap &danger) {

        const float seconds_per_step =
            ctx.tiles_per_second > 0.f ? 1.f / ctx.tiles_per_second : 1.f;

        return [&ctx, &danger, from, seconds_per_step](const TilePos& cell) {
            if (cell == from) {
                return true; // where the bot already is
            }
            if (!ctx.passable(cell)) {
                return false;
            }
            // Breadth-first search expands in step order, and on a grid the
            // Manhattan distance is that step count, so it doubles as the
            // arrival time without threading a depth through the search.
            const float arrival =
                static_cast<float>(manhattan_distance(from, cell)) * seconds_per_step
                + arrival_margin_seconds;
            return danger.safe_for(cell, arrival);
        };
    }

    Direction escape_after_bomb(const BotContext &ctx, const TilePos &from) {
        const DangerMap hypothetical = ctx.danger.with_bomb(
            ctx.grid, from, ctx.self.blast_radius(), ctx.bomb_fuse_seconds);

        // Judged against the hypothetical map, so the route out is
        // checked against the bomb being dropped *and* against everything
        // already burning. The cell the bomb goes on is exempt: the bot is
        // standing there and has not stepped off yet.
        const auto passable = passable_and_survivable(ctx, from, hypothetical);

        const float seconds_per_step =
            ctx.tiles_per_second > 0.f ? 1.f / ctx.tiles_per_second : 1.f;

        // An escape is a cell the fire never reaches **and** that the bot
        // can get to before the fuse runs out. Both halves matter, and the
        // first one is easy to get wrong: "reachable before it burns" is
        // satisfied by a cell the bot arrives at a moment before it
        // explodes, which is not an escape but a slower death. A unit test
        // caught exactly that.
        const auto goal = [&](const TilePos& cell) {
            if (cell == from) {
                return false; // standing still is not an escape
            }
            if (!hypothetical.safe(cell)) {
                return false;
            }
            const float arrival =
                static_cast<float>(manhattan_distance(from, cell)) * seconds_per_step
                + arrival_margin_seconds;
            return arrival < ctx.bomb_fuse_seconds;
        };

        const std::optional<TilePos> refuge =
            PathFinder::find_nearest(ctx.grid, from, passable, goal, escape_search_depth);

        if (!refuge.has_value()) {
            return Direction::None;
        }
        return PathFinder::first_step(ctx.grid, from, *refuge, passable);
    }

    Direction step_towards(const BotContext &ctx, const TilePos &goal) {
        const auto passable = passable_and_survivable(ctx, ctx.self_cell, ctx.danger);
        return PathFinder::first_step(ctx.grid, ctx.self_cell, goal, passable);
    }
}
