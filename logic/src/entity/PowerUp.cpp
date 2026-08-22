/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/entity/PowerUp.h"

#include <algorithm>

namespace bomberman::logic {

    PowerUp::PowerUp(const sif::math::Point2 position, const float size, const TilePos cell, const PowerUpKind kind,
                     const float shield_seconds)
        : Entity("PowerUp", position, size), cell_(cell), kind_(kind),
          shield_remaining_(std::max(0.f, shield_seconds)) {}

    void PowerUp::update(const float dt) {
        if (shield_remaining_ > 0.f) {
            shield_remaining_ = std::max(0.f, shield_remaining_ - dt);
        }
    }

    bool PowerUp::shielded() const {
        return shield_remaining_ > 0.f;
    }

    PowerUpKind PowerUp::kind() const {
        return kind_;
    }
    const TilePos& PowerUp::cell() const {
        return cell_;
    }
} // namespace bomberman::logic
