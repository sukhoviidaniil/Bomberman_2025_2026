/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_POWERUP_H
#define BOMBERMAN_LOGIC_POWERUP_H

#include "bomberman/logic/entity/Entity.h"
#include "bomberman/logic/events/GameEvents.h"
#include "bomberman/logic/grid/TilePos.h"

namespace bomberman::logic {

    /**
     * @brief A pick-up revealed by destroying a block.
     *
     * Passive: it sits on a cell until a character walks over it or a
     * blast reaches it ("Explosions also destroy any power-ups within
     * range").
     */
    class PowerUp : public Entity {
    public:
        PowerUp(sif::math::Point2 position, float size, TilePos cell, PowerUpKind kind);

        [[nodiscard]] PowerUpKind kind() const;
        [[nodiscard]] const TilePos& cell() const;

    private:
        TilePos cell_;
        PowerUpKind kind_;
    };
}

#endif //BOMBERMAN_LOGIC_POWERUP_H
