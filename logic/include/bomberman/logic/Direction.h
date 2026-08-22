/***************************************************************
 * Author:        Sukhovii Daniil
 * Created:       2025-12-22
 *   Email:       sukhovii.daniil@gmail.com
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_DIRECTION_H
#define BOMBERMAN_LOGIC_DIRECTION_H

#include <cstddef>
#include <type_traits>

#include "sif/math/Vector2.h"

namespace bomberman::logic {

    /**
     * @brief The four movement axes plus "not moving".
     *
     * The Pac-Man version also had an `Any` member used as a wildcard in
     * comparisons, which made `equal(a, b)` asymmetric (`equal(Any, Up)`
     * was true, `equal(Up, Any)` was false). Nothing in this project
     * needs a wildcard direction, so it is gone; `None` covers the only
     * case that mattered - a character standing still.
     */
    enum class Direction { None, Up, Right, Down, Left };

    /// @brief Number of real (non-None) directions.
    inline constexpr std::size_t direction_count = 4;

    constexpr Direction opposite(const Direction d) {
        switch (d) {
        case Direction::Up:
            return Direction::Down;
        case Direction::Down:
            return Direction::Up;
        case Direction::Right:
            return Direction::Left;
        case Direction::Left:
            return Direction::Right;
        default:
            return Direction::None;
        }
    }

    /**
     * @brief Maps 0..3 onto Up/Right/Down/Left.
     *
     * Used together with sif's Random::next_index(direction_count) to
     * pick a direction without ever forming an out-of-range index.
     */
    constexpr Direction by_index(const std::size_t i) {
        switch (i) {
        case 0:
            return Direction::Up;
        case 1:
            return Direction::Right;
        case 2:
            return Direction::Down;
        case 3:
            return Direction::Left;
        default:
            return Direction::None;
        }
    }

    /**
     * @brief Unit vector of a direction, in world space.
     *
     * y grows downwards, matching the grid's row order and sif's default
     * camera orientation, so "Up" is -y.
     */
    constexpr sif::math::Vector2 to_vector(const Direction d) {
        switch (d) {
        case Direction::Up:
            return {0.f, -1.f};
        case Direction::Down:
            return {0.f, 1.f};
        case Direction::Left:
            return {-1.f, 0.f};
        case Direction::Right:
            return {1.f, 0.f};
        default:
            return {0.f, 0.f};
        }
    }

    struct DirectionHash {
        std::size_t operator()(const Direction d) const noexcept {
            return static_cast<std::size_t>(std::underlying_type_t<Direction>(d));
        }
    };
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_DIRECTION_H
