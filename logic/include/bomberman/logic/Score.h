/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-15
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_SCORE_H
#define BOMBERMAN_LOGIC_SCORE_H

#include <cstdint>
#include <memory>

#include "bomberman/logic/events/GameEvents.h"

#include "sif/event/Event_Bus.h"
#include "sif/event/Observer.h"

namespace bomberman::logic {

/**
 * @brief Point values, in one place so balancing is a data change.
 */
struct ScoreRules {
    int per_second_alive = 5;
    int per_block_destroyed = 10;
    int per_power_up = 25;
    int per_enemy_killed = 200;
    int win_bonus = 1000;
    int loss_penalty = 500;
};

/**
 * @brief Keeps the running score by *listening*, not by being told.
 *
 * Score is an Observer of the world bus, exactly as the assignment
 * describes ("These same generic events are at the same time used by
 * the Score class (also an Observer) for updating the current score
 * when a block is destroyed, power-up is picked up, when the Player
 * wins or loses"). Nothing in the game rules mentions Score, so the
 * scoring formula can change without touching the World - in Pac-Man
 * the World called score->coin_collection() directly and the two were
 * welded together.
 *
 * The survival component is driven by game_events::Tick rather than
 * by a clock of its own, so pausing the game pauses the score.
 */
class Score final : public sif::event::Observer {
public:
    explicit Score(const std::shared_ptr<sif::event::Event_Bus>& world_bus, ScoreRules rules = {});

    [[nodiscard]] int points() const;

    /// @brief Whole seconds the player has survived this round.
    [[nodiscard]] int seconds_alive() const;

    [[nodiscard]] int blocks_destroyed() const;
    [[nodiscard]] int power_ups_taken() const;
    [[nodiscard]] int enemies_killed() const;

    [[nodiscard]] const ScoreRules& rules() const;

private:
    ScoreRules rules_;

    int points_ = 0;
    float seconds_alive_ = 0.f;
    int whole_seconds_awarded_ = 0;
    int blocks_destroyed_ = 0;
    int power_ups_taken_ = 0;
    int enemies_killed_ = 0;
};
} // namespace bomberman::logic

#endif // BOMBERMAN_LOGIC_SCORE_H
