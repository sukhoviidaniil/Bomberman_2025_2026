/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-15
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_VIEW_GAME_H
#define BOMBERMAN_VIEW_GAME_H

#include <memory>
#include <string>

#include "bomberman/logic/ScoreBoard.h"
#include "bomberman/view/state/StateManager.h"

#include "sif/asset/AssetHandle.h"
#include "sif/asset/internal/Font.h"
#include "sif/audio/AudioPlayer.h"
#include "sif/event/Event_Bus.h"
#include "sif/event/Event_Collector.h"
#include "sif/event/Observer.h"
#include "sif/render/Camera.h"
#include "sif/render/Renderer.h"

namespace bomberman::view {

    /**
     * @brief Owns the window, the frame loop and everything global.
     *
     * Exactly the responsibilities the assignment gives this class:
     * "creating the SFML window, running the main game loop and
     * setting-up the StateManager". It holds no game rules - it does not
     * know what a bomb is - and it translates key presses into requests
     * that the current state interprets.
     */
    class Game final : public sif::event::Observer {
    public:
        /**
         * @param data_dir Directory holding the asset registry and data.
         * @throws std::runtime_error if the backend or the assets cannot be created.
         */
        explicit Game(std::string data_dir);

        ~Game() override;

        /// @brief Runs until the state stack empties or the window closes.
        void run();

        // ===== Services the states use =====

        [[nodiscard]] sif::audio::AudioPlayer& audio() const;
        [[nodiscard]] const sif::rnd::Camera& camera() const;
        [[nodiscard]] sif::asset::AssetHandle<sif::asset::Font> ui_font() const;

        [[nodiscard]] logic::ScoreBoard& score_board();
        [[nodiscard]] const std::string& score_board_path() const;

    private:
        void bootstrap_assets();
        void handle_event(const sif::event::EventConcept& ev);

        std::string data_dir_;
        std::string score_board_path_;

        std::shared_ptr<sif::event::Event_Bus> bus_;
        std::shared_ptr<sif::rnd::Renderer> renderer_;
        std::unique_ptr<sif::event::Event_Collector> collector_;
        std::shared_ptr<sif::audio::AudioPlayer> audio_;

        sif::rnd::Camera camera_;
        sif::asset::AssetHandle<sif::asset::Font> ui_font_;

        logic::ScoreBoard score_board_;
        StateManager states_;

        bool window_open_ = true;
    };
}

#endif //BOMBERMAN_VIEW_GAME_H
