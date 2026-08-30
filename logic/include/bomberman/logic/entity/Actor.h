/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-13
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_ACTOR_H
#define BOMBERMAN_LOGIC_ACTOR_H

#include "bomberman/logic/Direction.h"
#include "bomberman/logic/entity/Entity.h"
#include "bomberman/logic/grid/TileGrid.h"

namespace bomberman::logic {

/**
 * @brief An entity that walks the grid.
 *
 * Movement is continuous, not cell-by-cell: the actor slides towards
 * the centre of its current cell, and only *at* a centre may it turn
 * or enter the next cell. That is what keeps a character aligned with
 * the corridors while still moving smoothly, and it is the one part
 * of the Pac-Man model that carried over essentially unchanged.
 *
 * The requested direction is remembered separately from the current
 * one, so pressing "up" while still crossing a cell takes effect at
 * the next centre instead of being dropped.
 */
class Actor : public Entity {
public:
    ~Actor() override;

    Actor(std::string name, sif::math::Point2 position, float size, float speed);

    /// @brief Requests a direction; applied at the next cell centre.
    void set_direction(Direction direction);

    [[nodiscard]] Direction direction() const;
    [[nodiscard]] Direction requested_direction() const;

    /// @brief True while the actor is actually advancing.
    [[nodiscard]] bool moving() const;

    [[nodiscard]] float speed() const;
    void set_speed(float speed);

    /**
     * @brief Advances the actor along the grid.
     *
     * @param dt Seconds since the previous frame.
     * @param grid Arena used for walkability tests.
     *
     * Emits entity_events::Moved when the position changes and
     * entity_events::MotionChanged when it starts or stops, so the
     * view never has to poll.
     */
    void move(float dt, const TileGrid& grid);

protected:
    /**
     * @brief May the actor enter this tile?
     *
     * Overridden by characters that can walk over a bomb they have
     * just placed ("After moving out of the bomb, the player can no
     * longer go through it").
     */
    [[nodiscard]] virtual bool can_enter(const TilePos& cell, const TileGrid& grid) const;

    Direction current_direction_ = Direction::None;
    Direction requested_direction_ = Direction::None;
    float speed_ = 0.f;
    bool moving_ = false;

private:
    void announce_motion(bool moving);
};
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_ACTOR_H
