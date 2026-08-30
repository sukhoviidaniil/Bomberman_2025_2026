/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-23
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/view/state/States.h"

#include <memory>
#include <utility>

#include "bomberman/view/ArenaView.h"
#include "bomberman/view/AssetNames.h"
#include "bomberman/view/Game.h"

#include "sif/render/elements/Rectangle.h"

namespace bomberman::view {
namespace {
const sif::intrnl::Color selected_color{250, 220, 120};
const sif::intrnl::Color normal_color{150, 155, 175};
const sif::intrnl::Color overlay_color{10, 12, 18, 200};

/**
 * @brief Dims whatever is underneath, for the overlay screens.
 *
 * Drawn here rather than declared in the scene file because sif's
 * layout engine stacks children in a line - it has no container
 * that puts one element *on top of* another.
 *
 * TODO(daniil): a <Stack> or absolute-positioned container in sif
 *  would let the backdrop live in the *.ui.xml with everything
 *  else, and would also be what a real HUD needs.
 */
void dim_background(sif::rnd::RenderFrame& frame, const sif::rnd::Camera& camera) {
    auto item = std::make_unique<sif::rnd::Rectangle>();
    item->rect = {0.f, 0.f, camera.screen_size().x, camera.screen_size().y};
    item->color = overlay_color;
    frame.temp_items.push_back(std::move(item));
}
} // namespace

// ===================== MenuState =====================

void MenuState::on_enter(StateManager& manager) {
    scene_ = UiScene(manager.game().scene_path("menu.ui.xml"));
    nav_ = MenuNav({"item_play", "item_settings", "item_quit"});
    needs_refresh_ = true;
    refresh(manager);
}

void MenuState::on_resume() {
    // Coming back from Settings or from a round: the name and the
    // scoreboard may both have changed underneath.
    needs_refresh_ = true;
}

void MenuState::refresh(StateManager& manager) {
    const auto& entries = manager.game().score_board().entries();

    for (std::size_t i = 0; i < logic::ScoreBoard::capacity; ++i) {
        const std::string slot = "score_" + std::to_string(i + 1);
        if (i < entries.size()) {
            scene_.set_text(slot, std::to_string(i + 1) + ".  " + entries[i].name + "   " +
                                      std::to_string(entries[i].points));
            scene_.set_color(slot, sif::intrnl::Color(220, 220, 235));
        } else {
            scene_.set_text(slot, "-");
            scene_.set_color(slot, sif::intrnl::Color(90, 95, 110));
        }
    }

    nav_.apply(scene_, selected_color, normal_color);
    needs_refresh_ = false;
}

void MenuState::update(StateManager& manager, const float dt) {
    if (needs_refresh_) {
        refresh(manager);
    }
    scene_.update(dt);
}

void MenuState::on_key(StateManager& manager, const sif::event::input::Key key) {
    using Key = sif::event::input::Key;

    switch (key) {
    case Key::Up:
    case Key::W:
        nav_.move(-1);
        needs_refresh_ = true;
        return;
    case Key::Down:
    case Key::S:
        nav_.move(1);
        needs_refresh_ = true;
        return;
    default:
        break;
    }

    if (key != Key::Enter && key != Key::Space) {
        if (key == Key::Escape) {
            manager.game().click();
            manager.quit();
        }
        return;
    }

    manager.game().click();

    const std::string& item = nav_.current();
    if (item == "item_play") {
        manager.push(std::make_unique<LevelState>());
    } else if (item == "item_settings") {
        manager.push(std::make_unique<SettingsState>());
    } else if (item == "item_quit") {
        manager.quit();
    }
}

void MenuState::append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const {
    scene_.append_render_items(frame, ctx.camera);
}

// ===================== SettingsState =====================

void SettingsState::on_enter(StateManager& manager) {
    scene_ = UiScene(manager.game().scene_path("settings.ui.xml"));
    refresh(manager);
}

void SettingsState::refresh(StateManager& manager) {
    const std::string& name = manager.game().profile().name();

    // A trailing caret, so an empty field still shows where typing goes.
    scene_.set_text("name_field", name + "_");
}

void SettingsState::update(StateManager& /*manager*/, const float dt) { scene_.update(dt); }

void SettingsState::on_key(StateManager& manager, const sif::event::input::Key key) {
    using Key = sif::event::input::Key;

    switch (key) {
    case Key::Backspace:
        manager.game().profile().backspace();
        refresh(manager);
        break;

    case Key::Enter:
    case Key::Escape:
        // Written on the way out rather than on every keystroke:
        // a name is edited character by character, and saving each
        // one would rewrite the file a dozen times per entry.
        manager.game().save_profile();
        manager.game().click();
        manager.pop();
        break;

    default:
        break;
    }
}

void SettingsState::on_text(StateManager& manager, const char32_t character) {
    manager.game().profile().append(character);
    refresh(manager);
}

void SettingsState::append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const {
    scene_.append_render_items(frame, ctx.camera);
}

// ===================== LevelState =====================

void LevelState::on_enter(StateManager& manager) {
    const logic::GameConfig& config = manager.game().config();

    assets_ = &manager.game().assets();
    hud_ = UiScene(manager.game().scene_path("hud.ui.xml"));

    world_bus_ = std::make_shared<sif::event::Event_Bus>();

    // Order matters: both observers subscribe before the World can emit
    // anything, so no event of the first frame is missed.
    score_ = std::make_unique<logic::Score>(world_bus_, config.score);
    audio_ = std::make_unique<AudioDirector>(world_bus_, manager.game().audio(), manager.game().assets(), config.audio);

    factory_ = std::make_shared<SFMLEntityFactory>(views_, manager.game().assets());
    world_ = std::make_unique<logic::World>(world_bus_, factory_, config.map, config.round, config.power_ups);
    world_->start_round();
}

void LevelState::update(StateManager& manager, const float dt) {
    if (world_ == nullptr) {
        return;
    }

    world_->update(dt);
    views_.update(dt);
    hud_.update(dt);

    if (score_ != nullptr) {
        hud_.set_text("score", "SCORE " + std::to_string(score_->points()));
    }

    if (const auto& player = world_->player(); player != nullptr) {
        const logic::PowerUpRules& rules = player->power_up_rules();
        const int speed_percent =
            rules.max_speed > 0.f ? static_cast<int>(player->speed() / rules.max_speed * 100.f) : 0;

        // The three stats the power-ups exist to change; without them a
        // pick-up has no visible effect at all.
        hud_.set_text("stats", "FIRE " + std::to_string(player->blast_radius()) + "    BOMBS " +
                                   std::to_string(player->bomb_budget()) + "    SPEED " +
                                   std::to_string(speed_percent) + "%");
    }

    if (world_->round_over() && !handed_over_) {
        handed_over_ = true;
        manager.push(std::make_unique<SaveScoreState>(world_->player_won(), score_->points()));
    }
}

void LevelState::on_key(StateManager& manager, const sif::event::input::Key key) {
    if (world_ == nullptr) {
        return;
    }

    using Key = sif::event::input::Key;
    switch (key) {
    case Key::Up:
    case Key::W:
        world_->set_player_direction(logic::Direction::Up);
        break;
    case Key::Down:
    case Key::S:
        world_->set_player_direction(logic::Direction::Down);
        break;
    case Key::Left:
    case Key::A:
        world_->set_player_direction(logic::Direction::Left);
        break;
    case Key::Right:
    case Key::D:
        world_->set_player_direction(logic::Direction::Right);
        break;
    case Key::Space:
        world_->player_place_bomb();
        break;
    case Key::Escape:
        manager.push(std::make_unique<PausedState>());
        break;
    default:
        break;
    }
}

void LevelState::append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const {
    if (world_ == nullptr || assets_ == nullptr) {
        return;
    }

    ArenaView::append_render_items(world_->grid(), *assets_, frame, ctx.camera);
    views_.append_render_items(frame, ctx.camera);
    hud_.append_render_items(frame, ctx.camera);
}

const logic::Score* LevelState::score() const { return score_.get(); }

// ===================== PausedState =====================

void PausedState::on_enter(StateManager& manager) { scene_ = UiScene(manager.game().scene_path("pause.ui.xml")); }

void PausedState::update(StateManager& /*manager*/, const float dt) {
    // Only the overlay animates: the level below is not updated while
    // this state is on top, which is what "paused" means.
    scene_.update(dt);
}

void PausedState::on_key(StateManager& manager, const sif::event::input::Key key) {
    using Key = sif::event::input::Key;
    switch (key) {
    case Key::Escape:
    case Key::Enter:
        manager.game().click();
        manager.pop(); // back into the level, exactly where it stopped
        break;
    case Key::S:
        manager.game().click();
        manager.pop_to_depth(1); // give up: the menu is at depth 1
        break;
    default:
        break;
    }
}

void PausedState::append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const {
    dim_background(frame, ctx.camera);
    scene_.append_render_items(frame, ctx.camera);
}

bool PausedState::draws_below() const { return true; }

// ===================== SaveScoreState =====================

SaveScoreState::SaveScoreState(const bool player_won, const int points) : player_won_(player_won), points_(points) {}

void SaveScoreState::on_enter(StateManager& manager) {
    scene_ = UiScene(manager.game().scene_path("save_score.ui.xml"));
    nav_ = MenuNav({"item_save", "item_rename", "item_discard"});
    needs_refresh_ = true;
    refresh(manager);
}

void SaveScoreState::on_resume() {
    // Back from Settings: the name on the screen is stale.
    needs_refresh_ = true;
}

void SaveScoreState::refresh(StateManager& manager) {
    scene_.set_text("result", player_won_ ? "YOU WIN" : "GAME OVER");
    scene_.set_text("score", "score " + std::to_string(points_));
    scene_.set_text("name_line", "saving as: " + manager.game().profile().name());

    const bool qualifies = manager.game().score_board().qualifies(points_);
    scene_.set_text("board_note", qualifies ? "this would make the top five" : "this is not enough for the top five");
    scene_.set_color("board_note", qualifies ? sif::intrnl::Color(120, 220, 150) : sif::intrnl::Color(150, 155, 175));

    nav_.apply(scene_, selected_color, normal_color);
    needs_refresh_ = false;
}

void SaveScoreState::update(StateManager& manager, const float dt) {
    if (needs_refresh_) {
        refresh(manager);
    }
    scene_.update(dt);
}

void SaveScoreState::save(StateManager& manager) {
    if (saved_) {
        return; // pressing Enter twice must not add two entries
    }
    saved_ = true;

    logic::ScoreBoard& board = manager.game().score_board();
    board.submit({manager.game().profile().name(), points_});
    board.save(manager.game().score_board_path());
}

void SaveScoreState::on_key(StateManager& manager, const sif::event::input::Key key) {
    using Key = sif::event::input::Key;

    switch (key) {
    case Key::Up:
    case Key::W:
        nav_.move(-1);
        needs_refresh_ = true;
        return;
    case Key::Down:
    case Key::S:
        nav_.move(1);
        needs_refresh_ = true;
        return;
    default:
        break;
    }

    if (key == Key::Escape) {
        // Escape discards: the safe reading of "I did not choose".
        manager.game().click();
        manager.pop_to_depth(1);
        return;
    }

    if (key != Key::Enter && key != Key::Space) {
        return;
    }

    manager.game().click();

    const std::string& item = nav_.current();
    if (item == "item_save") {
        save(manager);
        manager.pop_to_depth(1);
    } else if (item == "item_rename") {
        // Pushed, not replaced: Settings pops itself and this screen
        // comes back with the new name already in it.
        manager.push(std::make_unique<SettingsState>());
    } else if (item == "item_discard") {
        manager.pop_to_depth(1);
    }
}

void SaveScoreState::append_render_items(sif::rnd::RenderFrame& frame, const DrawContext& ctx) const {
    dim_background(frame, ctx.camera);
    scene_.append_render_items(frame, ctx.camera);
}

bool SaveScoreState::draws_below() const { return true; }
} // namespace bomberman::view
