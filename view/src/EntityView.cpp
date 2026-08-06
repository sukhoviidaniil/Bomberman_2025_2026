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

#include "sif/render/elements/Rectangle.h"

namespace bomberman::view {

    EntityView::~EntityView() = default;

    EntityView::EntityView(const std::shared_ptr<logic::Entity> &model) : model_(model) {
    }

    bool EntityView::orphaned() const {
        return model_.expired();
    }

    void EntityView::append_box(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera,
                                const sif::intrnl::Color color, const float scale) const {
        const auto model = model_.lock();
        if (model == nullptr) {
            return;
        }

        const logic::AABB box = model->box();
        const logic::AABB scaled{box.center, {box.half.x * scale, box.half.y * scale}};

        auto item = std::make_unique<sif::rnd::Rectangle>();
        item->rect = camera.world_to_screen(scaled.to_rect());
        item->color = color;

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

    void ViewRegistry::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) {
        // Reap first: a view whose model died last frame has nothing left
        // to draw, and keeping it would leak one object per explosion.
        std::erase_if(views_, [](const std::shared_ptr<EntityView>& v) { return v->orphaned(); });

        for (const auto& view : views_) {
            view->append_render_items(frame, camera);
        }
    }
}
