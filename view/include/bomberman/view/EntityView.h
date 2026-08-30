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

#include "sif/asset/AssetHandle.h"
#include "sif/asset/internal/PrimitiveAnimation.h"
#include "sif/event/Observer.h"
#include "sif/internal/Color.h"
#include "sif/render/Camera.h"
#include "sif/render/RenderFrame.h"

namespace bomberman::view {

/**
 * @brief What one entity looks like.
 *
 * A view is an Observer of *its own* model's bus, attached at
 * construction by the concrete factory - exactly as the assignment
 * prescribes ("By attaching the View observers to the Model subjects
 * directly when they are created in your concrete factory, you can
 * separate the logic from the SFML representation completely
 * transparently").
 *
 * @par What is pushed and what is pulled
 * State changes - a new direction, a death, a fuse going critical -
 * arrive as events, because they decide which animation plays. The
 * plain position is read from the model each frame instead: it changes
 * every tick anyway, so an event per frame per entity would be pure
 * overhead.
 *
 * @par Animation
 * Each view owns its playback cursor and advances it in update(). The
 * frame it maps to is answered by the asset (PrimitiveAnimation), so
 * the timing rules - wrap or hold on the last frame - live in one
 * unit-tested place instead of being reimplemented per entity.
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

    /// @brief Advances this view's animation cursor.
    virtual void update(float dt);

    /**
     * @brief Adds this entity's primitives to the frame.
     *
     * @param frame Draw list being assembled.
     * @param camera Projects normalized world coordinates to pixels.
     */
    virtual void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const = 0;

protected:
    /**
     * @brief Draws one frame of an animation over this entity.
     *
     * The sprite keeps its own aspect ratio rather than being squashed
     * into the entity's square box: a 60x94 character drawn as a square
     * looks wrong, and a character whose feet drift off the tile looks
     * worse. So the width follows the entity, the height follows the
     * artwork, and the sprite is anchored by its feet to the bottom of
     * the box (`anchor_bottom`) or by its middle (for bombs and fire).
     *
     * Draws nothing while the asset is still loading.
     *
     * @param elapsed Playback cursor, in seconds.
     * @param width_scale Sprite width as a multiple of the entity size.
     */
    void append_animation(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                          const sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>& animation, float elapsed,
                          float width_scale, bool anchor_bottom, sif::intrnl::Color tint = {}) const;

    /// @brief Draws a whole SpriteSingle centred on this entity.
    void append_sprite(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                       const sif::asset::AssetHandle<void>& sprite, float width_scale,
                       sif::intrnl::Color tint = {}) const;

    std::weak_ptr<logic::Entity> model_;

    /// @brief Seconds since this view's current animation started.
    float elapsed_ = 0.f;
};

/**
 * @brief Owns the views for one round.
 *
 * Views are created by the factory together with their models, but
 * something has to keep them alive, tick them and iterate them when
 * drawing; that is this. Orphaned views are reaped on each update, so
 * nothing has to notify anyone when the World removes an entity.
 */
class ViewRegistry {
public:
    void add(std::shared_ptr<EntityView> view);
    void clear();

    /// @brief Ticks every view and drops the ones whose model is gone.
    void update(float dt);

    void append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const;

    [[nodiscard]] std::size_t size() const;

private:
    std::vector<std::shared_ptr<EntityView>> views_;
};
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_ENTITYVIEW_H
