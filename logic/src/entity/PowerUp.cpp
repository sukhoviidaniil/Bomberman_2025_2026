/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/entity/PowerUp.h"

namespace bomberman::logic {

    PowerUp::PowerUp(const sif::math::Point2 position, const float size,
                     const TilePos cell, const PowerUpKind kind)
        : Entity("PowerUp", position, size), cell_(cell), kind_(kind) {
    }

    PowerUpKind PowerUp::kind() const { return kind_; }
    const TilePos & PowerUp::cell() const { return cell_; }
}
