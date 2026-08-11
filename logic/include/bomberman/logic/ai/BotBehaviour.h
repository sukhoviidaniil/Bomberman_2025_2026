/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_BOTBEHAVIOUR_H
#define BOMBERMAN_LOGIC_BOTBEHAVIOUR_H

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "bomberman/logic/ai/DangerMap.h"
#include "bomberman/logic/entity/Character.h"
#include "bomberman/logic/entity/PowerUp.h"
#include "bomberman/logic/grid/TileGrid.h"

namespace bomberman::logic::ai {

    /**
     * @brief What a bot decided to do this tick.
     *
     * Deliberately tiny and declarative: a decision is "steer this way and
     * maybe drop a bomb", never a direct mutation of the world. The World
     * applies it, so the AI cannot accidentally sidestep a rule (placing a
     * bomb it has no budget for, walking through a wall) - and a decision
     * can be inspected in a test without running a frame.
     */
    struct BotAction {
        Direction move = Direction::None;
        bool place_bomb = false;

        /// @brief Which behaviour produced this, for debugging and tests.
        std::string reason;
    };

    /**
     * @brief Everything a behaviour is allowed to look at.
     *
     * A read-only view of the round, assembled once per decision. Passing
     * it in rather than handing the AI a World& is what keeps a behaviour
     * from quietly gaining the ability to change the game.
     */
    struct BotContext {
        const TileGrid& grid;
        const DangerMap& danger;

        /// The bot deciding. Read for its own stats - blast radius, bomb
        /// budget, speed - which is how picking up a power-up automatically
        /// changes its behaviour.
        const Character& self;
        TilePos self_cell{};

        /// Every character, including this bot and the dead ones.
        const std::vector<std::shared_ptr<Character>>& characters;
        const std::vector<std::shared_ptr<PowerUp>>& power_ups;

        /// True where a bomb currently sits (bombs block movement).
        std::function<bool(const TilePos&)> has_bomb;

        /// Fuse length of a newly placed bomb; needed to judge escapes.
        float bomb_fuse_seconds = 2.f;

        /// How far the bot travels in a second, in tiles.
        float tiles_per_second = 1.f;

        /// @brief Can the bot walk into this cell right now?
        [[nodiscard]] bool passable(const TilePos& cell) const;

        /// @brief Cells within `radius` steps, ignoring danger.
        [[nodiscard]] bool within(const TilePos& cell, int radius) const;

        /// @brief Nearest living character other than this bot, or nullptr.
        [[nodiscard]] const Character* nearest_enemy() const;

        /// @brief Are there any destructible blocks left in the arena?
        [[nodiscard]] bool blocks_remain() const;
    };

    /**
     * @brief One thing a bot knows how to want.
     *
     * The assignment describes the AI as a list of conditions - "if a bomb
     * is going to blow them up, they should attempt to run to safety", "if
     * any power-ups are in their range, they should try to pick them up",
     * and so on. That is a *priority* of goals evaluated every decision,
     * not a state machine, so each goal is one of these and the brain asks
     * them in order until one answers.
     *
     * (The Pac-Man project modelled its ghosts as a mode FSM crossed with a
     * target strategy, which fits a game where the ghost's job never
     * changes and only its destination does. Here the job changes every few
     * seconds and the conditions overlap, so an FSM would need a transition
     * rule for every pair of goals; a priority chain reads exactly like the
     * specification instead.)
     *
     * A behaviour returns nothing when its precondition does not hold. That
     * is the whole protocol - no "am I applicable" query that could
     * disagree with the decision that follows it.
     */
    class BotBehaviour {
    public:
        virtual ~BotBehaviour() = default;

        BotBehaviour(const BotBehaviour&) = delete;
        BotBehaviour& operator=(const BotBehaviour&) = delete;

        /// @brief Human-readable name, used in BotAction::reason.
        [[nodiscard]] virtual std::string name() const = 0;

        /// @brief The action this behaviour wants, if it wants one.
        [[nodiscard]] virtual std::optional<BotAction> decide(const BotContext& ctx) const = 0;

    protected:
        BotBehaviour() = default;
    };

    // ===================================================================
    // Shared helpers - the questions more than one behaviour asks
    // ===================================================================

    /**
     * @brief Is there a reachable cell that survives, if a bomb is dropped here?
     *
     * Answered against a *hypothetical* danger map that includes the bomb
     * the bot is considering. Without this check a bot happily walls itself
     * into its own blast, which reads as a bug rather than as weak play.
     *
     * @param ctx Current situation.
     * @param from Where the bomb would be placed (the bot's own cell).
     * @return The escape route's first step, or Direction::None if there is
     * no escape - in which case the bomb must not be placed.
     */
    [[nodiscard]] Direction escape_after_bomb(const BotContext& ctx, const TilePos& from);

    /**
     * @brief A "can the bot walk here" predicate that also respects fire.
     *
     * Walls and bombs are not the only reason a cell is unusable: a cell
     * that will be burning by the time the bot arrives is just as blocked,
     * and forgetting that is subtle. A path can be perfectly walkable and
     * still lead through an explosion - which is exactly how bots died on
     * the first step of an escape route that was itself correctly chosen.
     *
     * Arrival time is estimated from the bot's own speed, so a character
     * that picked up Skates is willing to cross gaps a slower one is not.
     *
     * @param danger Which map to judge against - the real one, or the
     * hypothetical one that includes a bomb the bot is considering.
     */
    [[nodiscard]] std::function<bool(const TilePos&)> passable_and_survivable(
        const BotContext& ctx, const TilePos& from, const DangerMap& danger);

    /**
     * @brief First step of the safest short path towards a goal cell.
     *
     * "Safest" means the path only crosses cells that will still be there
     * when the bot arrives, estimated from its own speed - which is why a
     * bot that picked up Skates is willing to cross tighter gaps.
     *
     * @return Direction::None when no acceptable path exists.
     */
    [[nodiscard]] Direction step_towards(const BotContext& ctx, const TilePos& goal);
}

#endif //BOMBERMAN_LOGIC_BOTBEHAVIOUR_H
