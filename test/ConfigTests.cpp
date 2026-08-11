/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-06
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
***************************************************************/

#include "TestFramework.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "bomberman/logic/Config.h"
#include "bomberman/logic/World.h"
#include "bomberman/logic/grid/TileGrid.h"

#include "sif/internal/Random.h"

using namespace bomberman::logic;

namespace {
    /// Writes a config file into the temp directory and returns its path.
    std::string temp_config(const std::string& name, const std::string& contents) {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
        std::ofstream(path) << contents;
        return path.string();
    }

    template<class Fn>
    bool throws(Fn&& fn) {
        try {
            fn();
        } catch (const std::exception&) {
            return true;
        }
        return false;
    }

    /// Renders a grid as characters, so two arenas can be compared exactly.
    std::string fingerprint(const TileGrid& grid) {
        std::string out;
        for (int row = 0; row < static_cast<int>(grid.rows()); ++row) {
            for (int col = 0; col < static_cast<int>(grid.columns()); ++col) {
                switch (grid.get_tile({row, col})) {
                    case Tile::Indestructible: out += '#'; break;
                    case Tile::Destructible:   out += '+'; break;
                    default:                   out += '.'; break;
                }
            }
            out += '\n';
        }
        return out;
    }
}

// ---------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------

SIF_TEST(an_empty_config_object_keeps_every_default) {
    const GameConfig config = GameConfig::load(temp_config("bm_empty.json", "{}"));
    const GameConfig defaults;

    SIF_CHECK(!config.random_seed.has_value());
    SIF_CHECK(config.map.rows == defaults.map.rows);
    SIF_CHECK(config.round.bot_count == defaults.round.bot_count);
    SIF_CHECK(config.score.per_enemy_killed == defaults.score.per_enemy_killed);
    SIF_CHECK(config.window.width == defaults.window.width);
}

SIF_TEST(config_values_override_the_defaults) {
    const GameConfig config = GameConfig::load(temp_config("bm_values.json", R"({
        "random_seed": 42,
        "map": { "rows": 9, "columns": 15, "seed": 7 },
        "power_ups": { "drop_chance": 0.5, "skates_weight": 0.0, "max_blast_radius": 3 },
        "round": { "bot_count": 1, "bomb_fuse_seconds": 3.0 },
        "score": { "per_enemy_killed": 999 },
        "window": { "width": 1280, "height": 800, "title": "custom" },
        "audio": { "enabled": false, "master_volume": 0.25 }
    })"));

    SIF_CHECK(config.random_seed.has_value() && *config.random_seed == 42u);
    SIF_CHECK(config.map.rows == 9 && config.map.columns == 15);
    SIF_CHECK(config.power_ups.drop_chance == 0.5f);
    SIF_CHECK(config.power_ups.skates_weight == 0.f);
    SIF_CHECK(config.power_ups.max_blast_radius == 3);
    SIF_CHECK(config.map.seed.has_value() && *config.map.seed == 7u);
    SIF_CHECK(config.round.bot_count == 1);
    SIF_CHECK(config.score.per_enemy_killed == 999);
    SIF_CHECK(config.window.width == 1280 && config.window.title == "custom");
    SIF_CHECK(!config.audio.enabled);
}

SIF_TEST(a_missing_config_file_is_reported_not_ignored) {
    SIF_CHECK(throws([] { (void)GameConfig::load("/tmp/definitely-not-here.json"); }));
}

SIF_TEST(invalid_config_values_are_rejected_with_a_reason) {
    // Each of these used to be the kind of thing that only shows up as
    // strange behaviour three screens later.
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad1.json", R"({"map": {"rows": 1}})"));
    }));
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad2.json", R"({"power_ups": {"drop_chance": 5.0}})"));
    }));
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad5.json", R"({"power_ups": {"fire_weight": -1.0}})"));
    }));
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad6.json", R"({"power_ups": {"max_speed": 0.0}})"));
    }));
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad3.json", R"({"round": {"character_speed": -1.0}})"));
    }));
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_bad4.json", R"({"audio": {"master_volume": 2.0}})"));
    }));
}

SIF_TEST(a_ragged_layout_is_rejected) {
    SIF_CHECK(throws([] {
        (void)GameConfig::load(temp_config("bm_ragged.json", R"({
            "map": { "layout": ["#####", "#...#", "###"] }
        })"));
    }));
}

// ---------------------------------------------------------------------
// Seeded map generation
// ---------------------------------------------------------------------

SIF_TEST(the_same_map_seed_produces_the_same_arena) {
    TileGrid a(11, 13);
    a.generate_arena(0.75f, 12345u);

    TileGrid b(11, 13);
    b.generate_arena(0.75f, 12345u);

    SIF_CHECK(fingerprint(a) == fingerprint(b));
}

SIF_TEST(different_map_seeds_produce_different_arenas) {
    TileGrid a(11, 13);
    a.generate_arena(0.75f, 1u);

    TileGrid b(11, 13);
    b.generate_arena(0.75f, 2u);

    SIF_CHECK(fingerprint(a) != fingerprint(b));
}

SIF_TEST(a_seeded_arena_does_not_depend_on_earlier_draws) {
    // The point of seeding: whatever happened before, the arena is the
    // same. Without this a menu animation that drew a random number would
    // silently change the "reproducible" map.
    sif::intrnl::Random::instance().seed(999u);
    TileGrid a(9, 9);
    a.generate_arena(0.6f, 555u);

    for (int i = 0; i < 137; ++i) {
        (void)sif::intrnl::rand_int(0, 100);
    }

    TileGrid b(9, 9);
    b.generate_arena(0.6f, 555u);

    SIF_CHECK(fingerprint(a) == fingerprint(b));
}

// ---------------------------------------------------------------------
// Explicit layouts
// ---------------------------------------------------------------------

SIF_TEST(a_layout_is_used_verbatim) {
    const std::vector<std::string> layout = {
        "1.+#",
        "#+..",
        "..+2"
    };

    const TileGrid grid = TileGrid::from_layout(layout);

    SIF_CHECK(grid.rows() == 3 && grid.columns() == 4);
    SIF_CHECK(grid.get_tile({0, 0}) == Tile::Free);          // '1' is a spawn, hence free
    SIF_CHECK(grid.get_tile({0, 2}) == Tile::Destructible);
    SIF_CHECK(grid.get_tile({0, 3}) == Tile::Indestructible);
    SIF_CHECK(grid.get_tile({1, 0}) == Tile::Indestructible);
}

SIF_TEST(layout_digits_decide_where_characters_spawn) {
    const TileGrid grid = TileGrid::from_layout({
        "..2",
        "...",
        "1.."
    });

    const std::vector<TilePos>& spawns = grid.spawn_cells();
    SIF_CHECK(spawns.size() == 2);
    // Digit order, not reading order: '1' is always the player.
    SIF_CHECK(spawns[0] == TilePos(2, 0));
    SIF_CHECK(spawns[1] == TilePos(0, 2));
}

SIF_TEST(a_layout_without_digits_falls_back_to_the_corners) {
    const TileGrid grid = TileGrid::from_layout({
        "...",
        "...",
        "..."
    });

    SIF_CHECK(grid.spawn_cells().size() == 4);
    SIF_CHECK(grid.spawn_cells().front() == TilePos(0, 0));
}

SIF_TEST(an_unknown_layout_character_is_rejected) {
    SIF_CHECK(throws([] { (void)TileGrid::from_layout({"..%", "...", "..."}); }));
    SIF_CHECK(throws([] { (void)TileGrid::from_layout({}); }));
}

SIF_TEST(the_world_uses_the_layout_in_preference_to_a_seed) {
    MapConfig map;
    map.seed = 1u;                 // would generate a 11x13 arena...
    map.layout = {                 // ...but the layout wins
        "1..2",
        "....",
        "3..4"
    };

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map);
    world.start_round();

    SIF_CHECK(world.grid().rows() == 3);
    SIF_CHECK(world.grid().columns() == 4);
    SIF_CHECK(world.characters().size() == 4);
}

SIF_TEST(a_layout_with_few_spawns_spawns_few_bots) {
    MapConfig map;
    map.layout = {"1..", "...", "..2"};

    RoundConfig round;
    round.bot_count = 3; // more bots than the map has room for

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map, round);
    world.start_round();

    // One player plus one bot: stacking two characters on one cell would
    // kill both with the first bomb.
    SIF_CHECK(world.characters().size() == 2);
}
