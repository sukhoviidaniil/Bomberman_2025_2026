/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-10-23
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include <exception>
#include <iostream>
#include <string>

#include "bomberman/logic/Config.h"
#include "bomberman/view/Game.h"

#include "sif/internal/Random.h"

/**
 * @brief Entry point.
 *
 * @param argv[1] Optional data directory; defaults to assets/ so the
 * binary can be run straight from a build directory.
 *
 * Everything below the top-level try/catch reports failure by throwing -
 * a missing registry, an unreadable font, an unsupported backend. Without
 * this handler those reach std::terminate and the player sees nothing at
 * all, which is what the Pac-Man build did.
 */
int main(const int argc, char* argv[]) {
    const std::string data_dir = argc > 1 ? argv[1] : "assets/";
    const std::string config_path = argc > 2 ? argv[2] : data_dir + "config.json";

    try {
        // Everything tunable lives in one file: the seeds, the arena
        // (generated or hand-written), the balance, the window and the
        // scoring. Loading it before anything else means a bad value is
        // reported before a window is opened.
        const bomberman::logic::GameConfig config = bomberman::logic::GameConfig::load(config_path);

        if (config.random_seed.has_value()) {
            // Pins the one shared generator, so an entire session -
            // arena, power-up drops, every later draw - replays exactly.
            sif::intrnl::Random::instance().seed(*config.random_seed);
            // std::endl, not '\n': these lines are diagnostics that must
            // survive a process that is killed before it exits normally.
            std::cout << "random seed: " << *config.random_seed << std::endl;
        }
        if (config.map.seed.has_value()) {
            std::cout << "map seed: " << *config.map.seed << std::endl;
        } else if (!config.map.layout.empty()) {
            std::cout << "map: explicit layout from " << config_path << std::endl;
        }

        bomberman::view::Game game(data_dir, config);
        game.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Fatal: unknown error\n";
        return 1;
    }
}
