/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-14
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/UiScene.h"

#include <stdexcept>
#include <utility>

#include "sif/layout_engine/Parser.h"
#include "sif/layout_engine/Tokenizer.h"
#include "sif/layout_engine/UIFactory.h"
#include "sif/render/FrameContext.h"

namespace bomberman::view {

    UiScene::UiScene(const std::string &scene_file) {
        const std::vector<sif::ui::Token> tokens = sif::ui::Tokenizer::tokenize(scene_file);
        std::unique_ptr<sif::ui::Node> node = sif::ui::Parser::parse(tokens);
        if (node == nullptr) {
            throw std::runtime_error("scene '" + scene_file + "' contains no root element");
        }
        root_ = sif::ui::UIFactory::instance().build(*node);
    }

    bool UiScene::loaded() const {
        return root_ != nullptr;
    }

    void UiScene::update(const float dt) {
        if (root_ != nullptr) {
            root_->update(dt);
        }
    }

    void UiScene::append_render_items(sif::rnd::RenderFrame &frame, const sif::rnd::Camera &camera) const {
        if (root_ == nullptr) {
            return;
        }

        // Laid out every frame against the *current* screen size, which is
        // what makes a resize re-flow the screen instead of stretching it.
        const sif::math::Vector2 screen = camera.screen_size();
        root_->measure(screen);
        root_->layout({0.f, 0.f, screen.x, screen.y});

        const sif::rnd::FrameContext ctx(true);
        root_->append_render_items(frame, ctx);
    }

    void UiScene::set_text(const std::string &element_name, const std::string &value) {
        if (root_ == nullptr) {
            return;
        }
        if (auto* label = root_->find_by_name<sif::ui::Text>(element_name)) {
            label->text = value;
        }
    }

    void UiScene::set_color(const std::string &element_name, const sif::intrnl::Color color) {
        if (root_ == nullptr) {
            return;
        }
        if (auto* label = root_->find_by_name<sif::ui::Text>(element_name)) {
            label->color = color;
        }
    }

    void UiScene::set_visible(const std::string &element_name, const bool visible) {
        if (root_ == nullptr) {
            return;
        }
        if (auto* element = root_->find_by_name<sif::ui::UIElement>(element_name)) {
            element->visible = visible;
        }
    }

    sif::ui::UIElement * UiScene::root() const {
        return root_.get();
    }

    // ===================== MenuNav =====================

    MenuNav::MenuNav(std::vector<std::string> item_names) : items_(std::move(item_names)) {
    }

    void MenuNav::move(const int delta) {
        if (items_.empty()) {
            return;
        }
        // Wraps, because a menu that stops at its ends makes the player
        // work out which end they are at.
        const int count = static_cast<int>(items_.size());
        int next = (static_cast<int>(index_) + delta) % count;
        if (next < 0) {
            next += count;
        }
        index_ = static_cast<std::size_t>(next);
    }

    const std::string & MenuNav::current() const {
        static const std::string none;
        return items_.empty() ? none : items_[index_];
    }

    std::size_t MenuNav::index() const { return index_; }
    bool MenuNav::empty() const { return items_.empty(); }

    void MenuNav::apply(UiScene &scene, const sif::intrnl::Color selected,
                        const sif::intrnl::Color normal) const {
        for (std::size_t i = 0; i < items_.size(); ++i) {
            scene.set_color(items_[i], i == index_ ? selected : normal);
        }
    }
}
