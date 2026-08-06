/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_ENTITYVIEW_H
#define BOMBERMAN_VIEW_ENTITYVIEW_H

#include <memory>
#include <vector>

#include "bomberman/logic/entity/Entity.h"

#include "sif/event/Observer.h"
#include "sif/internal/Color.h"
#include "sif/render/Camera.h"
#include "sif/render/RenderFrame.h"

namespace bomberman::view {

    /**
     * @brief What one entity looks like.
     *
     * A view is an Observer of *its own* model's bus. It is attached at
     * construction by the concrete factory, exactly as the assignment
     * prescribes ("By attaching the View observers to the Model subjects
     * directly when they are created in your concrete factory, you can
     * separate the logic from the SFML representation completely
     * transparently").
     *
     * @par What is pushed and what is pulled
     * State changes - a new direction, a death, a fuse going critical -
     * arrive as events, because they are what decides which animation
     * plays. The plain position is read from the model each frame
     * instead: it changes every tick anyway, so an event per frame per
     * entity would be pure overhead with no extra information. Pac-Man
     * pulled *everything* (the renderer walked the whole model every
     * frame and no view subscribed to anything), which is the part being
     * corrected here.
     *
     * The model is held weakly: the World owns entities and destroys them
     * when they expire, and a view must not keep a dead bomb alive.
     */
    class EntityView : public sif::event::Observer {
    public:
        ~EntityView() override;

        explicit EntityView(const std::shared_ptr<logic::Entity>& model);

        /// @brief True once the model is gone; the registry drops the view.
        [[nodiscard]] bool orphaned() const;

        /**
         * @brief Adds this entity's primitives to the frame.
         *
         * @param frame Draw list being assembled.
         * @param camera Projects normalized world coordinates to pixels.
         */
        virtual void append_render_items(sif::rnd::RenderFrame& frame,
                                         const sif::rnd::Camera& camera) const = 0;

    protected:
        /// @brief Convenience: pushes one solid rectangle for this entity.
        void append_box(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                        sif::intrnl::Color color, float scale = 1.f) const;

        std::weak_ptr<logic::Entity> model_;
        bool dead_ = false;
    };

    /**
     * @brief Owns the views for one round.
     *
     * Views are created by the factory together with their models, but
     * somebody has to keep them alive and iterate them when drawing;
     * that is this. Orphaned views are reaped on each draw, so nothing
     * has to notify anyone when the World removes an entity.
     */
    class ViewRegistry {
    public:
        void add(std::shared_ptr<EntityView> view);
        void clear();

        void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera);

        [[nodiscard]] std::size_t size() const;

    private:
        std::vector<std::shared_ptr<EntityView>> views_;
    };
}

#endif //BOMBERMAN_VIEW_ENTITYVIEW_H
