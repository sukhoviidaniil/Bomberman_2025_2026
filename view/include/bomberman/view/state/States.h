/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-23
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_STATES_H
#define BOMBERMAN_VIEW_STATES_H

#include <memory>
#include <string>
#include <vector>

#include "bomberman/logic/Score.h"
#include "bomberman/logic/World.h"
#include "bomberman/view/EntityView.h"
#include "bomberman/view/SFMLEntityFactory.h"
#include "bomberman/view/state/State.h"

namespace bomberman::view {

    /**
     * @brief Start screen: the top-five table and a "Play" prompt.
     *
     * Stays at the bottom of the stack for the whole session, so
     * returning to it is a pop rather than a rebuild - and the scoreboard
     * it shows is the same object the level updates.
     */
    class MenuState final : public State {
    public:
        void update(StateManager& manager, float dt) override;
        void on_key(StateManager& manager, sif::event::input::Key key) override;
        void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;
        void on_enter(StateManager& manager) override;
        void on_resume() override;

    private:
        StateManager* manager_ = nullptr;
        float elapsed_ = 0.f;
    };

    /**
     * @brief The round itself: owns the World, its views and the Score.
     *
     * Everything with a lifetime of one round lives here and dies with
     * the state, so starting a new round cannot inherit anything from the
     * previous one. (Pac-Man kept its model in a factory that outlived
     * the level, so a new game began with the coins the *previous* game
     * had left uneaten.)
     */
    class LevelState final : public State {
    public:
        void on_enter(StateManager& manager) override;
        void update(StateManager& manager, float dt) override;
        void on_key(StateManager& manager, sif::event::input::Key key) override;
        void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;

        [[nodiscard]] const logic::Score* score() const;

    private:
        std::shared_ptr<sif::event::Event_Bus> world_bus_;
        ViewRegistry views_;
        std::shared_ptr<SFMLEntityFactory> factory_;
        std::unique_ptr<logic::World> world_;
        std::unique_ptr<logic::Score> score_;
        bool handed_over_ = false;
    };

    /// @brief Overlay drawn on top of the frozen level.
    class PausedState final : public State {
    public:
        void update(StateManager& manager, float dt) override;
        void on_key(StateManager& manager, sif::event::input::Key key) override;
        void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;
        [[nodiscard]] bool draws_below() const override;
    };

    /// @brief Result screen; submits the score to the board.
    class GameOverState final : public State {
    public:
        GameOverState(bool player_won, int points);

        void on_enter(StateManager& manager) override;
        void update(StateManager& manager, float dt) override;
        void on_key(StateManager& manager, sif::event::input::Key key) override;
        void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;
        [[nodiscard]] bool draws_below() const override;

    private:
        bool player_won_;
        int points_;
        bool recorded_ = false;
    };
}

#endif //BOMBERMAN_VIEW_STATES_H
