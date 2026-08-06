/***************************************************************
 * Author:        Sukhovii Daniil
 * Created:       2026-08-05
 *   Email:       sukhovii.daniil@gmail.com
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_ENTITYVIEWS_H
#define BOMBERMAN_VIEW_ENTITYVIEWS_H

#include "bomberman/view/EntityView.h"

#include "bomberman/logic/entity/Bomb.h"
#include "bomberman/logic/entity/Character.h"
#include "bomberman/logic/entity/Explosion.h"
#include "bomberman/logic/entity/PowerUp.h"

/**
 * @file
 *
 * The four concrete views, together in one file because they are one
 * family: each is a dozen lines that turns "this model said X" into "draw
 * Y", and they change as a group whenever the visual language of the game
 * changes.
 *
 * TODO(daniil): all four currently draw flat coloured boxes. Replace them
 *  with sif sprites/animations once the Bomberman sprite sheet is in
 *  data/: a walking animation per direction, a death animation, a
 *  ticking-bomb animation and a growing-and-fading explosion, as section
 *  2.2 of the assignment requires. sif::ui::Animation already does the
 *  frame timing, so the work is authoring the *.asset.json descriptors,
 *  not writing animation code.
 */
namespace bomberman::view {

    /**
     * @brief Draws a bomber and reacts to its movement and death.
     *
     * Keeps `facing_` and `moving_` from the model's events; those are
     * exactly the two inputs an animated sprite needs (which direction,
     * walking or standing), so swapping the placeholder rectangle for a
     * sprite sheet touches only append_render_items.
     */
    class CharacterView final : public EntityView {
    public:
        CharacterView(const std::shared_ptr<logic::Character>& model, sif::intrnl::Color color);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        sif::intrnl::Color color_;
        logic::Direction facing_ = logic::Direction::Down;
        bool moving_ = false;
    };

    /// @brief Draws a bomb; flashes once its fuse goes critical.
    class BombView final : public EntityView {
    public:
        explicit BombView(const std::shared_ptr<logic::Bomb>& model);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        std::weak_ptr<logic::Bomb> bomb_;
        bool critical_ = false;
    };

    /// @brief Draws one tile of fire, growing and fading over its lifetime.
    class ExplosionView final : public EntityView {
    public:
        explicit ExplosionView(const std::shared_ptr<logic::Explosion>& model);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        std::weak_ptr<logic::Explosion> explosion_;
    };

    /// @brief Draws a pick-up, colour-coded per kind.
    class PowerUpView final : public EntityView {
    public:
        explicit PowerUpView(const std::shared_ptr<logic::PowerUp>& model);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        logic::PowerUpKind kind_;
    };
}

#endif //BOMBERMAN_VIEW_ENTITYVIEWS_H
