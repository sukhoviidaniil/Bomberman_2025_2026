/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_BOMB_H
#define BOMBERMAN_LOGIC_BOMB_H

#include "bomberman/logic/entity/Entity.h"
#include "bomberman/logic/grid/TilePos.h"

namespace bomberman::logic {

    class Character;

    /**
     * @brief A ticking bomb sitting on one cell.
     *
     * Holds a *non-owning* pointer to the character who placed it: the
     * World owns every character, a bomb may outlive its owner (they can
     * blow themselves up), and the owner is only ever read to give back
     * a bomb slot and to attribute the kill. A shared_ptr here would keep
     * a dead character alive; a weak_ptr is the honest spelling.
     */
    class Bomb : public Entity {
    public:
        Bomb(sif::math::Point2 position, float size, TilePos cell,
             std::weak_ptr<Character> owner, unsigned int radius, float fuse_seconds);

        void update(float dt) override;

        [[nodiscard]] const TilePos& cell() const;
        [[nodiscard]] unsigned int radius() const;
        [[nodiscard]] std::weak_ptr<Character> owner() const;

        /// @brief Seconds left on the fuse.
        [[nodiscard]] float fuse_remaining() const;

        /// @brief True once the fuse ran out or a blast set it off.
        [[nodiscard]] bool detonated() const;

        /**
         * @brief Sets the bomb off immediately.
         *
         * This is what makes chain reactions possible: a blast calls it
         * on every bomb it reaches, and the World picks the newly
         * detonated bombs up on the same frame.
         */
        void detonate();

    private:
        TilePos cell_;
        std::weak_ptr<Character> owner_;
        unsigned int radius_;
        float fuse_remaining_;
        bool detonated_ = false;
        bool announced_critical_ = false;
    };
}

#endif //BOMBERMAN_LOGIC_BOMB_H
