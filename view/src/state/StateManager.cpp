/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-11-26
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/state/StateManager.h"

#include <utility>

namespace bomberman::view {

    void State::on_enter(StateManager&) {}
    void State::on_pause() {}
    void State::on_text(StateManager&, char32_t) {}
    void State::on_resume() {}
    bool State::draws_below() const { return false; }

    StateManager::StateManager(Game &game) : game_(game) {
    }

    Game & StateManager::game() const { return game_; }

    void StateManager::push(std::unique_ptr<State> state) {
        if (state != nullptr) {
            pending_.push_back({Action::Push, std::move(state), 0});
        }
    }

    void StateManager::pop() {
        pending_.push_back({Action::Pop, nullptr, 0});
    }

    void StateManager::pop_to_depth(const std::size_t depth) {
        pending_.push_back({Action::PopToDepth, nullptr, depth});
    }

    void StateManager::quit() {
        pending_.push_back({Action::Quit, nullptr, 0});
    }

    void StateManager::update(const float dt) {
        if (!stack_.empty()) {
            stack_.back()->update(*this, dt);
        }
        apply_pending();
    }

    void StateManager::on_key(const sif::event::input::Key key) {
        if (!stack_.empty()) {
            stack_.back()->on_key(*this, key);
        }
        apply_pending();
    }

    void StateManager::flush() {
        apply_pending();
    }

    void StateManager::on_text(const char32_t character) {
        if (!stack_.empty()) {
            stack_.back()->on_text(*this, character);
        }
        apply_pending();
    }

    void StateManager::apply_pending() {
        // Moved out first: a state's on_enter may queue further
        // transitions, and iterating a vector that is being appended to
        // is how this kind of code usually breaks.
        std::vector<Pending> queue;
        queue.swap(pending_);

        for (Pending& item : queue) {
            switch (item.action) {
                case Action::Push:
                    if (!stack_.empty()) {
                        stack_.back()->on_pause();
                    }
                    stack_.push_back(std::move(item.state));
                    stack_.back()->on_enter(*this);
                    break;

                case Action::Pop:
                    if (stack_.empty()) {
                        break; // popping an empty stack is a no-op, not UB
                    }
                    stack_.pop_back();
                    if (!stack_.empty()) {
                        stack_.back()->on_resume();
                    }
                    break;

                case Action::PopToDepth:
                    while (stack_.size() > item.depth) {
                        stack_.pop_back();
                    }
                    if (!stack_.empty()) {
                        stack_.back()->on_resume();
                    }
                    break;

                case Action::Quit:
                    stack_.clear();
                    break;
            }
        }
    }

    void StateManager::append_render_items(sif::rnd::RenderFrame &frame, const DrawContext &ctx) const {
        if (stack_.empty()) {
            return;
        }

        // Find the deepest state that has to be drawn, then draw upwards
        // so overlays land on top.
        std::size_t first = stack_.size() - 1;
        while (first > 0 && stack_[first]->draws_below()) {
            --first;
        }

        for (std::size_t i = first; i < stack_.size(); ++i) {
            stack_[i]->append_render_items(frame, ctx);
        }
    }

    bool StateManager::running() const { return !stack_.empty(); }
    std::size_t StateManager::depth() const { return stack_.size(); }
}
