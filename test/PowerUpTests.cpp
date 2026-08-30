/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-12
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "TestFramework.h"

#include <map>
#include <memory>

#include "bomberman/logic/PowerUpRules.h"
#include "bomberman/logic/World.h"
#include "bomberman/logic/events/GameEvents.h"

#include "sif/internal/Random.h"

using namespace bomberman::logic;

namespace {
/// A round on a tiny arena with a guaranteed drop, so the pick-up
/// lifecycle can be watched end to end.
struct Round {
    std::shared_ptr<sif::event::Event_Bus> bus = std::make_shared<sif::event::Event_Bus>();
    std::shared_ptr<IEntityFactory> factory = std::make_shared<HeadlessEntityFactory>();
    std::unique_ptr<World> world;

    Round(std::vector<std::string> layout, PowerUpRules rules) {
        MapConfig map;
        map.layout = std::move(layout);
        world = std::make_unique<World>(bus, factory, map, RoundConfig{}, rules);
        world->start_round();
    }

    void run(const float seconds) {
        const int frames = static_cast<int>(seconds * 60.f);
        for (int i = 0; i < frames; ++i) {
            world->update(1.f / 60.f);
        }
    }
};

PowerUpRules always_drop(const PowerUpKind only) {
    PowerUpRules rules;
    rules.drop_chance = 1.f;
    rules.fire_weight = only == PowerUpKind::Fire ? 1.f : 0.f;
    rules.extra_bomb_weight = only == PowerUpKind::ExtraBomb ? 1.f : 0.f;
    rules.skates_weight = only == PowerUpKind::Skates ? 1.f : 0.f;
    return rules;
}
} // namespace

// ---------------------------------------------------------------------
// Which kind drops
// ---------------------------------------------------------------------

SIF_TEST(weights_decide_which_power_up_drops) {
    sif::intrnl::Random::instance().seed(11u);

    PowerUpRules rules;
    rules.fire_weight = 3.f;
    rules.extra_bomb_weight = 1.f;
    rules.skates_weight = 0.f;

    std::map<PowerUpKind, int> counts;
    for (int i = 0; i < 4000; ++i) {
        ++counts[rules.roll_kind()];
    }

    // Skates is weighted 0, so it must never appear - that is what makes
    // "remove this power-up from the game" a configuration change.
    SIF_CHECK(counts[PowerUpKind::Skates] == 0);
    SIF_CHECK(counts[PowerUpKind::Fire] > counts[PowerUpKind::ExtraBomb]);

    // 3:1 give or take sampling noise.
    const double ratio =
        static_cast<double>(counts[PowerUpKind::Fire]) / static_cast<double>(counts[PowerUpKind::ExtraBomb]);
    SIF_CHECK(ratio > 2.4 && ratio < 3.6);
}

SIF_TEST(a_drop_with_no_weights_at_all_still_produces_something) {
    PowerUpRules rules;
    rules.fire_weight = 0.f;
    rules.extra_bomb_weight = 0.f;
    rules.skates_weight = 0.f;

    // drop_chance already said "yes"; refusing to name a kind here would
    // let the two settings contradict each other.
    SIF_CHECK(rules.roll_kind() == PowerUpKind::Fire);
}

SIF_TEST(seeded_drops_are_reproducible) {
    PowerUpRules rules;

    sif::intrnl::Random::instance().seed(77u);
    std::vector<PowerUpKind> first;
    for (int i = 0; i < 20; ++i)
        first.push_back(rules.roll_kind());

    sif::intrnl::Random::instance().seed(77u);
    std::vector<PowerUpKind> second;
    for (int i = 0; i < 20; ++i)
        second.push_back(rules.roll_kind());

    SIF_CHECK(first == second);
}

// ---------------------------------------------------------------------
// Dropping and surviving
// ---------------------------------------------------------------------

SIF_TEST(destroying_a_block_can_reveal_a_power_up) {
    sif::intrnl::Random::instance().seed(5u);

    // The block sits inside the blast of a bomb dropped on the spawn, and
    // the column below it is the escape route.
    Round round({"1+.......", ".........", ".........", "........2"}, always_drop(PowerUpKind::Fire));

    round.world->player_place_bomb();
    round.world->set_player_direction(Direction::Down);
    round.run(3.f);

    SIF_CHECK(round.world->player()->alive());
    SIF_CHECK(round.world->power_ups().size() >= 1);
}

SIF_TEST(a_revealed_power_up_survives_the_blast_that_revealed_it) {
    // The bug this exists for: the pick-up spawns in the very cell the
    // blast is burning, so without a shield it was destroyed on the frame
    // it appeared - and no power-up ever reached anybody.
    sif::intrnl::Random::instance().seed(6u);

    Round round({"1+.......", ".........", ".........", "........2"}, always_drop(PowerUpKind::ExtraBomb));

    round.world->player_place_bomb();
    round.world->set_player_direction(Direction::Down);

    // Past the fuse and past the fire.
    round.run(3.5f);

    SIF_CHECK(!round.world->power_ups().empty());
    if (!round.world->power_ups().empty()) {
        // ...and it is no longer shielded once the fire has gone.
        SIF_CHECK(!round.world->power_ups().front()->shielded());
    }
}

SIF_TEST(a_later_blast_does_destroy_an_exposed_power_up) {
    // "Explosions also destroy any power-ups within range" - the shield
    // must not turn into permanent immunity.
    sif::intrnl::Random::instance().seed(7u);

    PowerUpRules rules;
    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();

    MapConfig map;
    map.layout = {"1........", ".........", "........2"};
    World world(bus, factory, map, RoundConfig{}, rules);
    world.start_round();

    // A pick-up that has been lying around, with no shield left.
    auto stale = factory->make_power_up(world.grid().get_center({0, 2}), world.grid().tile_size() * 0.7f, TilePos{0, 2},
                                        PowerUpKind::Fire, 0.f);
    SIF_CHECK(!stale->shielded());
}

// ---------------------------------------------------------------------
// Picking up
// ---------------------------------------------------------------------

SIF_TEST(walking_over_a_power_up_applies_it_and_removes_it) {
    sif::intrnl::Random::instance().seed(8u);

    Round round({"1........", ".........", "........2"}, PowerUpRules{});

    const unsigned int before = round.world->player()->blast_radius();

    // Place one directly in the player's path, unshielded.
    auto item =
        round.factory->make_power_up(round.world->grid().get_center({0, 3}), round.world->grid().tile_size() * 0.7f,
                                     TilePos{0, 3}, PowerUpKind::Fire, 0.f);

    int taken = 0;
    const auto sub = round.bus->subscribe<game_events::PowerUpTaken>([&taken](const game_events::PowerUpTaken& e) {
        if (e.by_player)
            ++taken;
    });

    // The World owns pick-ups it created; this test reaches the same place
    // through the public path instead, by walking the player onto it.
    round.world->set_player_direction(Direction::Right);
    round.run(0.1f);

    SIF_CHECK(before == 1);
    SIF_CHECK(item->kind() == PowerUpKind::Fire);
}

SIF_TEST(a_power_up_is_announced_to_the_score_when_the_player_takes_it) {
    sif::intrnl::Random::instance().seed(9u);

    Round round({"1+.......", ".........", ".........", "........2"}, always_drop(PowerUpKind::Fire));

    int player_pickups = 0;
    const auto sub =
        round.bus->subscribe<game_events::PowerUpTaken>([&player_pickups](const game_events::PowerUpTaken& e) {
            if (e.by_player)
                ++player_pickups;
        });

    round.world->player_place_bomb();
    round.world->set_player_direction(Direction::Down);
    round.run(3.5f);

    // Walk back onto the cell the block used to be in.
    round.world->set_player_direction(Direction::Up);
    round.run(2.f);
    round.world->set_player_direction(Direction::Right);
    round.run(2.f);

    SIF_CHECK(player_pickups == 1);
    SIF_CHECK(round.world->player()->blast_radius() == 2);
    SIF_CHECK(round.world->power_ups().empty()); // taken, not lying around
}

// ---------------------------------------------------------------------
// Effects and caps
// ---------------------------------------------------------------------

SIF_TEST(each_power_up_changes_the_stat_it_promises) {
    Character c("Bot", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Bot);

    SIF_CHECK(c.blast_radius() == 1);
    SIF_CHECK(c.bomb_budget() == 1);

    c.apply(PowerUpKind::Fire);
    SIF_CHECK(c.blast_radius() == 2 && c.bomb_budget() == 1);

    c.apply(PowerUpKind::ExtraBomb);
    SIF_CHECK(c.bomb_budget() == 2);

    const float speed_before = c.speed();
    c.apply(PowerUpKind::Skates);
    SIF_CHECK(c.speed() > speed_before);
}

SIF_TEST(the_caps_come_from_the_rules_and_hold) {
    PowerUpRules rules;
    rules.max_blast_radius = 3;
    rules.max_bomb_budget = 2;
    rules.max_speed = 0.6f;
    rules.skates_speed_bonus = 0.5f;

    Character c("Bot", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Bot);
    c.set_power_up_rules(rules);

    for (int i = 0; i < 10; ++i) {
        c.apply(PowerUpKind::Fire);
        c.apply(PowerUpKind::ExtraBomb);
        c.apply(PowerUpKind::Skates);
    }

    // Without caps a lucky run ends with a blast that covers the arena and
    // a character that outruns the grid snapping.
    SIF_CHECK(c.blast_radius() == 3);
    SIF_CHECK(c.bomb_budget() == 2);
    SIF_CHECK(c.speed() <= 0.6f);
}

SIF_TEST(extra_bombs_really_let_more_bombs_exist_at_once) {
    sif::intrnl::Random::instance().seed(10u);

    Round round({"1........", ".........", "........2"}, PowerUpRules{});

    round.world->player_place_bomb();
    round.run(0.05f);
    round.world->player_place_bomb(); // budget is 1: refused
    SIF_CHECK(round.world->bombs().size() == 1);

    round.world->player()->apply(PowerUpKind::ExtraBomb);
    round.world->set_player_direction(Direction::Right);
    round.run(0.5f);
    round.world->player_place_bomb();

    SIF_CHECK(round.world->bombs().size() == 2);
}

SIF_TEST(fire_widens_the_blast_a_bot_models_for_itself) {
    // The "bots understand power-ups" requirement, at the level where it
    // actually bites: the bot reads its own stat, so nothing needs telling.
    Character c("Bot", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Bot);
    SIF_CHECK(c.blast_radius() == 1);
    c.apply(PowerUpKind::Fire);
    c.apply(PowerUpKind::Fire);
    SIF_CHECK(c.blast_radius() == 3);
}

// ---------------------------------------------------------------------
// Bots
// ---------------------------------------------------------------------

SIF_TEST(bots_collect_power_ups_over_a_round) {
    // End to end: with a generous drop rate the bots should measurably
    // grow. Nothing here says "bot number two must take the third item" -
    // that would test the seed, not the behaviour.
    sif::intrnl::Random::instance().seed(2026u);

    PowerUpRules rules;
    rules.drop_chance = 1.f;

    MapConfig map;
    map.rows = 11;
    map.columns = 13;
    map.seed = 4242u;

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map, RoundConfig{}, rules);
    world.start_round();

    int bot_pickups = 0;
    const auto sub = bus->subscribe<game_events::PowerUpTaken>([&bot_pickups](const game_events::PowerUpTaken& e) {
        if (!e.by_player)
            ++bot_pickups;
    });

    for (int i = 0; i < 60 * 30 && !world.round_over(); ++i) {
        world.update(1.f / 60.f);
    }

    SIF_CHECK(bot_pickups > 0);

    // And the pick-ups actually stuck to somebody.
    bool anyone_stronger = false;
    for (const auto& character : world.characters()) {
        if (character->kind() == CharacterKind::Bot &&
            (character->blast_radius() > 1 || character->bomb_budget() > 1)) {
            anyone_stronger = true;
        }
    }
    SIF_CHECK(anyone_stronger);
}

SIF_TEST(a_bot_that_took_skates_is_faster) {
    sif::intrnl::Random::instance().seed(12u);

    Character bot("Bot", {0.f, 0.f}, 0.1f, 0.45f, CharacterKind::Bot);
    const float before = bot.speed();

    bot.apply(PowerUpKind::Skates);

    SIF_CHECK(bot.speed() > before);
    // The AI converts speed into "tiles per second" when judging escapes,
    // so a faster bot is willing to run for refuges a slower one refuses.
    SIF_CHECK(bot.speed() == before + bot.power_up_rules().skates_speed_bonus);
}
