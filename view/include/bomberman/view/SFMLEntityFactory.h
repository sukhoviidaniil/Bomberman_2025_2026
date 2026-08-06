/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_SFMLENTITYFACTORY_H
#define BOMBERMAN_VIEW_SFMLENTITYFACTORY_H

#include "bomberman/logic/IEntityFactory.h"
#include "bomberman/view/EntityView.h"

namespace bomberman::view {

    /**
     * @brief The concrete factory: builds a model and its view together.
     *
     * This is the half of the Abstract Factory that lives in the
     * representation layer. The World calls make_bomb(); this creates the
     * logic::Bomb, creates a BombView subscribed to that bomb's bus, and
     * registers the view for drawing. The World gets back a plain model
     * and never learns a view exists.
     *
     * Because it is handed to the World as an IEntityFactory&, replacing
     * the front-end means writing one more class here - no change to a
     * single line of game logic.
     */
    class SFMLEntityFactory final : public logic::IEntityFactory {
    public:
        explicit SFMLEntityFactory(ViewRegistry& views);

        [[nodiscard]] std::shared_ptr<logic::Character> make_character(
            logic::CharacterKind kind, sif::math::Point2 position, float size, float speed) override;

        [[nodiscard]] std::shared_ptr<logic::Bomb> make_bomb(
            sif::math::Point2 position, float size, logic::TilePos cell,
            std::weak_ptr<logic::Character> owner, unsigned int radius, float fuse_seconds) override;

        [[nodiscard]] std::shared_ptr<logic::Explosion> make_explosion(
            sif::math::Point2 position, float size, logic::TilePos cell,
            float lifetime_seconds, bool from_player) override;

        [[nodiscard]] std::shared_ptr<logic::PowerUp> make_power_up(
            sif::math::Point2 position, float size, logic::TilePos cell, logic::PowerUpKind kind) override;

    private:
        ViewRegistry& views_;
        std::size_t bots_created_ = 0;
    };
}

#endif //BOMBERMAN_VIEW_SFMLENTITYFACTORY_H
