/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/view/EntityViews.h"

#include <cmath>

#include "bomberman/logic/events/GameEvents.h"
#include "bomberman/view/AssetNames.h"

namespace bomberman::view {
namespace {
/// Characters are drawn slightly wider than their collision box so
/// they overlap the tile edges a little, the way the art expects.
constexpr float character_scale = 1.15f;
constexpr float bomb_scale = 1.0f;
constexpr float explosion_scale = 1.6f;
constexpr float item_scale = 1.0f;
} // namespace

// ================= CharacterView =================

CharacterView::CharacterView(const std::shared_ptr<logic::Character>& model, const GameAssets& assets,
                             const sif::intrnl::Color tint)
    : EntityView(model), assets_(assets), tint_(tint) {

    current_ = assets_.player_idle(facing_);

    track(model->bus()->subscribe<logic::entity_events::MotionChanged>(
        [this](const logic::entity_events::MotionChanged& e) {
            set_state(e.direction == logic::Direction::None ? facing_ : e.direction, e.moving, dead_);
        }));

    track(model->bus()->subscribe<logic::entity_events::Moved>([this](const logic::entity_events::Moved& e) {
        if (e.direction != logic::Direction::None) {
            set_state(e.direction, true, dead_);
        }
    }));

    track(model->bus()->subscribe<logic::entity_events::Died>(
        [this](const logic::entity_events::Died&) { set_state(facing_, false, true); }));
}

void CharacterView::set_state(const logic::Direction direction, const bool moving, const bool dead) {
    const bool changed = direction != facing_ || moving != moving_ || dead != dead_;
    facing_ = direction;
    moving_ = moving;
    dead_ = dead;

    if (!changed) {
        return;
    }

    current_ = dead_ ? assets_.animation(assets::player_die)
                     : (moving_ ? assets_.player_walk(facing_) : assets_.player_idle(facing_));

    // Restart, so a change of direction begins the new cycle at its
    // first frame instead of wherever the previous one happened to be.
    elapsed_ = 0.f;
}

void CharacterView::append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const {
    append_animation(frame, camera, current_, elapsed_, character_scale, true, tint_);
}

// ================= BombView =================

BombView::BombView(const std::shared_ptr<logic::Bomb>& model, const GameAssets& assets)
    : EntityView(model), animation_(assets.animation(assets::bomb)) {

    track(model->bus()->subscribe<logic::entity_events::FuseCritical>(
        [this](const logic::entity_events::FuseCritical&) { critical_ = true; }));
}

void BombView::update(const float dt) {
    // Twice as fast once the fuse is critical: the same asset conveys
    // urgency without a second animation, and because it is the model
    // that decides when "critical" starts, the tell stays in step with
    // the actual explosion.
    EntityView::update(critical_ ? dt * 2.f : dt);
}

void BombView::append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const {
    append_animation(frame, camera, animation_, elapsed_, bomb_scale, false);
}

// ================= ExplosionView =================

ExplosionView::ExplosionView(const std::shared_ptr<logic::Explosion>& model, const GameAssets& assets)
    : EntityView(model), explosion_(model), animation_(assets.animation(assets::explosion)) {}

void ExplosionView::append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const {
    const auto explosion = explosion_.lock();
    if (explosion == nullptr || !animation_.ready()) {
        return;
    }

    const std::shared_ptr<sif::asset::PrimitiveAnimation> asset = animation_.lock();
    if (asset == nullptr || asset->frame_count() == 0) {
        return;
    }

    // Driven by the model's own progress rather than by this view's
    // cursor, so the fire finishes exactly when the entity does however
    // long the configured explosion lasts.
    const float duration = asset->frame_duration_seconds() * static_cast<float>(asset->frame_count());
    append_animation(frame, camera, animation_, explosion->progress() * duration, explosion_scale, false);
}

// ================= PowerUpView =================

PowerUpView::PowerUpView(const std::shared_ptr<logic::PowerUp>& model, const GameAssets& assets)
    : EntityView(model), sprite_(assets.item(model->kind())) {}

void PowerUpView::append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const {
    // A slow breathing motion; a static icon on a static tile is easy
    // to walk past without noticing.
    const float pulse = 1.f + 0.08f * std::sin(elapsed_ * 3.f);
    append_sprite(frame, camera, sprite_, item_scale * pulse);
}
} // namespace bomberman::view
