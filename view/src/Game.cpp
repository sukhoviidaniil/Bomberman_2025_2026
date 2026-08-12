/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-15
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/Game.h"

#include <stdexcept>
#include <utility>

#include "bomberman/view/AssetNames.h"
#include "bomberman/view/state/States.h"

#include "Graphics_Factory.h"

#include "sif/asset/AssetRegistry.h"
#include "sif/asset/internal/AssetImporter.h"
#include "sif/event/events/input_Keyboard.h"
#include "sif/event/events/window.hpp"
#include "sif/internal/Delta_Timer.h"
#include "sif/render/FrameContext.h"

namespace bomberman::view {
    namespace {
        /// The arena is square, so letterboxing keeps the tiles square.
        constexpr sif::rnd::AspectPolicy arena_aspect = sif::rnd::AspectPolicy::Fit;
    }

    Game::Game(std::string data_dir, logic::GameConfig config)
        : data_dir_(std::move(data_dir))
        , bus_(std::make_shared<sif::event::Event_Bus>())
        , config_(std::move(config))
        , camera_({static_cast<float>(config_.window.width),
                   static_cast<float>(config_.window.height)}, arena_aspect)
        , states_(*this) {

        if (!data_dir_.empty() && data_dir_.back() != '/') {
            data_dir_.push_back('/');
        }
        score_board_path_ = data_dir_ + "scoreboard.json";
        profile_path_ = data_dir_ + "player.json";

        const sif::ast::RB_Config render_config{
            .type = sif::ast::RB_Type::SFML,
            .window_name = config_.window.title,
            .window_width = config_.window.width,
            .window_height = config_.window.height,
            .fps = config_.window.fps
        };
        const sif::ast::EC_Config collector_config{.type = sif::ast::RB_Type::SFML};

        app::Graphics_Factory& factory = app::Graphics_Factory::instance();

        renderer_ = factory.make_Renderer(render_config);
        renderer_->track_global(bus_);
        collector_ = factory.make_Event_Collector(collector_config);
        audio_ = factory.make_AudioPlayer(render_config);

        factory.register_asset_loaders(render_config, sif::asset::AssetRegistry::instance());
        bootstrap_assets();

        score_board_.load(score_board_path_);
        profile_.load(profile_path_);

        track(bus_->subscribe<sif::event::window::Window_Closed>(
            [this](const sif::event::window::Window_Closed&) {
                window_open_ = false;
            }));

        track(bus_->subscribe<sif::event::window::Window_Resized>(
            [this](const sif::event::window::Window_Resized& e) {
                // Re-project instead of letting SFML stretch the frame:
                // the arena stays square whatever the window shape.
                camera_.set_screen_size({static_cast<float>(e.width), static_cast<float>(e.height)});
            }));

        states_.push(std::make_unique<MenuState>());
        states_.flush(); // otherwise the stack is still empty when run() checks it
    }

    Game::~Game() = default;

    void Game::bootstrap_assets() {
        sif::asset::AssetImporter& importer = sif::asset::AssetImporter::instance();
        sif::asset::AssetRegistry& registry = sif::asset::AssetRegistry::instance();

        importer.load_from_file(data_dir_ + "bin/registry.rgst.json");
        importer.load_in_registry();

        registry.set_asset_dir(data_dir_);

        // Requesting every asset here starts all the background loads at
        // once, so the textures are in flight while the menu is drawn.
        assets_ = std::make_unique<GameAssets>();
    }

    void Game::handle_event(const sif::event::EventConcept &ev) {
        if (has(ev.mask(), sif::event::EventMask::Window)) {
            bus_->emit(ev);
            return;
        }

        if (ev.type() != std::type_index(typeid(sif::event::input::KeyPressed))
            && ev.type() != std::type_index(typeid(sif::event::input::TextEntered))) {
            return;
        }

        if (ev.type() == std::type_index(typeid(sif::event::input::TextEntered))) {
            const auto& typed = *static_cast<const sif::event::input::TextEntered*>(ev.data());
            if (typed.printable()) {
                states_.on_text(typed.unicode);
            }
            return;
        }

        const auto& pressed = *static_cast<const sif::event::input::KeyPressed*>(ev.data());
        states_.on_key(pressed.key);
    }

    void Game::run() {
        // Take one tick before the loop so the first frame is not charged
        // with the whole start-up time.
        sif::intrnl::Delta_Timer::instance().tick();

        while (window_open_ && states_.running()) {
            const float dt = sif::intrnl::Delta_Timer::instance().tick();

            states_.update(dt);

            sif::rnd::RenderFrame frame;
            const DrawContext draw_ctx{camera_, ui_font()};
            states_.append_render_items(frame, draw_ctx);
            renderer_->render(frame);

            collector_->collect();
            while (!collector_->event_store_.empty()) {
                const std::unique_ptr<sif::event::EventConcept> ev = collector_->event_store_.pop_concept();
                handle_event(*ev);
            }
        }

        audio_->stop_all();

        // Asset loads run on detached threads; do not tear the registry
        // down underneath one of them.
        sif::asset::AssetRegistry::instance().wait_for_idle();
    }

    sif::audio::AudioPlayer & Game::audio() const { return *audio_; }
    const sif::rnd::Camera & Game::camera() const { return camera_; }
    sif::asset::AssetHandle<sif::asset::Font> Game::ui_font() const {
        return assets_->font(assets::ui_font);
    }

    const logic::GameConfig & Game::config() const { return config_; }

    const GameAssets & Game::assets() const { return *assets_; }
    logic::ScoreBoard & Game::score_board() { return score_board_; }
    const std::string & Game::score_board_path() const { return score_board_path_; }

    logic::PlayerProfile & Game::profile() { return profile_; }

    void Game::save_profile() const { profile_.save(profile_path_); }

    std::string Game::scene_path(const std::string &scene_file) const {
        return data_dir_ + "bin/scenes/" + scene_file;
    }

    void Game::click() const {
        audio_->play(assets_->sound(assets::sfx_menu), 0.8f);
    }
}
