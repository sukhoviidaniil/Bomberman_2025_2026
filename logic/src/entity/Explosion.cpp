/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/entity/Explosion.h"

#include <algorithm>

namespace bomberman::logic {

    Explosion::Explosion(const sif::math::Point2 position, const float size, const TilePos cell,
                         const float lifetime_seconds, const bool from_player)
        : Entity("Explosion", position, size), cell_(cell), lifetime_(std::max(0.001f, lifetime_seconds)),
          from_player_(from_player) {}

    void Explosion::update(const float dt) {
        elapsed_ += dt;
        if (elapsed_ >= lifetime_) {
            expire();
        }
    }

    const TilePos& Explosion::cell() const {
        return cell_;
    }

    float Explosion::progress() const {
        return std::clamp(elapsed_ / lifetime_, 0.f, 1.f);
    }

    bool Explosion::from_player() const {
        return from_player_;
    }
} // namespace bomberman::logic
