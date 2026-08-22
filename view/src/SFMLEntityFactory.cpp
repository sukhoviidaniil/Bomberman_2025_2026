/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/view/SFMLEntityFactory.h"

#include <utility>

#include "bomberman/view/EntityViews.h"

namespace bomberman::view {
    namespace {
        /// Distinct colours so the three bots are told apart at a glance.
        const sif::intrnl::Color player_color{255, 255, 255};
        // Tints, not repaints: the art keeps its shading and the bots
        // stay instantly distinguishable from the player and each other.
        const sif::intrnl::Color bot_colors[] = {{255, 130, 130}, {130, 180, 255}, {170, 255, 150}};
    } // namespace

    SFMLEntityFactory::SFMLEntityFactory(ViewRegistry& views, const GameAssets& assets)
        : views_(views), assets_(assets) {}

    std::shared_ptr<logic::Character> SFMLEntityFactory::make_character(const logic::CharacterKind kind,
                                                                        const sif::math::Point2 position,
                                                                        const float size, const float speed) {

        auto model = std::make_shared<logic::Character>(kind == logic::CharacterKind::Player ? "Player" : "Bot",
                                                        position, size, speed, kind);

        sif::intrnl::Color color = player_color;
        if (kind == logic::CharacterKind::Bot) {
            const std::size_t index = bots_created_ % (sizeof(bot_colors) / sizeof(bot_colors[0]));
            color = bot_colors[index];
            ++bots_created_;
        }

        views_.add(std::make_shared<CharacterView>(model, assets_, color));
        return model;
    }

    std::shared_ptr<logic::Bomb> SFMLEntityFactory::make_bomb(const sif::math::Point2 position, const float size,
                                                              const logic::TilePos cell,
                                                              std::weak_ptr<logic::Character> owner,
                                                              const unsigned int radius, const float fuse_seconds) {

        auto model = std::make_shared<logic::Bomb>(position, size, cell, std::move(owner), radius, fuse_seconds);
        views_.add(std::make_shared<BombView>(model, assets_));
        return model;
    }

    std::shared_ptr<logic::Explosion> SFMLEntityFactory::make_explosion(const sif::math::Point2 position,
                                                                        const float size, const logic::TilePos cell,
                                                                        const float lifetime_seconds,
                                                                        const bool from_player) {

        auto model = std::make_shared<logic::Explosion>(position, size, cell, lifetime_seconds, from_player);
        views_.add(std::make_shared<ExplosionView>(model, assets_));
        return model;
    }

    std::shared_ptr<logic::PowerUp> SFMLEntityFactory::make_power_up(const sif::math::Point2 position, const float size,
                                                                     const logic::TilePos cell,
                                                                     const logic::PowerUpKind kind,
                                                                     const float shield_seconds) {

        auto model = std::make_shared<logic::PowerUp>(position, size, cell, kind, shield_seconds);
        views_.add(std::make_shared<PowerUpView>(model, assets_));
        return model;
    }
} // namespace bomberman::view
