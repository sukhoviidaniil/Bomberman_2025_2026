/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-17
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/entity/Actor.h"

#include <algorithm>
#include <utility>

#include "bomberman/logic/events/GameEvents.h"

namespace bomberman::logic {

Actor::~Actor() = default;

Actor::Actor(std::string name, const sif::math::Point2 position, const float size, const float speed)
    : Entity(std::move(name), position, size), speed_(speed) {}

void Actor::set_direction(const Direction direction) { requested_direction_ = direction; }

Direction Actor::direction() const { return current_direction_; }
Direction Actor::requested_direction() const { return requested_direction_; }
bool Actor::moving() const { return moving_; }
float Actor::speed() const { return speed_; }

void Actor::set_speed(const float speed) { speed_ = std::max(0.f, speed); }

bool Actor::can_enter(const TilePos& cell, const TileGrid& grid) const { return walkable(grid.get_tile(cell)); }

void Actor::announce_motion(const bool moving) {
    if (moving == moving_) {
        return;
    }
    moving_ = moving;
    bus_->emit(entity_events::MotionChanged{moving_, current_direction_});
}

void Actor::move(const float dt, const TileGrid& grid) {
    if (speed_ <= 0.f || dt <= 0.f) {
        announce_motion(false);
        return;
    }

    const sif::math::Point2 start = position_;

    float remaining = speed_ * dt;
    // Distances below this count as "exactly at the centre"; scaled by
    // the tile size so it stays meaningful whatever the arena size.
    const float eps = grid.tile_size() * 0.001f;

    while (remaining > eps) {
        const auto cell = grid.get_TilePos(position_);
        if (!cell.has_value()) {
            break; // outside the arena: nothing sensible to do
        }

        const sif::math::Point2 center = grid.get_center(*cell);
        sif::math::Vector2 to_center(center - position_);
        float dist_to_center = to_center.length();

        if (dist_to_center <= eps) {
            position_ = center;
            dist_to_center = 0.f;
        }

        // 1. Finish crossing towards the centre of the current cell.
        if (dist_to_center > 0.f) {
            const sif::math::Vector2 heading = to_vector(current_direction_);
            if (to_center.dot(heading) > 0.f) {
                const float step = std::min(dist_to_center, remaining);
                position_ += (to_center / dist_to_center * step).to_Point2();
                remaining -= step;

                if (step < dist_to_center) {
                    continue; // still between cells
                }
                position_ = center;
                dist_to_center = 0.f;
            }
        }

        // 2. At a centre the requested direction may be honoured.
        if (requested_direction_ != current_direction_ && requested_direction_ != Direction::None) {
            if (can_enter(grid.neighbour(*cell, requested_direction_), grid)) {
                current_direction_ = requested_direction_;
                position_ = center;
            }
        }

        // 3. Step into the next cell, if it lets us in.
        const TilePos next_cell = grid.neighbour(*cell, current_direction_);
        if (current_direction_ == Direction::None || !can_enter(next_cell, grid)) {
            position_ = center;
            break;
        }

        const sif::math::Point2 next_center = grid.get_center(next_cell);
        const sif::math::Vector2 to_next(next_center - position_);
        const float dist_next = to_next.length();
        if (dist_next <= eps) {
            position_ = next_center;
            continue;
        }

        const float step = std::min(dist_next, remaining);
        position_ += (to_next / dist_next * step).to_Point2();
        remaining -= step;
    }

    const bool advanced = std::abs(position_.x - start.x) > eps || std::abs(position_.y - start.y) > eps;

    announce_motion(advanced);
    if (advanced) {
        bus_->emit(entity_events::Moved{position_, current_direction_});
    }
}
} // namespace bomberman::logic
