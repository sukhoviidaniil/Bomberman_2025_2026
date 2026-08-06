/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/entity/Bomb.h"

#include <utility>

#include "bomberman/logic/entity/Character.h"
#include "bomberman/logic/events/GameEvents.h"

namespace bomberman::logic {
    namespace {
        /// How long before the blast the view starts flashing.
        constexpr float critical_fuse_seconds = 1.f;
    }

    Bomb::Bomb(const sif::math::Point2 position, const float size, const TilePos cell,
               std::weak_ptr<Character> owner, const unsigned int radius, const float fuse_seconds)
        : Entity("Bomb", position, size)
        , cell_(cell)
        , owner_(std::move(owner))
        , radius_(radius)
        , fuse_remaining_(fuse_seconds) {
    }

    void Bomb::update(const float dt) {
        if (detonated_) {
            return;
        }

        fuse_remaining_ -= dt;

        if (!announced_critical_ && fuse_remaining_ <= critical_fuse_seconds) {
            announced_critical_ = true;
            bus_->emit(entity_events::FuseCritical{});
        }

        if (fuse_remaining_ <= 0.f) {
            detonate();
        }
    }

    const TilePos & Bomb::cell() const { return cell_; }
    unsigned int Bomb::radius() const { return radius_; }
    std::weak_ptr<Character> Bomb::owner() const { return owner_; }
    float Bomb::fuse_remaining() const { return fuse_remaining_; }
    bool Bomb::detonated() const { return detonated_; }

    void Bomb::detonate() {
        if (detonated_) {
            return; // a bomb caught by two blasts still explodes once
        }
        detonated_ = true;
        fuse_remaining_ = 0.f;
    }
}
