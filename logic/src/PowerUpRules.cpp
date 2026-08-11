/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-12
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/PowerUpRules.h"

#include <stdexcept>

#include "sif/internal/Random.h"

namespace bomberman::logic {

    float PowerUpRules::total_weight() const {
        return fire_weight + extra_bomb_weight + skates_weight;
    }

    PowerUpKind PowerUpRules::roll_kind() const {
        const float total = total_weight();
        if (total <= 0.f) {
            // Every weight is zero but a drop was already decided. Refusing
            // to produce anything here would mean drop_chance and the
            // weights could silently contradict each other.
            return PowerUpKind::Fire;
        }

        // Walk the cumulative weights. Written as an explicit chain rather
        // than as an index cast into the enum: a cast quietly breaks the
        // moment somebody reorders or extends PowerUpKind, and nothing
        // would report it.
        float roll = sif::intrnl::rand_float(0.f, total);

        if (roll < fire_weight) {
            return PowerUpKind::Fire;
        }
        roll -= fire_weight;

        if (roll < extra_bomb_weight) {
            return PowerUpKind::ExtraBomb;
        }
        return PowerUpKind::Skates;
    }

    void PowerUpRules::validate() const {
        if (drop_chance < 0.f || drop_chance > 1.f) {
            throw std::runtime_error("config: power_ups.drop_chance must be within [0, 1]");
        }
        if (fire_weight < 0.f || extra_bomb_weight < 0.f || skates_weight < 0.f) {
            throw std::runtime_error("config: power_ups weights must not be negative");
        }
        if (max_blast_radius < 1) {
            throw std::runtime_error("config: power_ups.max_blast_radius must be at least 1");
        }
        if (max_bomb_budget < 1) {
            throw std::runtime_error("config: power_ups.max_bomb_budget must be at least 1");
        }
        if (skates_speed_bonus < 0.f) {
            throw std::runtime_error("config: power_ups.skates_speed_bonus must not be negative");
        }
        if (max_speed <= 0.f) {
            throw std::runtime_error("config: power_ups.max_speed must be positive");
        }
    }
}
