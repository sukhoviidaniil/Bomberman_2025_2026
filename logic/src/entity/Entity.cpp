/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-17
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/entity/Entity.h"

#include <utility>

namespace bomberman::logic {

Entity::~Entity() = default;

Entity::Entity(std::string name, const sif::math::Point2 position, const float size)
    : position_(position), size_(size), name_(std::move(name)) {}

const std::string& Entity::name() const { return name_; }

sif::math::Point2 Entity::position() const { return position_; }

float Entity::size() const { return size_; }

AABB Entity::box() const { return AABB::square(position_, size_); }

sif::intrnl::Rect Entity::rect() const { return box().to_rect(); }

bool Entity::expired() const { return expired_; }

void Entity::expire() { expired_ = true; }

void Entity::update(float /*dt*/) {}

const std::shared_ptr<sif::event::Event_Bus>& Entity::bus() const { return bus_; }

void Entity::set_position(const sif::math::Point2 position) { position_ = position; }
} // namespace bomberman::logic
