/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_BEHAVIOURS_H
#define BOMBERMAN_LOGIC_BEHAVIOURS_H

#include "bomberman/logic/ai/BotBehaviour.h"

/**
 * @file
 *
 * The five behaviours, one per line of the assignment's AI list, plus a
 * fallback so a bot is never simply frozen.
 *
 * They are in one file because they are one family: each is a short
 * `decide()` that asks the shared helpers a question, and they are read and
 * changed together when the AI is tuned.
 */
namespace bomberman::logic::ai {

    /**
     * @brief "Basic survival instincts; if a bomb is going to blow them up,
     * they should attempt to run to safety."
     *
     * Highest priority in every personality: nothing a bot might want is
     * worth dying for, and a bot that collects a power-up inside a blast
     * looks broken in a way that a merely passive bot does not.
     */
    class SurviveBehaviour final : public BotBehaviour {
    public:
        [[nodiscard]] std::string name() const override { return "survive"; }
        [[nodiscard]] std::optional<BotAction> decide(const BotContext& ctx) const override;
    };

    /**
     * @brief "If any power-ups are in their range, they should try to pick
     * them up."
     *
     * "Range" is a search radius rather than the whole arena: a bot that
     * crosses the map for a pick-up abandons the fight and looks aimless.
     */
    class CollectPowerUpBehaviour final : public BotBehaviour {
    public:
        explicit CollectPowerUpBehaviour(int search_radius = 8);

        [[nodiscard]] std::string name() const override { return "collect"; }
        [[nodiscard]] std::optional<BotAction> decide(const BotContext& ctx) const override;

    private:
        int search_radius_;
    };

    /**
     * @brief "If any destructible walls are in their range, they should
     * regularly place bombs next to them to increase their playfield."
     *
     * Places a bomb only when an escape route exists - see
     * escape_after_bomb. Otherwise walks to the nearest cell that touches a
     * destructible block.
     */
    class BreakBlocksBehaviour final : public BotBehaviour {
    public:
        explicit BreakBlocksBehaviour(int search_radius = 10);

        [[nodiscard]] std::string name() const override { return "break"; }
        [[nodiscard]] std::optional<BotAction> decide(const BotContext& ctx) const override;

    private:
        int search_radius_;
    };

    /**
     * @brief "If any enemies are nearby or no breakable walls remain, they
     * should try to kill them with their bombs."
     *
     * Bombs an enemy when it is in line and inside the bot's *own* blast
     * radius - so a bot that picked up Fire starts taking shots it would
     * not have taken before, which is the "bots understand power-ups"
     * requirement falling out of the stats rather than being special-cased.
     */
    class HuntBehaviour final : public BotBehaviour {
    public:
        explicit HuntBehaviour(int engage_radius = 6);

        [[nodiscard]] std::string name() const override { return "hunt"; }
        [[nodiscard]] std::optional<BotAction> decide(const BotContext& ctx) const override;

    private:
        /// @brief True if `target` is in line with `from` and within radius,
        /// with nothing but free cells in between.
        [[nodiscard]] static bool in_blast_line(const BotContext& ctx, const TilePos& from, const TilePos& target,
                                                unsigned int radius);

        int engage_radius_;
    };

    /**
     * @brief Last resort: keep moving.
     *
     * Picks a random viable direction, preferring to carry on rather than
     * turn back, so an idle bot paces instead of vibrating in place. This
     * is where the assignment's "Random ... mainly in the AI" actually gets
     * used.
     */
    class WanderBehaviour final : public BotBehaviour {
    public:
        [[nodiscard]] std::string name() const override { return "wander"; }
        [[nodiscard]] std::optional<BotAction> decide(const BotContext& ctx) const override;
    };
} // namespace bomberman::logic::ai

#endif // BOMBERMAN_LOGIC_BEHAVIOURS_H
