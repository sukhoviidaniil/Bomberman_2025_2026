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

#include <algorithm>

#include "bomberman/logic/events/GameEvents.h"

namespace bomberman::view {
    namespace {
        sif::intrnl::Color power_up_color(const logic::PowerUpKind kind) {
            switch (kind) {
                case logic::PowerUpKind::Fire:      return {230, 90, 60};
                case logic::PowerUpKind::ExtraBomb: return {90, 130, 230};
                case logic::PowerUpKind::Skates:    return {90, 220, 140};
            }
            return {200, 200, 200};
        }
    }

    // ================= CharacterView =================

    CharacterView::CharacterView(const std::shared_ptr<logic::Character> &model, const sif::intrnl::Color color)
        : EntityView(model), color_(color) {

        track(model->bus()->subscribe<logic::entity_events::MotionChanged>(
            [this](const logic::entity_events::MotionChanged& e) {
                moving_ = e.moving;
                if (e.direction != logic::Direction::None) {
                    facing_ = e.direction;
                }
            }));

        track(model->bus()->subscribe<logic::entity_events::Moved>(
            [this](const logic::entity_events::Moved& e) {
                if (e.direction != logic::Direction::None) {
                    facing_ = e.direction;
                }
            }));

        track(model->bus()->subscribe<logic::entity_events::Died>(
            [this](const logic::entity_events::Died&) {
                dead_ = true;
            }));
    }

    void CharacterView::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) const {
        sif::intrnl::Color color = color_;
        if (dead_) {
            // TODO(daniil): play the death animation here instead of
            //  greying the box out.
            color = {90, 90, 90, 160};
        }

        append_box(frame, camera, color, moving_ ? 1.f : 0.92f);
    }

    // ================= BombView =================

    BombView::BombView(const std::shared_ptr<logic::Bomb> &model)
        : EntityView(model), bomb_(model) {

        track(model->bus()->subscribe<logic::entity_events::FuseCritical>(
            [this](const logic::entity_events::FuseCritical&) {
                critical_ = true;
            }));
    }

    void BombView::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) const {
        const auto bomb = bomb_.lock();
        if (bomb == nullptr) {
            return;
        }

        // The pulse is driven by the model's own fuse, so it stays in
        // step with the explosion no matter the frame rate.
        const float pulse = critical_
            ? 0.85f + 0.15f * std::abs(std::sin(bomb->fuse_remaining() * 18.f))
            : 0.8f;

        append_box(frame, camera, sif::intrnl::Color{30, 30, 40}, pulse);
    }

    // ================= ExplosionView =================

    ExplosionView::ExplosionView(const std::shared_ptr<logic::Explosion> &model)
        : EntityView(model), explosion_(model) {
    }

    void ExplosionView::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) const {
        const auto explosion = explosion_.lock();
        if (explosion == nullptr) {
            return;
        }

        const float t = explosion->progress();
        const auto alpha = static_cast<std::uint8_t>(255.f * (1.f - t));

        append_box(frame, camera, sif::intrnl::Color{250, 170, 60, alpha}, 0.6f + 0.4f * t);
    }

    // ================= PowerUpView =================

    PowerUpView::PowerUpView(const std::shared_ptr<logic::PowerUp> &model)
        : EntityView(model), kind_(model->kind()) {
    }

    void PowerUpView::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) const {
        append_box(frame, camera, power_up_color(kind_));
    }
}
