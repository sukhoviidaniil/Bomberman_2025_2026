/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-12
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_POWERUPRULES_H
#define BOMBERMAN_LOGIC_POWERUPRULES_H

#include <cstddef>

#include "bomberman/logic/events/GameEvents.h"

namespace bomberman::logic {

/**
 * @brief Everything about power-ups that a designer might want to change.
 *
 * Kept out of Character and out of World: the drop odds, the caps and
 * the size of a Skates step are balance, and balance is data. The
 * alternative - the constants this replaced - meant a rebuild to answer
 * "is Fire too strong?".
 *
 * @par Why weights and not one shared chance
 * Uniform drops make Skates as common as Fire, and Skates compounds:
 * a fast bot outruns its own blast radius and becomes hard to corner.
 * Weights let the three be tuned against each other; setting one to 0
 * removes that power-up from the game entirely, which is a useful thing
 * to be able to do without touching code.
 */
struct PowerUpRules {
    /// Probability that destroying a block reveals anything at all.
    float drop_chance = 0.25f;

    // Relative odds once something does drop. Any non-negative values;
    // only their ratio matters.
    float fire_weight = 1.f;
    float extra_bomb_weight = 1.f;
    float skates_weight = 0.7f;

    /// Upper bounds, so a lucky run cannot produce an unplayable game.
    unsigned int max_blast_radius = 8;
    std::size_t max_bomb_budget = 8;

    /// Added to the character's speed, in world units per second.
    float skates_speed_bonus = 0.12f;

    /// Speed ceiling: past this a character crosses a tile faster than
    /// it can react, and the grid-snapped movement starts to overshoot.
    float max_speed = 1.1f;

    /**
     * @brief Picks a kind according to the weights.
     *
     * Draws from the shared generator (sif::intrnl::Random), so a
     * seeded run reproduces the same drops.
     *
     * @return The chosen kind; Fire if every weight is zero, because a
     * drop that has already been decided has to be *something*.
     */
    [[nodiscard]] PowerUpKind roll_kind() const;

    /// @brief Total of the three weights.
    [[nodiscard]] float total_weight() const;

    /**
     * @brief Checks the invariants the loader promises.
     *
     * @throws std::runtime_error describing the first problem found.
     */
    void validate() const;
};
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_POWERUPRULES_H
