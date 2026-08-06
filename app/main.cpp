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

#include "bomberman/view/Game.h"

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

    try {
        bomberman::view::Game game(data_dir);
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
