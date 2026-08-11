/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_CONFIG_H
#define BOMBERMAN_LOGIC_CONFIG_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bomberman/logic/Score.h"
#include "bomberman/logic/ai/BotBrain.h"

namespace bomberman::logic {

    /**
     * @brief How the arena for a round is produced.
     *
     * Three mutually exclusive sources, in priority order:
     *
     *  1. `layout` - an explicit character matrix. Used verbatim, which is
     *     what makes a hand-designed level (or a regression test for a
     *     specific situation) possible at all.
     *  2. `seed` - procedural generation from a fixed seed. The same seed
     *     always produces the same arena, so "the bug on map 12345" is a
     *     thing that can be reported and reproduced.
     *  3. neither - procedural generation from the global random stream,
     *     i.e. a different arena every run.
     */
    struct MapConfig {
        std::size_t rows = 11;
        std::size_t columns = 13;

        /// Probability that a free cell becomes a destructible block.
        float destructible_chance = 0.75f;

        /// Probability that destroying a block reveals a power-up.
        float power_up_chance = 0.25f;

        /// Seed for procedural generation; empty means "use the global stream".
        std::optional<std::uint32_t> seed;

        /**
         * @brief Explicit arena, one string per row.
         *
         * Legend (see TileGrid::from_layout):
         *   `#` indestructible   `+` destructible   `.` or space  free
         *   `1`-`4`              spawn cell (also free)
         */
        std::vector<std::string> layout;
    };

    /// @brief Everything about one round that is not the map.
    struct RoundConfig {
        float character_speed = 0.45f;  ///< World units per second
        float character_size = 0.85f;   ///< Fraction of a tile
        float bomb_fuse_seconds = 2.f;
        float explosion_seconds = 0.5f;
        std::size_t bot_count = 3;

        /**
         * @brief One personality per bot, cycled if there are fewer than bots.
         *
         * Empty means "all balanced". A personality is only the order in
         * which a bot weighs its goals, so this is a genuine gameplay knob
         * rather than a cosmetic one - three aggressive bots play very
         * differently from three collectors.
         */
        std::vector<ai::BotPersonality> bot_personalities;
    };

    /// @brief Window and frame-rate settings, consumed by the view layer.
    struct WindowConfig {
        std::string title = "Bomberman";
        unsigned int width = 960;
        unsigned int height = 720;
        int fps = 60;
    };

    /// @brief Audio settings, consumed by the view layer.
    struct AudioConfig {
        bool enabled = true;
        float master_volume = 0.7f;
        float sfx_volume = 1.f;
    };

    /**
     * @brief The whole game, described as data.
     *
     * Everything a player or a marker might want to change without a
     * rebuild lives here: the seeds, the arena, the balance, the window and
     * the scoring. Nothing in this struct depends on SFML, so a headless
     * test can load the very same file the game does.
     *
     * @par Reproducibility
     * `random_seed` seeds the one shared generator (sif::intrnl::Random) at
     * start-up, which makes an entire session deterministic - bot decisions,
     * power-up drops and, unless overridden, the arena. `map.seed` is
     * separate on purpose: it pins the arena while leaving everything else
     * free, which is what you want when reproducing "this map plays badly"
     * rather than "this exact run crashed".
     */
    struct GameConfig {
        std::optional<std::uint32_t> random_seed;

        MapConfig map;
        RoundConfig round;
        ScoreRules score;
        WindowConfig window;
        AudioConfig audio;

        /**
         * @brief Reads a configuration file.
         *
         * Every field is optional; anything absent keeps the default above,
         * so a two-line file that only sets a seed is valid. Anything
         * *present but wrong* (a layout with rows of different lengths, a
         * negative speed) is an error rather than something quietly ignored.
         *
         * @throws std::runtime_error if the file is missing, is not valid
         * JSON, or contains a value that cannot be used.
         */
        [[nodiscard]] static GameConfig load(const std::string& filepath);

        /**
         * @brief Checks the invariants the loader promises.
         *
         * @throws std::runtime_error describing the first problem found.
         */
        void validate() const;
    };
}

#endif //BOMBERMAN_LOGIC_CONFIG_H
