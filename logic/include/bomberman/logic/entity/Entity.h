/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-10-23
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_ENTITY_H
#define BOMBERMAN_LOGIC_ENTITY_H

#include <string>

#include "bomberman/logic/collision/AABB.h"

#include "sif/event/Event_Bus.h"
#include "sif/math/Point2.h"

namespace bomberman::logic {

    /**
     * @brief Base of everything the World owns.
     *
     * An entity is a named, positioned, sized box that can announce what
     * happens to it. It knows nothing about how it looks: the view
     * subscribes to the entity's bus and draws whatever it hears about,
     * which is the separation the assignment asks for and the reason a
     * second front-end would need no changes down here.
     *
     * The Pac-Man Entity carried a polymorphic HitBox pointer; a square
     * box computed from the position covers every entity in this game, so
     * the extra indirection (and the four hitbox classes behind it) is
     * gone.
     */
    class Entity {
    public:
        virtual ~Entity();

        /**
         * @param name Identifier used by views to pick a sprite.
         * @param position World position of the entity's centre.
         * @param size Full edge length of its square box, in world units.
         */
        Entity(std::string name, sif::math::Point2 position, float size);

        Entity(const Entity&) = delete;
        Entity& operator=(const Entity&) = delete;

        [[nodiscard]] const std::string& name() const;
        [[nodiscard]] sif::math::Point2 position() const;
        [[nodiscard]] float size() const;
        [[nodiscard]] AABB box() const;

        /// @brief The entity's box as a top-left/size rectangle.
        [[nodiscard]] sif::intrnl::Rect rect() const;

        /**
         * @brief True once the entity should be removed by the World.
         *
         * Entities never delete themselves; they say so and the World
         * does it at a well-defined point in the frame. Removing an
         * entity from inside its own update would invalidate the
         * iteration that is running.
         */
        [[nodiscard]] bool expired() const;
        void expire();

        /**
         * @brief Advances the entity by one frame.
         *
         * @param dt Seconds since the previous frame, from sif's
         * Delta_Timer. Every movement and every timer multiplies by it,
         * so behaviour does not depend on the frame rate.
         */
        virtual void update(float dt);

        /**
         * @brief Per-entity event bus the matching view subscribes to.
         *
         * Held as a shared_ptr because the view holds a subscription to
         * it and may outlive one frame of the model; sif::event::Observer
         * unsubscribes in its destructor, so neither side dangles.
         */
        [[nodiscard]] const std::shared_ptr<sif::event::Event_Bus>& bus() const;

    protected:
        void set_position(sif::math::Point2 position);

        sif::math::Point2 position_;
        float size_;
        std::string name_;
        bool expired_ = false;

        std::shared_ptr<sif::event::Event_Bus> bus_ = std::make_shared<sif::event::Event_Bus>();
    };
}

#endif //BOMBERMAN_LOGIC_ENTITY_H
