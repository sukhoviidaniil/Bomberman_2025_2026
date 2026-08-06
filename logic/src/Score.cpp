/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-21
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/Score.h"

namespace bomberman::logic {

    Score::Score(const std::shared_ptr<sif::event::Event_Bus> &world_bus, const ScoreRules rules)
        : rules_(rules) {

        track(world_bus->subscribe<game_events::Tick>(
            [this](const game_events::Tick& e) {
                seconds_alive_ += e.dt;

                // Award in whole seconds so the displayed score does not
                // flicker with the frame rate, while the underlying
                // accumulator stays frame-rate independent.
                //
                // The tolerance is not decoration: sixty frames of 1/60 s
                // sum to 0.99999994f, so a plain cast would drop one
                // second per minute of play. A unit test caught this
                // exactly once and would catch it again.
                constexpr float tolerance = 1e-3f;
                const auto whole = static_cast<int>(seconds_alive_ + tolerance);
                if (whole > whole_seconds_awarded_) {
                    points_ += (whole - whole_seconds_awarded_) * rules_.per_second_alive;
                    whole_seconds_awarded_ = whole;
                }
            }));

        track(world_bus->subscribe<game_events::BlockDestroyed>(
            [this](const game_events::BlockDestroyed& e) {
                if (!e.by_player) {
                    return; // a bot clearing its own way is not the player's achievement
                }
                ++blocks_destroyed_;
                points_ += rules_.per_block_destroyed;
            }));

        track(world_bus->subscribe<game_events::PowerUpTaken>(
            [this](const game_events::PowerUpTaken& e) {
                if (!e.by_player) {
                    return;
                }
                ++power_ups_taken_;
                points_ += rules_.per_power_up;
            }));

        track(world_bus->subscribe<game_events::CharacterKilled>(
            [this](const game_events::CharacterKilled& e) {
                if (e.victim == CharacterKind::Player || !e.by_player) {
                    return;
                }
                ++enemies_killed_;
                points_ += rules_.per_enemy_killed;
            }));

        track(world_bus->subscribe<game_events::RoundEnded>(
            [this](const game_events::RoundEnded& e) {
                points_ += e.player_won ? rules_.win_bonus : -rules_.loss_penalty;
                if (points_ < 0) {
                    points_ = 0; // a negative high score would be odd on the board
                }
            }));
    }

    int Score::points() const { return points_; }
    int Score::seconds_alive() const { return whole_seconds_awarded_; }
    int Score::blocks_destroyed() const { return blocks_destroyed_; }
    int Score::power_ups_taken() const { return power_ups_taken_; }
    int Score::enemies_killed() const { return enemies_killed_; }
    const ScoreRules & Score::rules() const { return rules_; }
}
