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

#include <utility>

#include "bomberman/view/ArenaView.h"
#include "bomberman/view/AssetNames.h"
#include "bomberman/view/Game.h"

#include "sif/render/elements/Rectangle.h"
#include "sif/render/elements/Text.h"

namespace bomberman::view {
    namespace {
        const sif::intrnl::Color text_color{235, 235, 245};
        const sif::intrnl::Color dim_color{170, 175, 190};
        const sif::intrnl::Color overlay_color{10, 12, 18, 190};

        /// Draws one line of text at a pixel position.
        void text(sif::rnd::RenderFrame& frame, const DrawContext& ctx,
                  const std::string& value, const float y, const int size,
                  const sif::intrnl::Color color) {

            if (!ctx.font.ready()) {
                return; // the font is still loading; skip the label, not the frame
            }

            auto item = std::make_unique<sif::rnd::Text>();
            item->text = value;
            item->size = size;
            item->color = color;
            item->font = ctx.font;
            // Centred horizontally by eye; a real HUD should be built from
            // sif::ui elements instead.
            //
            // TODO(daniil): replace these hand-placed labels with a
            //  *.ui.xml scene loaded through sif's layout engine, the way
            //  the sif demos do. The engine already measures text with the
            //  real font metrics, so centring becomes exact instead of
            //  approximate.
            const float width = ctx.camera.screen_size().x;
            item->rect = {width * 0.5f - static_cast<float>(value.size()) * static_cast<float>(size) * 0.27f,
                          y, 0.f, 0.f};
            frame.temp_items.push_back(std::move(item));
        }

        void full_screen_overlay(sif::rnd::RenderFrame& frame, const DrawContext& ctx) {
            auto item = std::make_unique<sif::rnd::Rectangle>();
            item->rect = {0.f, 0.f, ctx.camera.screen_size().x, ctx.camera.screen_size().y};
            item->color = overlay_color;
            frame.temp_items.push_back(std::move(item));
        }
    }

    // ===================== MenuState =====================

    void MenuState::on_enter(StateManager &manager) {
        manager_ = &manager;
    }

    void MenuState::on_resume() {
        elapsed_ = 0.f;
    }

    void MenuState::update(StateManager &manager, const float dt) {
        manager_ = &manager;
        elapsed_ += dt;
    }

    void MenuState::on_key(StateManager &manager, const sif::event::input::Key key) {
        switch (key) {
            case sif::event::input::Key::Enter:
            case sif::event::input::Key::Space:
                click(manager);
                manager.push(std::make_unique<LevelState>());
                break;
            case sif::event::input::Key::Escape:
                click(manager);
                manager.quit();
                break;
            default:
                break;
        }
    }

    void MenuState::click(StateManager &manager) {
        // A menu that answers a key press with silence feels broken even
        // when it is not; sfx_menu was loaded from the first commit and
        // never played.
        manager.game().audio().play(
            manager.game().assets().sound(assets::sfx_menu), 0.8f);
    }

    void MenuState::append_render_items(sif::rnd::RenderFrame &frame, const DrawContext &ctx) const {
        text(frame, ctx, "BOMBERMAN", 90.f, 56, text_color);
        text(frame, ctx, "TOP 5", 200.f, 28, dim_color);

        float y = 250.f;
        if (manager_ != nullptr) {
            const auto& entries = manager_->game().score_board().entries();
            if (entries.empty()) {
                text(frame, ctx, "no scores yet", y, 22, dim_color);
            }
            for (std::size_t i = 0; i < entries.size(); ++i) {
                text(frame, ctx,
                     std::to_string(i + 1) + ".  " + entries[i].name + "   " + std::to_string(entries[i].points),
                     y, 24, text_color);
                y += 40.f;
            }
        }

        text(frame, ctx, "ENTER - play      ESC - quit", 600.f, 24, dim_color);
    }

    // ===================== LevelState =====================

    void LevelState::on_enter(StateManager &manager) {
        const logic::GameConfig& config = manager.game().config();

        assets_ = &manager.game().assets();
        world_bus_ = std::make_shared<sif::event::Event_Bus>();

        // Order matters: both observers subscribe before the World can
        // emit anything, so no event of the first frame is missed.
        score_ = std::make_unique<logic::Score>(world_bus_, config.score);
        audio_ = std::make_unique<AudioDirector>(
            world_bus_, manager.game().audio(), manager.game().assets(), config.audio);

        factory_ = std::make_shared<SFMLEntityFactory>(views_, manager.game().assets());
        world_ = std::make_unique<logic::World>(
            world_bus_, factory_, config.map, config.round, config.power_ups);
        world_->start_round();
    }

    void LevelState::update(StateManager &manager, const float dt) {
        if (world_ == nullptr) {
            return;
        }

        world_->update(dt);
        views_.update(dt);

        if (world_->round_over() && !handed_over_) {
            handed_over_ = true;
            manager.push(std::make_unique<GameOverState>(world_->player_won(), score_->points()));
        }
    }

    void LevelState::on_key(StateManager &manager, const sif::event::input::Key key) {
        if (world_ == nullptr) {
            return;
        }

        using Key = sif::event::input::Key;
        switch (key) {
            case Key::Up:    case Key::W: world_->set_player_direction(logic::Direction::Up); break;
            case Key::Down:  case Key::S: world_->set_player_direction(logic::Direction::Down); break;
            case Key::Left:  case Key::A: world_->set_player_direction(logic::Direction::Left); break;
            case Key::Right: case Key::D: world_->set_player_direction(logic::Direction::Right); break;
            case Key::Space: world_->player_place_bomb(); break;
            case Key::Escape: manager.push(std::make_unique<PausedState>()); break;
            default: break;
        }
    }

    void LevelState::append_render_items(sif::rnd::RenderFrame &frame, const DrawContext &ctx) const {
        if (world_ == nullptr || assets_ == nullptr) {
            return;
        }

        ArenaView::append_render_items(world_->grid(), *assets_, frame, ctx.camera);
        views_.append_render_items(frame, ctx.camera);

        if (score_ != nullptr) {
            text(frame, ctx, "SCORE  " + std::to_string(score_->points()), 16.f, 24, text_color);
        }

        // The three stats the power-ups exist to change. Without them a
        // player has no way to tell that a pick-up did anything, which
        // makes the whole mechanic invisible.
        if (const auto& player = world_->player(); player != nullptr) {
            const float speed_percent = player->power_up_rules().max_speed > 0.f
                ? player->speed() / player->power_up_rules().max_speed * 100.f
                : 0.f;

            text(frame, ctx,
                 "FIRE " + std::to_string(player->blast_radius())
                 + "    BOMBS " + std::to_string(player->bomb_budget())
                 + "    SPEED " + std::to_string(static_cast<int>(speed_percent)) + "%",
                 46.f, 20, dim_color);
        }
    }

    const logic::Score * LevelState::score() const { return score_.get(); }

    // ===================== PausedState =====================

    void PausedState::update(StateManager & /*manager*/, float /*dt*/) {
        // Deliberately empty: the level below is not updated while this
        // state is on top, which is what "paused" means.
    }

    void PausedState::on_key(StateManager &manager, const sif::event::input::Key key) {
        using Key = sif::event::input::Key;
        switch (key) {
            case Key::Escape:
            case Key::Enter:
                manager.pop(); // back into the level, exactly where it stopped
                break;
            case Key::S:
                // Give up: drop the level too, leaving the menu at depth 1.
                manager.pop_to_depth(1);
                break;
            default:
                break;
        }
    }

    void PausedState::append_render_items(sif::rnd::RenderFrame &frame, const DrawContext &ctx) const {
        full_screen_overlay(frame, ctx);
        text(frame, ctx, "PAUSED", 280.f, 48, text_color);
        text(frame, ctx, "ESC - resume      S - back to menu", 380.f, 24, dim_color);
    }

    bool PausedState::draws_below() const { return true; }

    // ===================== GameOverState =====================

    GameOverState::GameOverState(const bool player_won, const int points)
        : player_won_(player_won), points_(points) {
    }

    void GameOverState::on_enter(StateManager &manager) {
        if (recorded_) {
            return;
        }
        recorded_ = true;

        logic::ScoreBoard& board = manager.game().score_board();
        if (board.submit({"player", points_})) {
            // TODO(daniil): ask for a name instead of hard-coding
            //  "player" - the board already stores one per entry.
            board.save(manager.game().score_board_path());
        }
    }

    void GameOverState::update(StateManager & /*manager*/, float /*dt*/) {
    }

    void GameOverState::on_key(StateManager &manager, const sif::event::input::Key key) {
        if (key == sif::event::input::Key::Enter || key == sif::event::input::Key::Escape) {
            manager.pop_to_depth(1); // back to the menu that was never destroyed
        }
    }

    void GameOverState::append_render_items(sif::rnd::RenderFrame &frame, const DrawContext &ctx) const {
        full_screen_overlay(frame, ctx);
        text(frame, ctx, player_won_ ? "YOU WIN" : "GAME OVER", 260.f, 52, text_color);
        text(frame, ctx, "score  " + std::to_string(points_), 350.f, 30, text_color);
        text(frame, ctx, "ENTER - back to menu", 430.f, 24, dim_color);
    }

    bool GameOverState::draws_below() const { return true; }
}
