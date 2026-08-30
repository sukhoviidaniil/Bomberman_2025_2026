/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/IEntityFactory.h"

#include <utility>

namespace bomberman::logic {

std::shared_ptr<Character> HeadlessEntityFactory::make_character(const CharacterKind kind,
                                                                 const sif::math::Point2 position, const float size,
                                                                 const float speed) {
    return std::make_shared<Character>(kind == CharacterKind::Player ? "Player" : "Bot", position, size, speed, kind);
}

std::shared_ptr<Bomb> HeadlessEntityFactory::make_bomb(const sif::math::Point2 position, const float size,
                                                       const TilePos cell, std::weak_ptr<Character> owner,
                                                       const unsigned int radius, const float fuse_seconds) {
    return std::make_shared<Bomb>(position, size, cell, std::move(owner), radius, fuse_seconds);
}

std::shared_ptr<Explosion> HeadlessEntityFactory::make_explosion(const sif::math::Point2 position, const float size,
                                                                 const TilePos cell, const float lifetime_seconds,
                                                                 const bool from_player) {
    return std::make_shared<Explosion>(position, size, cell, lifetime_seconds, from_player);
}

std::shared_ptr<PowerUp> HeadlessEntityFactory::make_power_up(const sif::math::Point2 position, const float size,
                                                              const TilePos cell, const PowerUpKind kind,
                                                              const float shield_seconds) {
    return std::make_shared<PowerUp>(position, size, cell, kind, shield_seconds);
}
} // namespace bomberman::logic
