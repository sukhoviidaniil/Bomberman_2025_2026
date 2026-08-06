/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_EXPLOSION_H
#define BOMBERMAN_LOGIC_EXPLOSION_H

#include "bomberman/logic/entity/Entity.h"
#include "bomberman/logic/grid/TilePos.h"

namespace bomberman::logic {

    /**
     * @brief One tile of fire, alive for a fraction of a second.
     *
     * A blast is modelled as several of these rather than as one
     * cross-shaped object: each tile then has its own box for collision,
     * its own animation phase, and the cross can be interrupted per
     * direction by a wall without any special-casing.
     */
    class Explosion : public Entity {
    public:
        Explosion(sif::math::Point2 position, float size, TilePos cell,
                  float lifetime_seconds, bool from_player);

        void update(float dt) override;

        [[nodiscard]] const TilePos& cell() const;

        /// @brief 0 at ignition, 1 when it fades out; drives the animation.
        [[nodiscard]] float progress() const;

        /// @brief True when the player's own bomb caused this tile of fire.
        [[nodiscard]] bool from_player() const;

    private:
        TilePos cell_;
        float lifetime_;
        float elapsed_ = 0.f;
        bool from_player_;
    };
}

#endif //BOMBERMAN_LOGIC_EXPLOSION_H
