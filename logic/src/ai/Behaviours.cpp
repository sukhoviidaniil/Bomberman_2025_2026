/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/ai/Behaviours.h"

#include <algorithm>
#include <vector>

#include "bomberman/logic/ai/PathFinder.h"

#include "sif/internal/Random.h"

namespace bomberman::logic::ai {
    namespace {
        /// How far a "run to safety" search will look.
        constexpr int refuge_search_depth = 10;

        /// Cells adjacent to `cell`, in grid order.
        std::vector<TilePos> neighbours(const TileGrid& grid, const TilePos& cell) {
            std::vector<TilePos> out;
            out.reserve(direction_count);
            for (std::size_t i = 0; i < direction_count; ++i) {
                out.push_back(grid.neighbour(cell, by_index(i)));
            }
            return out;
        }

        /// True if any neighbour of `cell` is a destructible block.
        bool touches_block(const TileGrid& grid, const TilePos& cell) {
            for (const TilePos& next : neighbours(grid, cell)) {
                if (grid.get_tile(next) == Tile::Destructible) {
                    return true;
                }
            }
            return false;
        }
    }

    // ===================== Survive =====================

    std::optional<BotAction> SurviveBehaviour::decide(const BotContext &ctx) const {
        if (ctx.danger.safe(ctx.self_cell)) {
            return std::nullopt; // nothing to run from
        }

        // The way out must itself survive: a route that crosses a cell
        // which burns before the bot gets there is not an escape.
        const auto passable = passable_and_survivable(ctx, ctx.self_cell, ctx.danger);

        // A refuge is a cell no blast reaches at all - not merely one that
        // burns later than this one. Running from a two-second fuse into a
        // three-second fuse buys a second and then dies anyway.
        const auto is_refuge = [&ctx](const TilePos& cell) {
            return cell != ctx.self_cell && ctx.danger.safe(cell);
        };

        const std::optional<TilePos> refuge = PathFinder::find_nearest(
            ctx.grid, ctx.self_cell, passable, is_refuge, refuge_search_depth);

        if (refuge.has_value()) {
            const Direction step = PathFinder::first_step(ctx.grid, ctx.self_cell, *refuge, passable);
            if (step != Direction::None) {
                return BotAction{step, false, name()};
            }
        }

        // Cornered: there is no cell the fire will not reach. Buy time -
        // step to whichever neighbour burns latest.
        //
        // Returning nothing here is *not* the harmless fallback it looks
        // like. A bot that decides nothing keeps walking in whatever
        // direction it already had, which in a blast means straight into
        // the fire. "No opinion" and "carry on" are the same thing to an
        // Actor, so survival must always have an opinion.
        Direction best = Direction::None;
        float best_time = ctx.danger.seconds_until_blast(ctx.self_cell);

        for (std::size_t i = 0; i < direction_count; ++i) {
            const Direction dir = by_index(i);
            const TilePos next = ctx.grid.neighbour(ctx.self_cell, dir);
            if (!ctx.passable(next)) {
                continue;
            }
            const float time = ctx.danger.seconds_until_blast(next);
            if (time > best_time) {
                best_time = time;
                best = dir;
            }
        }

        if (best == Direction::None) {
            return std::nullopt; // genuinely nowhere better; let the rest decide
        }

        return BotAction{best, false, name()};
    }

    // ===================== Collect =====================

    CollectPowerUpBehaviour::CollectPowerUpBehaviour(const int search_radius)
        : search_radius_(search_radius) {
    }

    std::optional<BotAction> CollectPowerUpBehaviour::decide(const BotContext &ctx) const {
        const PowerUp* best = nullptr;
        int best_distance = 0;
        TilePos best_cell{};

        for (const auto& power_up : ctx.power_ups) {
            if (power_up == nullptr || power_up->expired()) {
                continue;
            }
            const TilePos cell = power_up->cell();
            const int distance = manhattan_distance(ctx.self_cell, cell);
            if (distance > search_radius_) {
                continue;
            }
            if (best == nullptr || distance < best_distance) {
                best = power_up.get();
                best_distance = distance;
                best_cell = cell;
            }
        }

        if (best == nullptr) {
            return std::nullopt;
        }

        const Direction step = step_towards(ctx, best_cell);
        if (step == Direction::None) {
            return std::nullopt; // unreachable, or the way there is on fire
        }

        return BotAction{step, false, name()};
    }

    // ===================== Break blocks =====================

    BreakBlocksBehaviour::BreakBlocksBehaviour(const int search_radius)
        : search_radius_(search_radius) {
    }

    std::optional<BotAction> BreakBlocksBehaviour::decide(const BotContext &ctx) const {
        if (!ctx.self.can_place_bomb()) {
            return std::nullopt; // no bomb to spare; let another goal decide
        }

        // Already in position: drop a bomb, but only if there is a way out.
        if (touches_block(ctx.grid, ctx.self_cell)) {
            const Direction escape = escape_after_bomb(ctx, ctx.self_cell);
            if (escape != Direction::None) {
                // Bomb *and* start leaving in the same decision - waiting a
                // tick to move is how a bot ends up inside its own blast.
                return BotAction{escape, true, name()};
            }
            return std::nullopt;
        }

        const auto passable = passable_and_survivable(ctx, ctx.self_cell, ctx.danger);
        const auto goal = [&ctx](const TilePos& cell) { return touches_block(ctx.grid, cell); };

        const std::optional<TilePos> target = PathFinder::find_nearest(
            ctx.grid, ctx.self_cell, passable, goal, search_radius_);

        if (!target.has_value()) {
            return std::nullopt;
        }

        const Direction step = step_towards(ctx, *target);
        if (step == Direction::None) {
            return std::nullopt;
        }

        return BotAction{step, false, name()};
    }

    // ===================== Hunt =====================

    HuntBehaviour::HuntBehaviour(const int engage_radius) : engage_radius_(engage_radius) {
    }

    bool HuntBehaviour::in_blast_line(const BotContext &ctx, const TilePos &from,
                                      const TilePos &target, const unsigned int radius) {
        if (from.row != target.row && from.col != target.col) {
            return false;
        }

        const int distance = manhattan_distance(from, target);
        if (distance > static_cast<int>(radius)) {
            return false;
        }

        // distance == 0 is the best shot there is: the two are standing on
        // the same cell, and a bomb placed here cannot miss. Rejecting it
        // (as this did) meant a bot would chase its target across the arena,
        // catch it, and then stand next to it forever - the round never
        // ended, which looked like a stuck simulation rather than like a
        // one-character mistake in a bounds check.
        if (distance == 0) {
            return true;
        }

        // The blast has to actually get there: one wall in between and the
        // shot is wasted.
        const int row_step = (target.row > from.row) - (target.row < from.row);
        const int col_step = (target.col > from.col) - (target.col < from.col);

        for (int step = 1; step < distance; ++step) {
            const TilePos between{from.row + row_step * step, from.col + col_step * step};
            if (ctx.grid.get_tile(between) != Tile::Free) {
                return false;
            }
        }
        return true;
    }

    std::optional<BotAction> HuntBehaviour::decide(const BotContext &ctx) const {
        const Character* enemy = ctx.nearest_enemy();
        if (enemy == nullptr) {
            return std::nullopt;
        }

        const auto enemy_cell = ctx.grid.get_TilePos(enemy->position());
        if (!enemy_cell.has_value()) {
            return std::nullopt;
        }

        const int distance = manhattan_distance(ctx.self_cell, *enemy_cell);

        // "If any enemies are nearby **or** no breakable walls remain" -
        // once the arena is open there is nothing else left to do.
        if (distance > engage_radius_ && ctx.blocks_remain()) {
            return std::nullopt;
        }

        // In line and in range: take the shot, if there is a way out.
        if (ctx.self.can_place_bomb() &&
            in_blast_line(ctx, ctx.self_cell, *enemy_cell, ctx.self.blast_radius())) {
            const Direction escape = escape_after_bomb(ctx, ctx.self_cell);
            if (escape != Direction::None) {
                return BotAction{escape, true, name()};
            }
        }

        const Direction step = step_towards(ctx, *enemy_cell);
        if (step == Direction::None) {
            return std::nullopt;
        }

        return BotAction{step, false, name()};
    }

    // ===================== Wander =====================

    std::optional<BotAction> WanderBehaviour::decide(const BotContext &ctx) const {
        std::vector<Direction> viable;
        viable.reserve(direction_count);

        for (std::size_t i = 0; i < direction_count; ++i) {
            const Direction dir = by_index(i);
            const TilePos next = ctx.grid.neighbour(ctx.self_cell, dir);
            if (ctx.passable(next) && ctx.danger.safe(next)) {
                viable.push_back(dir);
            }
        }

        if (viable.empty()) {
            return std::nullopt;
        }

        // Prefer carrying on: turning back at every cell looks like a
        // stutter rather than like patrolling.
        const Direction current = ctx.self.direction();
        if (current != Direction::None && viable.size() > 1) {
            const Direction back = opposite(current);
            if (std::find(viable.begin(), viable.end(), current) != viable.end()
                && sif::intrnl::rand_chance(0.7f)) {
                return BotAction{current, false, name()};
            }
            std::erase(viable, back);
        }

        // next_index, not next_int(0, size - 1): on an empty list the
        // subtraction would wrap and index into nothing.
        const Direction chosen = viable[sif::intrnl::rand_index(viable.size())];
        return BotAction{chosen, false, name()};
    }
}
