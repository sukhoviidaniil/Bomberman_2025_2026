/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-16
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/
#ifndef BOMBERMAN_LOGIC_WORLD_H
#define BOMBERMAN_LOGIC_WORLD_H

#include <memory>
#include <vector>

#include "bomberman/logic/Config.h"
#include "bomberman/logic/IEntityFactory.h"
#include "bomberman/logic/grid/TileGrid.h"

#include "sif/event/Event_Bus.h"

namespace bomberman::logic {

    /**
     * @brief Owns every entity and runs the rules that connect them.
     *
     * The "entity controller" of the MVC split: the World creates and
     * destroys entities, moves them, resolves what happens when they
     * touch, and announces the results on its bus. It holds no SFML type
     * and no view - entities arrive fully formed from an IEntityFactory,
     * so the same World runs identically under the SFML front-end and in
     * a test.
     */
    class World {
    public:
        /**
         * @param bus Where gameplay events are published (Score listens here).
         * @param factory Builds entities - with their views attached, when
         * the representation layer supplies the factory.
         * @param map How the arena is produced: explicit layout, seed, or
         * neither.
         * @param round Speeds, timings and how many bots to spawn.
         */
        World(std::shared_ptr<sif::event::Event_Bus> bus,
              std::shared_ptr<IEntityFactory> factory,
              MapConfig map = {},
              RoundConfig round = {});

        World(const World&) = delete;
        World& operator=(const World&) = delete;

        /// @brief Builds the arena and spawns the player plus the bots.
        void start_round();

        /**
         * @brief Advances the whole simulation by one frame.
         *
         * @param dt Seconds since the previous frame.
         */
        void update(float dt);

        // ===== Player input =====

        /// @brief Steers the player; applied at the next cell centre.
        void set_player_direction(Direction direction);

        /// @brief Places a bomb under the player, if their budget allows.
        void player_place_bomb();

        // ===== Queries used by the views and the HUD =====

        [[nodiscard]] const TileGrid& grid() const;
        [[nodiscard]] const std::shared_ptr<Character>& player() const;
        [[nodiscard]] const std::vector<std::shared_ptr<Character>>& characters() const;
        [[nodiscard]] const std::vector<std::shared_ptr<Bomb>>& bombs() const;
        [[nodiscard]] const std::vector<std::shared_ptr<Explosion>>& explosions() const;
        [[nodiscard]] const std::vector<std::shared_ptr<PowerUp>>& power_ups() const;

        /// @brief True once the round is decided; the state machine polls it.
        [[nodiscard]] bool round_over() const;
        [[nodiscard]] bool player_won() const;

        /// @brief True when a bomb currently occupies the cell.
        [[nodiscard]] bool has_bomb_at(const TilePos& cell) const;

    private:
        void spawn_characters();
        void place_bomb_for(const std::shared_ptr<Character>& character);

        /// @brief Turns detonated bombs into fire, recursively via chains.
        void resolve_detonations();

        /// @brief Expands one blast, stopping at walls and setting off bombs.
        void spread_blast(const Bomb& bomb);

        /// @brief Kills characters and burns power-ups standing in fire.
        void resolve_collisions();

        /// @brief Lets characters walk off a bomb exactly once.
        void update_bomb_passability();

        void remove_expired();
        void check_round_over();

        std::shared_ptr<sif::event::Event_Bus> bus_;
        std::shared_ptr<IEntityFactory> factory_;
        MapConfig map_;
        RoundConfig round_;

        TileGrid grid_;

        std::shared_ptr<Character> player_;
        std::vector<std::shared_ptr<Character>> characters_;
        std::vector<std::shared_ptr<Bomb>> bombs_;
        std::vector<std::shared_ptr<Explosion>> explosions_;
        std::vector<std::shared_ptr<PowerUp>> power_ups_;

        bool round_over_ = false;
        bool player_won_ = false;
    };
}

#endif //BOMBERMAN_LOGIC_WORLD_H
