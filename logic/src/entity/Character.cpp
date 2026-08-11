/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/entity/Character.h"

#include <algorithm>
#include <utility>

namespace bomberman::logic {
    Character::Character(std::string name, const sif::math::Point2 position, const float size,
                         const float speed, const CharacterKind kind)
        : Actor(std::move(name), position, size, speed), kind_(kind) {
    }

    CharacterKind Character::kind() const { return kind_; }
    bool Character::alive() const { return alive_; }

    void Character::kill() {
        if (!alive_) {
            return; // two blasts in one frame must not kill twice
        }
        alive_ = false;
        set_speed(0.f);
        bus_->emit(entity_events::Died{});
    }

    unsigned int Character::blast_radius() const { return blast_radius_; }
    std::size_t Character::bomb_budget() const { return bomb_budget_; }
    std::size_t Character::bombs_placed() const { return bombs_placed_; }

    bool Character::can_place_bomb() const {
        return alive_ && bombs_placed_ < bomb_budget_;
    }

    void Character::on_bomb_placed() {
        ++bombs_placed_;
    }

    void Character::on_bomb_exploded() {
        if (bombs_placed_ > 0) {
            --bombs_placed_;
        }
    }

    void Character::apply(const PowerUpKind kind) {
        // Every effect is permanent and capped. The caps are not decoration:
        // an uncapped blast radius eventually covers the whole arena, and an
        // uncapped speed lets a character cross a tile faster than the
        // grid-snapping can keep up with.
        switch (kind) {
            case PowerUpKind::Fire:
                blast_radius_ = std::min(blast_radius_ + 1, rules_.max_blast_radius);
                break;
            case PowerUpKind::ExtraBomb:
                bomb_budget_ = std::min(bomb_budget_ + 1, rules_.max_bomb_budget);
                break;
            case PowerUpKind::Skates:
                set_speed(std::min(speed() + rules_.skates_speed_bonus, rules_.max_speed));
                break;
        }
    }

    void Character::set_power_up_rules(const PowerUpRules &rules) {
        rules_ = rules;
    }

    const PowerUpRules & Character::power_up_rules() const {
        return rules_;
    }

    void Character::allow_leaving(const TilePos &cell) {
        has_pass_cell_ = true;
        pass_cell_ = cell;
    }

    bool Character::may_pass(const TilePos &cell) const {
        return has_pass_cell_ && pass_cell_ == cell;
    }

    void Character::forget_leaving() {
        has_pass_cell_ = false;
    }

    void Character::set_obstacle_check(std::function<bool(const TilePos &)> check) {
        obstacle_check_ = std::move(check);
    }

    bool Character::can_enter(const TilePos &cell, const TileGrid &grid) const {
        if (!Actor::can_enter(cell, grid)) {
            return false;
        }
        if (obstacle_check_ && obstacle_check_(cell)) {
            // The one exception: the bomb this character is currently
            // standing on, until they have stepped off it.
            return may_pass(cell);
        }
        return true;
    }
}
