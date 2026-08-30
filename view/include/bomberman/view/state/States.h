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
#include "bomberman/view/AudioDirector.h"
#include "bomberman/view/EntityView.h"
#include "bomberman/view/SFMLEntityFactory.h"
#include "bomberman/view/UiScene.h"
#include "bomberman/view/state/State.h"

namespace bomberman::view {

/**
 * @brief Start screen: the top-five table and the main menu.
 *
 * Stays at the bottom of the stack for the whole session, so returning
 * to it is a pop rather than a rebuild - and the scoreboard it shows is
 * the same object the level updates.
 */
class MenuState final : public State {
public:
    void on_enter(StateManager& manager) override;
    void on_resume() override;
    void update(StateManager& manager, float dt) override;
    void on_key(StateManager& manager, sif::event::input::Key key) override;
    void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;

private:
    void refresh(StateManager& manager);

    UiScene scene_;
    MenuNav nav_;
    bool needs_refresh_ = true;
};

/**
 * @brief Name entry, reachable from the menu and from the save screen.
 *
 * Pops itself when done, which is how one screen serves both callers:
 * whoever pushed it is underneath and comes back automatically. That is
 * the whole reason the state machine is a stack rather than a set of
 * transitions.
 */
class SettingsState final : public State {
public:
    void on_enter(StateManager& manager) override;
    void update(StateManager& manager, float dt) override;
    void on_key(StateManager& manager, sif::event::input::Key key) override;
    void on_text(StateManager& manager, char32_t character) override;
    void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;

private:
    void refresh(StateManager& manager);

    UiScene scene_;
};

/**
 * @brief The round itself: owns the World, its views and the Score.
 *
 * Everything with a lifetime of one round lives here and dies with the
 * state, so starting a new round cannot inherit anything from the
 * previous one.
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
    std::unique_ptr<AudioDirector> audio_;

    /// Borrowed from the Game, which outlives every state.
    const GameAssets* assets_ = nullptr;

    UiScene hud_;
    bool handed_over_ = false;
};

/// @brief Overlay drawn on top of the frozen level.
class PausedState final : public State {
public:
    void on_enter(StateManager& manager) override;
    void update(StateManager& manager, float dt) override;
    void on_key(StateManager& manager, sif::event::input::Key key) override;
    void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;
    [[nodiscard]] bool draws_below() const override;

private:
    UiScene scene_;
};

/**
 * @brief Result screen: keep this score, rename first, or throw it away.
 *
 * The score is *not* recorded on entry. A player who does not want a
 * bad round on the board should not have to delete it afterwards, and
 * one who mistyped their name should be able to fix it before it is
 * written rather than after.
 */
class SaveScoreState final : public State {
public:
    SaveScoreState(bool player_won, int points);

    void on_enter(StateManager& manager) override;
    void on_resume() override;
    void update(StateManager& manager, float dt) override;
    void on_key(StateManager& manager, sif::event::input::Key key) override;
    void append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const override;
    [[nodiscard]] bool draws_below() const override;

private:
    void refresh(StateManager& manager);
    void save(StateManager& manager);

    bool player_won_;
    int points_;
    bool saved_ = false;
    bool needs_refresh_ = true;

    UiScene scene_;
    MenuNav nav_;
};
} // namespace bomberman::view

#endif // BOMBERMAN_VIEW_STATES_H
