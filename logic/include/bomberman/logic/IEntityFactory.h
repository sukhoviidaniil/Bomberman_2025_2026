/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_IENTITYFACTORY_H
#define BOMBERMAN_LOGIC_IENTITYFACTORY_H

#include <memory>

#include "bomberman/logic/entity/Bomb.h"
#include "bomberman/logic/entity/Character.h"
#include "bomberman/logic/entity/Explosion.h"
#include "bomberman/logic/entity/PowerUp.h"

namespace bomberman::logic {

/**
 * @brief Abstract Factory: how the World creates entities.
 *
 * This interface is the whole point of the pattern as the assignment
 * describes it - "The logic library defines a simple abstract factory
 * interface, which is adhered to by a concrete implementation in the
 * representation code. Finally, the Game class provides a pointer to
 * this concrete factory to the World, which can then use it to
 * produce Entities that already have the correct View attached."
 *
 * So: the World asks for a bomb, the concrete factory in the SFML
 * layer builds the model *and* the view, subscribes the view to the
 * model's bus, and hands the model back. The World never learns that
 * views exist. In Pac-Man there was no such interface at all - the
 * model constructed its own entities and the renderer re-read the
 * whole model every frame - which is why swapping the front-end there
 * would have meant touching the game rules.
 *
 * @par Why raw std::shared_ptr and not unique_ptr
 * A bomb is referenced by its owner and by the explosion that comes
 * out of it; a character is referenced by the World, by the bot AI
 * and (weakly) by every bomb it placed. Shared ownership is genuine
 * here, and the weak_ptr on the other side is what expresses "may
 * already be gone".
 */
class IEntityFactory {
public:
    virtual ~IEntityFactory() = default;

    IEntityFactory(const IEntityFactory&) = delete;
    IEntityFactory& operator=(const IEntityFactory&) = delete;

    [[nodiscard]] virtual std::shared_ptr<Character> make_character(CharacterKind kind, sif::math::Point2 position,
                                                                    float size, float speed) = 0;

    [[nodiscard]] virtual std::shared_ptr<Bomb> make_bomb(sif::math::Point2 position, float size, TilePos cell,
                                                          std::weak_ptr<Character> owner, unsigned int radius,
                                                          float fuse_seconds) = 0;

    [[nodiscard]] virtual std::shared_ptr<Explosion> make_explosion(sif::math::Point2 position, float size,
                                                                    TilePos cell, float lifetime_seconds,
                                                                    bool from_player) = 0;

    [[nodiscard]] virtual std::shared_ptr<PowerUp> make_power_up(sif::math::Point2 position, float size, TilePos cell,
                                                                 PowerUpKind kind, float shield_seconds) = 0;

protected:
    IEntityFactory() = default;
};

/**
 * @brief Factory that builds models only, with no view attached.
 *
 * Used by the tests and by any head-less run: the game rules can then
 * be exercised end to end without a window, which is the practical
 * proof that logic and representation really are separate.
 */
class HeadlessEntityFactory final : public IEntityFactory {
public:
    [[nodiscard]] std::shared_ptr<Character> make_character(CharacterKind kind, sif::math::Point2 position, float size,
                                                            float speed) override;

    [[nodiscard]] std::shared_ptr<Bomb> make_bomb(sif::math::Point2 position, float size, TilePos cell,
                                                  std::weak_ptr<Character> owner, unsigned int radius,
                                                  float fuse_seconds) override;

    [[nodiscard]] std::shared_ptr<Explosion> make_explosion(sif::math::Point2 position, float size, TilePos cell,
                                                            float lifetime_seconds, bool from_player) override;

    [[nodiscard]] std::shared_ptr<PowerUp> make_power_up(sif::math::Point2 position, float size, TilePos cell,
                                                         PowerUpKind kind, float shield_seconds) override;
};
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_IENTITYFACTORY_H
