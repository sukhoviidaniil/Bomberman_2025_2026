/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_CHARACTER_H
#define BOMBERMAN_LOGIC_CHARACTER_H

#include <cstddef>
#include <functional>

#include "bomberman/logic/PowerUpRules.h"
#include "bomberman/logic/entity/Actor.h"
#include "bomberman/logic/events/GameEvents.h"

namespace bomberman::logic {

    /**
     * @brief A bomber: the player or one of the three bots.
     *
     * Holds the three stats the power-ups modify (blast radius, bomb
     * budget, speed) and the bookkeeping needed to walk off a freshly
     * placed bomb exactly once.
     */
    class Character : public Actor {
    public:
        Character(std::string name, sif::math::Point2 position, float size, float speed, CharacterKind kind);

        [[nodiscard]] CharacterKind kind() const;
        [[nodiscard]] bool alive() const;

        /// @brief Kills the character and announces it to its view.
        void kill();

        // ===== Stats =====

        [[nodiscard]] unsigned int blast_radius() const;
        [[nodiscard]] std::size_t bomb_budget() const;
        [[nodiscard]] std::size_t bombs_placed() const;

        /// @brief True while the character may place another bomb.
        [[nodiscard]] bool can_place_bomb() const;

        /// @brief Called by the World when a bomb of this character is created/exploded.
        void on_bomb_placed();
        void on_bomb_exploded();

        /**
         * @brief Applies a power-up permanently.
         *
         * The stat changes live here rather than in the World so that a
         * bot which picks one up automatically "understands" it - the
         * assignment asks for exactly that ("If they have a bigger bomb
         * radius, they should understand that they need to escape further
         * away"): the AI reads blast_radius() and gets the new value.
         */
        void apply(PowerUpKind kind);

        /**
         * @brief Installs the balance a power-up is applied against.
         *
         * Held per character rather than looked up globally, so a future
         * mode ("the player caps out later than the bots") is a different
         * value rather than a different code path. Defaults to the rules'
         * own defaults, which is what keeps a Character usable in a test
         * without a configuration file.
         */
        void set_power_up_rules(const PowerUpRules& rules);

        [[nodiscard]] const PowerUpRules& power_up_rules() const;

        // ===== Standing on a bomb =====

        /**
         * @brief Marks the cell the character is currently allowed to
         * walk out of even though a bomb sits there.
         *
         * Cleared automatically once the character has left it, which is
         * what turns the bomb solid behind them.
         */
        void allow_leaving(const TilePos& cell);
        [[nodiscard]] bool may_pass(const TilePos& cell) const;
        void forget_leaving();

        /**
         * @brief Installs the "is this cell blocked by an entity" question.
         *
         * Movement needs to know about bombs, and a Character has no view
         * of the world's entity lists. A predicate supplied by the World
         * keeps that knowledge where it belongs instead of handing every
         * character a back-reference to the whole game.
         */
        void set_obstacle_check(std::function<bool(const TilePos&)> check);

    protected:
        [[nodiscard]] bool can_enter(const TilePos& cell, const TileGrid& grid) const override;

    private:
        CharacterKind kind_;
        bool alive_ = true;

        unsigned int blast_radius_ = 1;
        std::size_t bomb_budget_ = 1;
        std::size_t bombs_placed_ = 0;

        bool has_pass_cell_ = false;
        TilePos pass_cell_{};

        std::function<bool(const TilePos&)> obstacle_check_;
        PowerUpRules rules_;
    };
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_CHARACTER_H
