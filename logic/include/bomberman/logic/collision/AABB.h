/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-18
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_AABB_H
#define BOMBERMAN_LOGIC_AABB_H

#include <cmath>

#include "sif/internal/Rect.h"
#include "sif/math/Point2.h"
#include "sif/math/Vector2.h"

namespace bomberman::logic {

/**
 * @brief Axis-aligned bounding box: a centre and a half-size.
 *
 * The assignment allows collision detection with plain intersecting
 * rectangles, and for a grid game that is genuinely enough - which is
 * why the Pac-Man hierarchy of HitBox / HitBox_Circle /
 * HitBox_Shape / Separating_Axis_Theorem is not carried over. Three
 * of those four classes were never instantiated, and the two that
 * were had their half-extent and full-extent meanings swapped
 * (HitBox_Rectangle stored the full width in a field documented as a
 * half-width, and HitBox_Shape computed its maximum starting from
 * FLT_MIN, which is the smallest *positive* float).
 *
 * Here `half` is a half-size, everywhere, and there is one class.
 */
struct AABB {
    sif::math::Point2 center{0.f, 0.f};
    sif::math::Vector2 half{0.f, 0.f};

    constexpr AABB() = default;
    constexpr AABB(const sif::math::Point2 c, const sif::math::Vector2 h) : center(c), half(h) {}

    /// @brief Square box of the given full size, centred on a point.
    static constexpr AABB square(const sif::math::Point2 c, const float size) {
        return {c, {size * 0.5f, size * 0.5f}};
    }

    [[nodiscard]] constexpr float left() const { return center.x - half.x; }
    [[nodiscard]] constexpr float right() const { return center.x + half.x; }
    [[nodiscard]] constexpr float top() const { return center.y - half.y; }
    [[nodiscard]] constexpr float bottom() const { return center.y + half.y; }

    /// @brief The same box as a top-left/size rectangle, for rendering.
    ///
    /// Not constexpr: sif::intrnl::Rect's constructor is not, so
    /// marking this constexpr would only produce a warning.
    [[nodiscard]] sif::intrnl::Rect to_rect() const { return {left(), top(), half.x * 2.f, half.y * 2.f}; }

    [[nodiscard]] constexpr bool contains(const sif::math::Point2 p) const {
        return p.x >= left() && p.x <= right() && p.y >= top() && p.y <= bottom();
    }
};

/**
 * @brief True when two boxes overlap.
 *
 * Touching edges do not count as an overlap: two characters standing
 * in adjacent cells share an edge, and treating that as a collision
 * would make every neighbour lethal.
 */
[[nodiscard]] constexpr bool intersects(const AABB& a, const AABB& b) {
    return a.left() < b.right() && a.right() > b.left() && a.top() < b.bottom() && a.bottom() > b.top();
}

/**
 * @brief True when the boxes overlap by more than `tolerance`.
 *
 * Used for "did this character get caught by the blast" style checks,
 * where a grazing overlap of a fraction of a pixel should not count.
 */
[[nodiscard]] inline bool intersects(const AABB& a, const AABB& b, const float tolerance) {
    const float overlap_x = std::min(a.right(), b.right()) - std::max(a.left(), b.left());
    const float overlap_y = std::min(a.bottom(), b.bottom()) - std::max(a.top(), b.top());
    return overlap_x > tolerance && overlap_y > tolerance;
}
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_AABB_H
