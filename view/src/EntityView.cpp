/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/view/EntityView.h"

#include <algorithm>
#include <utility>

#include "sif/render/elements/Sprite.h"

namespace bomberman::view {

    EntityView::~EntityView() = default;

    EntityView::EntityView(const std::shared_ptr<logic::Entity>& model) : model_(model) {}

    bool EntityView::orphaned() const {
        return model_.expired();
    }

    void EntityView::update(const float dt) {
        elapsed_ += dt;
    }

    void EntityView::append_animation(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                                      const sif::asset::AssetHandle<sif::asset::PrimitiveAnimation>& animation,
                                      const float elapsed, const float width_scale, const bool anchor_bottom,
                                      const sif::intrnl::Color tint) const {
        const auto model = model_.lock();
        if (model == nullptr || !animation.ready()) {
            return;
        }

        // lock(), not get(): the frame rectangle is read out of the asset
        // here, and a raw pointer from get() is only guaranteed for the
        // expression that produced it.
        const std::shared_ptr<sif::asset::PrimitiveAnimation> asset = animation.lock();
        if (asset == nullptr || asset->frame_count() == 0) {
            return;
        }

        const sif::intrnl::Rect src = asset->frame_at(elapsed);
        if (src.width <= 0.f || src.height <= 0.f) {
            return;
        }

        const logic::AABB box = model->box();
        const float width = model->size() * width_scale;
        const float height = width * (src.height / src.width);

        const float left = box.center.x - width * 0.5f;
        const float top = anchor_bottom ? box.bottom() - height         // feet on the tile
                                        : box.center.y - height * 0.5f; // centred, for bombs and fire

        auto item = std::make_unique<sif::rnd::Sprite>();
        item->rect = camera.world_to_screen(sif::intrnl::Rect(left, top, width, height));
        item->asset = sif::asset::AssetHandle<void>(animation.record());
        item->kind = sif::asset::AssetType::PrimitiveAnimation;
        item->src_rect = src;
        item->tint = tint;

        frame.temp_items.push_back(std::move(item));
    }

    void EntityView::append_sprite(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera,
                                   const sif::asset::AssetHandle<void>& sprite, const float width_scale,
                                   const sif::intrnl::Color tint) const {
        const auto model = model_.lock();
        if (model == nullptr || !sprite.ready()) {
            return;
        }

        const float size = model->size() * width_scale;
        const logic::AABB box{model->position(), {size * 0.5f, size * 0.5f}};

        auto item = std::make_unique<sif::rnd::Sprite>();
        item->rect = camera.world_to_screen(box.to_rect());
        item->asset = sprite;
        item->kind = sif::asset::AssetType::SpriteSingle;
        item->tint = tint;

        frame.temp_items.push_back(std::move(item));
    }

    void ViewRegistry::add(std::shared_ptr<EntityView> view) {
        if (view != nullptr) {
            views_.push_back(std::move(view));
        }
    }

    void ViewRegistry::clear() {
        views_.clear();
    }

    std::size_t ViewRegistry::size() const {
        return views_.size();
    }

    void ViewRegistry::update(const float dt) {
        // Reap first: a view whose model died last frame has nothing left
        // to draw, and keeping it would leak one object per explosion.
        std::erase_if(views_, [](const std::shared_ptr<EntityView>& v) { return v->orphaned(); });

        for (const auto& view : views_) {
            view->update(dt);
        }
    }

    void ViewRegistry::append_render_items(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) const {
        for (const auto& view : views_) {
            view->append_render_items(frame, camera);
        }
    }
} // namespace bomberman::view
