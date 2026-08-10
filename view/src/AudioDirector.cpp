/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/view/AudioDirector.h"

#include <algorithm>

#include "bomberman/logic/events/GameEvents.h"
#include "bomberman/view/AssetNames.h"

namespace bomberman::view {

    AudioDirector::AudioDirector(const std::shared_ptr<sif::event::Event_Bus> &world_bus,
                                 sif::audio::AudioPlayer &audio,
                                 const GameAssets &assets,
                                 const logic::AudioConfig &config)
        : audio_(audio), assets_(assets), config_(config) {

        audio_.set_master_volume(config_.enabled ? config_.master_volume : 0.f);

        track(world_bus->subscribe<logic::game_events::BombPlaced>(
            [this](const logic::game_events::BombPlaced& e) {
                // Quieter for bots: three of them dropping bombs at the
                // far end of the arena should not drown out the player.
                play(assets::sfx_bomb_place, e.by_player ? 1.f : 0.45f);
            }));

        track(world_bus->subscribe<logic::game_events::BombExploded>(
            [this](const logic::game_events::BombExploded&) {
                // Once per bomb, not once per burning tile - a radius-4
                // blast would otherwise fire seventeen overlapping copies
                // of the same sample.
                play(assets::sfx_explosion);
            }));

        track(world_bus->subscribe<logic::game_events::PowerUpTaken>(
            [this](const logic::game_events::PowerUpTaken& e) {
                play(assets::sfx_pickup, e.by_player ? 1.f : 0.4f);
            }));

        track(world_bus->subscribe<logic::game_events::CharacterKilled>(
            [this](const logic::game_events::CharacterKilled& e) {
                play(assets::sfx_death, e.victim == logic::CharacterKind::Player ? 1.f : 0.6f);
            }));

        track(world_bus->subscribe<logic::game_events::RoundEnded>(
            [this](const logic::game_events::RoundEnded& e) {
                play(e.player_won ? assets::sfx_victory : assets::sfx_defeat);
            }));
    }

    void AudioDirector::play(const std::string &asset_name, const float volume_scale) const {
        if (!config_.enabled) {
            return;
        }

        // A sound that has not finished loading is not an error; the
        // player returns an invalid voice and the game carries on.
        audio_.play(assets_.sound(asset_name),
                    std::clamp(config_.sfx_volume * volume_scale, 0.f, 1.f));
    }
}
