/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_VIEW_ENTITYVIEWS_H
#define BOMBERMAN_VIEW_ENTITYVIEWS_H

#include "bomberman/view/EntityView.h"
#include "bomberman/view/GameAssets.h"

#include "bomberman/logic/entity/Bomb.h"
#include "bomberman/logic/entity/Character.h"
#include "bomberman/logic/entity/Explosion.h"
#include "bomberman/logic/entity/PowerUp.h"

/**
 * @file
 *
 * The four concrete views, together in one file because they are one
 * family: each turns "this model said X" into "draw frame Y of animation
 * Z", and they change as a group whenever the visual language of the game
 * changes.
 */
namespace bomberman::view {

    /**
     * @brief Draws a bomber: walk cycle per direction, idle, death.
     *
     * The direction and the moving/standing flag come from the model's
     * events, and they are the only two inputs the animation needs. The
     * cursor restarts whenever the animation changes, so turning a corner
     * does not drop the character into the middle of a different cycle.
     *
     * Bots reuse the player artwork with a per-bot tint - four separately
     * drawn characters would be four times the art for no gameplay
     * difference, and the tint keeps them instantly distinguishable.
     */
    class CharacterView final : public EntityView {
    public:
        CharacterView(const std::shared_ptr<logic::Character>& model, const GameAssets& assets,
                      sif::intrnl::Color tint);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        /// @brief Switches animation and restarts the cursor if it changed.
        void set_state(logic::Direction direction, bool moving, bool dead);

        const GameAssets& assets_;
        sif::intrnl::Color tint_;

        logic::Direction facing_ = logic::Direction::Down;
        bool moving_ = false;
        bool dead_ = false;

        sif::asset::AssetHandle<sif::asset::PrimitiveAnimation> current_;
    };

    /// @brief Draws a bomb; the fuse animation speeds up once it goes critical.
    class BombView final : public EntityView {
    public:
        BombView(const std::shared_ptr<logic::Bomb>& model, const GameAssets& assets);

        void update(float dt) override;
        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        sif::asset::AssetHandle<sif::asset::PrimitiveAnimation> animation_;
        bool critical_ = false;
    };

    /// @brief Draws one tile of fire; the animation plays once and fades.
    class ExplosionView final : public EntityView {
    public:
        ExplosionView(const std::shared_ptr<logic::Explosion>& model, const GameAssets& assets);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        std::weak_ptr<logic::Explosion> explosion_;
        sif::asset::AssetHandle<sif::asset::PrimitiveAnimation> animation_;
    };

    /// @brief Draws a pick-up, gently pulsing so it reads as collectable.
    class PowerUpView final : public EntityView {
    public:
        PowerUpView(const std::shared_ptr<logic::PowerUp>& model, const GameAssets& assets);

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const override;

    private:
        sif::asset::AssetHandle<void> sprite_;
    };
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_ENTITYVIEWS_H
