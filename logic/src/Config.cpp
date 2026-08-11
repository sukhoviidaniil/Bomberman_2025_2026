/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "bomberman/logic/Config.h"

#include <filesystem>
#include <stdexcept>

#include "sif/io/from_JSON.h"

namespace bomberman::logic {
    namespace {
        /// Reads an optional sub-object; returns an empty object if absent.
        nlohmann::json section(const nlohmann::json& j, const std::string& key) {
            if (!j.contains(key)) {
                return nlohmann::json::object();
            }
            if (!j.at(key).is_object()) {
                throw std::runtime_error("config: '" + key + "' must be an object");
            }
            return j.at(key);
        }

        std::optional<std::uint32_t> optional_seed(const nlohmann::json& j, const std::string& key) {
            if (!j.contains(key) || j.at(key).is_null()) {
                return std::nullopt;
            }
            return j.at(key).get<std::uint32_t>();
        }
    }

    GameConfig GameConfig::load(const std::string &filepath) {
        if (!std::filesystem::exists(filepath)) {
            throw std::runtime_error("config: file does not exist: " + filepath);
        }

        // sif's reader already turns a parse failure into a message naming
        // the file, so there is nothing to add here.
        const nlohmann::json j = sif::io::get_json_data(filepath);
        if (!j.is_object()) {
            throw std::runtime_error("config: '" + filepath + "' must contain a JSON object");
        }

        GameConfig config;

        config.random_seed = optional_seed(j, "random_seed");

        const nlohmann::json map = section(j, "map");
        config.map.rows = sif::io::get_checked<std::size_t>(map, "rows", config.map.rows);
        config.map.columns = sif::io::get_checked<std::size_t>(map, "columns", config.map.columns);
        config.map.destructible_chance =
            sif::io::get_checked<float>(map, "destructible_chance", config.map.destructible_chance);
        config.map.seed = optional_seed(map, "seed");

        if (map.contains("layout") && !map.at("layout").is_null()) {
            if (!map.at("layout").is_array()) {
                throw std::runtime_error("config: 'map.layout' must be an array of strings");
            }
            config.map.layout = map.at("layout").get<std::vector<std::string>>();
        }

        const nlohmann::json round = section(j, "round");
        config.round.character_speed =
            sif::io::get_checked<float>(round, "character_speed", config.round.character_speed);
        config.round.character_size =
            sif::io::get_checked<float>(round, "character_size", config.round.character_size);
        config.round.bomb_fuse_seconds =
            sif::io::get_checked<float>(round, "bomb_fuse_seconds", config.round.bomb_fuse_seconds);
        config.round.explosion_seconds =
            sif::io::get_checked<float>(round, "explosion_seconds", config.round.explosion_seconds);
        config.round.bot_count =
            sif::io::get_checked<std::size_t>(round, "bot_count", config.round.bot_count);

        if (round.contains("bot_personalities") && !round.at("bot_personalities").is_null()) {
            if (!round.at("bot_personalities").is_array()) {
                throw std::runtime_error("config: 'round.bot_personalities' must be an array of strings");
            }
            for (const auto& entry : round.at("bot_personalities")) {
                // personality_from_string names the offending value and the
                // accepted ones, so a typo is a one-line fix rather than a
                // hunt through the source.
                config.round.bot_personalities.push_back(
                    ai::personality_from_string(entry.get<std::string>()));
            }
        }

        const nlohmann::json power_ups = section(j, "power_ups");
        config.power_ups.drop_chance =
            sif::io::get_checked<float>(power_ups, "drop_chance", config.power_ups.drop_chance);
        config.power_ups.fire_weight =
            sif::io::get_checked<float>(power_ups, "fire_weight", config.power_ups.fire_weight);
        config.power_ups.extra_bomb_weight =
            sif::io::get_checked<float>(power_ups, "extra_bomb_weight", config.power_ups.extra_bomb_weight);
        config.power_ups.skates_weight =
            sif::io::get_checked<float>(power_ups, "skates_weight", config.power_ups.skates_weight);
        config.power_ups.max_blast_radius =
            sif::io::get_checked<unsigned int>(power_ups, "max_blast_radius", config.power_ups.max_blast_radius);
        config.power_ups.max_bomb_budget =
            sif::io::get_checked<std::size_t>(power_ups, "max_bomb_budget", config.power_ups.max_bomb_budget);
        config.power_ups.skates_speed_bonus =
            sif::io::get_checked<float>(power_ups, "skates_speed_bonus", config.power_ups.skates_speed_bonus);
        config.power_ups.max_speed =
            sif::io::get_checked<float>(power_ups, "max_speed", config.power_ups.max_speed);

        const nlohmann::json score = section(j, "score");
        config.score.per_second_alive =
            sif::io::get_checked<int>(score, "per_second_alive", config.score.per_second_alive);
        config.score.per_block_destroyed =
            sif::io::get_checked<int>(score, "per_block_destroyed", config.score.per_block_destroyed);
        config.score.per_power_up =
            sif::io::get_checked<int>(score, "per_power_up", config.score.per_power_up);
        config.score.per_enemy_killed =
            sif::io::get_checked<int>(score, "per_enemy_killed", config.score.per_enemy_killed);
        config.score.win_bonus =
            sif::io::get_checked<int>(score, "win_bonus", config.score.win_bonus);
        config.score.loss_penalty =
            sif::io::get_checked<int>(score, "loss_penalty", config.score.loss_penalty);

        const nlohmann::json window = section(j, "window");
        config.window.title = sif::io::get_checked<std::string>(window, "title", config.window.title);
        config.window.width = sif::io::get_checked<unsigned int>(window, "width", config.window.width);
        config.window.height = sif::io::get_checked<unsigned int>(window, "height", config.window.height);
        config.window.fps = sif::io::get_checked<int>(window, "fps", config.window.fps);

        const nlohmann::json audio = section(j, "audio");
        config.audio.enabled = sif::io::get_checked<bool>(audio, "enabled", config.audio.enabled);
        config.audio.master_volume =
            sif::io::get_checked<float>(audio, "master_volume", config.audio.master_volume);
        config.audio.sfx_volume =
            sif::io::get_checked<float>(audio, "sfx_volume", config.audio.sfx_volume);

        config.validate();
        return config;
    }

    void GameConfig::validate() const {
        // A configuration file is edited by hand, so every complaint names
        // the key and the value: "config: map.rows must be at least 3" is
        // actionable, "invalid configuration" is not.
        if (!map.layout.empty()) {
            if (map.layout.size() < 3) {
                throw std::runtime_error("config: map.layout needs at least 3 rows");
            }
            const std::size_t width = map.layout.front().size();
            if (width < 3) {
                throw std::runtime_error("config: map.layout rows need at least 3 columns");
            }
            for (std::size_t i = 0; i < map.layout.size(); ++i) {
                if (map.layout[i].size() != width) {
                    throw std::runtime_error(
                        "config: map.layout row " + std::to_string(i) + " is " +
                        std::to_string(map.layout[i].size()) + " characters wide, expected " +
                        std::to_string(width) + " - every row must be the same length");
                }
            }
        } else if (map.rows < 3 || map.columns < 3) {
            throw std::runtime_error("config: map.rows and map.columns must be at least 3");
        }

        if (map.destructible_chance < 0.f || map.destructible_chance > 1.f) {
            throw std::runtime_error("config: map.destructible_chance must be within [0, 1]");
        }
        power_ups.validate();

        if (round.character_speed <= 0.f) {
            throw std::runtime_error("config: round.character_speed must be positive");
        }
        if (round.character_size <= 0.f || round.character_size > 1.f) {
            throw std::runtime_error("config: round.character_size must be within (0, 1]");
        }
        if (round.bomb_fuse_seconds <= 0.f) {
            throw std::runtime_error("config: round.bomb_fuse_seconds must be positive");
        }
        if (round.explosion_seconds <= 0.f) {
            throw std::runtime_error("config: round.explosion_seconds must be positive");
        }

        if (window.width < 320 || window.height < 240) {
            throw std::runtime_error("config: window.width/height are unusably small");
        }
        if (audio.master_volume < 0.f || audio.master_volume > 1.f) {
            throw std::runtime_error("config: audio.master_volume must be within [0, 1]");
        }
    }
}
