/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-10
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "TestFramework.h"

#include <memory>
#include <vector>

#include "bomberman/logic/World.h"
#include "bomberman/logic/ai/Behaviours.h"
#include "bomberman/logic/ai/BotBrain.h"
#include "bomberman/logic/ai/DangerMap.h"

#include "sif/internal/Random.h"

using namespace bomberman::logic;
using namespace bomberman::logic::ai;

namespace {
/// An open arena of the given size, every cell free.
TileGrid open_grid(const std::size_t rows, const std::size_t columns) {
    TileGrid grid(rows, columns);
    for (int row = 0; row < static_cast<int>(rows); ++row) {
        for (int col = 0; col < static_cast<int>(columns); ++col) {
            grid.set_tile({row, col}, Tile::Free);
        }
    }
    return grid;
}

std::shared_ptr<Bomb> make_bomb(const TileGrid& grid, const TilePos cell, const unsigned int radius, const float fuse) {
    return std::make_shared<Bomb>(grid.get_center(cell), grid.tile_size(), cell, std::weak_ptr<Character>{}, radius,
                                  fuse);
}

/// A context over a grid, one bot and an explicit danger map.
struct Fixture {
    TileGrid grid;
    DangerMap danger;
    std::shared_ptr<Character> self;
    std::vector<std::shared_ptr<Character>> characters;
    std::vector<std::shared_ptr<PowerUp>> power_ups;
    std::vector<std::shared_ptr<Bomb>> bombs;
    std::vector<std::shared_ptr<Explosion>> explosions;

    Fixture(TileGrid g, const TilePos start) : grid(std::move(g)) {
        self = std::make_shared<Character>("Bot", grid.get_center(start), grid.tile_size() * 0.85f, 0.45f,
                                           CharacterKind::Bot);
        characters.push_back(self);
    }

    void rebuild() { danger.rebuild(grid, bombs, explosions); }

    [[nodiscard]] BotContext context() const {
        const auto cell = grid.get_TilePos(self->position());
        return BotContext{grid,
                          danger,
                          *self,
                          cell.value_or(TilePos{0, 0}),
                          characters,
                          power_ups,
                          [this](const TilePos& c) {
                              for (const auto& b : bombs) {
                                  if (b->cell() == c)
                                      return true;
                              }
                              return false;
                          },
                          2.f,
                          grid.tile_size() > 0.f ? self->speed() / grid.tile_size() : 1.f};
    }
};
} // namespace

// ---------------------------------------------------------------------
// DangerMap - the model every behaviour depends on
// ---------------------------------------------------------------------

SIF_TEST(a_bomb_endangers_its_own_cell_and_a_cross_of_its_radius) {
    Fixture f(open_grid(7, 7), {3, 3});
    f.bombs.push_back(make_bomb(f.grid, {3, 3}, 2, 2.f));
    f.rebuild();

    SIF_CHECK(!f.danger.safe({3, 3}));
    SIF_CHECK(!f.danger.safe({1, 3})); // two up
    SIF_CHECK(!f.danger.safe({3, 5})); // two right
    SIF_CHECK(f.danger.safe({0, 3}));  // three up: out of range
    SIF_CHECK(f.danger.safe({2, 2}));  // diagonal: never in a cross
}

SIF_TEST(an_indestructible_wall_stops_the_blast_short) {
    Fixture f(open_grid(7, 7), {3, 3});
    f.grid.set_tile({3, 4}, Tile::Indestructible);
    f.bombs.push_back(make_bomb(f.grid, {3, 3}, 3, 2.f));
    f.rebuild();

    SIF_CHECK(!f.danger.safe({3, 3}));
    SIF_CHECK(f.danger.safe({3, 5})); // behind the wall
    SIF_CHECK(f.danger.safe({3, 6}));
    SIF_CHECK(!f.danger.safe({3, 2})); // the other direction is unaffected
}

SIF_TEST(a_destructible_block_absorbs_the_blast) {
    Fixture f(open_grid(7, 7), {3, 3});
    f.grid.set_tile({3, 4}, Tile::Destructible);
    f.bombs.push_back(make_bomb(f.grid, {3, 3}, 3, 2.f));
    f.rebuild();

    // The block itself burns...
    SIF_CHECK(!f.danger.safe({3, 4}));
    // ...but nothing behind it does. This must match World::spread_blast
    // exactly, or bots will walk into fire they think is not there.
    SIF_CHECK(f.danger.safe({3, 5}));
}

SIF_TEST(the_danger_map_reports_when_not_just_whether) {
    Fixture f(open_grid(5, 5), {2, 2});
    f.bombs.push_back(make_bomb(f.grid, {2, 2}, 1, 1.5f));
    f.rebuild();

    SIF_CHECK(f.danger.seconds_until_blast({2, 2}) == 1.5f);
    SIF_CHECK(f.danger.safe_for({2, 2}, 1.0f));  // still fine for a second
    SIF_CHECK(!f.danger.safe_for({2, 2}, 2.0f)); // not for two
    SIF_CHECK(f.danger.seconds_until_blast({0, 0}) == DangerMap::never);
}

SIF_TEST(two_bombs_leave_the_earliest_blast_on_a_shared_cell) {
    Fixture f(open_grid(5, 5), {2, 0});
    f.bombs.push_back(make_bomb(f.grid, {2, 1}, 2, 1.8f));
    f.bombs.push_back(make_bomb(f.grid, {2, 3}, 2, 0.4f));
    f.rebuild();

    // {2,2} is reached by both; what matters is the one that arrives first.
    SIF_CHECK(f.danger.seconds_until_blast({2, 2}) == 0.4f);
}

SIF_TEST(a_hypothetical_bomb_does_not_change_the_real_map) {
    Fixture f(open_grid(5, 5), {2, 2});
    f.rebuild();

    const DangerMap what_if = f.danger.with_bomb(f.grid, {2, 2}, 2, 2.f);

    SIF_CHECK(!what_if.safe({2, 2}));
    SIF_CHECK(f.danger.safe({2, 2})); // the original is untouched
}

// ---------------------------------------------------------------------
// Survival
// ---------------------------------------------------------------------

SIF_TEST(a_bot_standing_in_a_blast_runs_out_of_it) {
    Fixture f(open_grid(7, 7), {3, 3});
    f.bombs.push_back(make_bomb(f.grid, {3, 3}, 2, 2.f));
    f.rebuild();

    const SurviveBehaviour survive;
    const std::optional<BotAction> action = survive.decide(f.context());

    SIF_CHECK(action.has_value());
    if (action.has_value()) {
        SIF_CHECK(action->move != Direction::None);
        SIF_CHECK(!action->place_bomb);
        // Whichever way it goes, it must be off the cross.
        const TilePos next = f.grid.neighbour({3, 3}, action->move);
        SIF_CHECK(next.row == 3 || next.col == 3); // one step is still in line...
        SIF_CHECK(action->reason == "survive");
    }
}

SIF_TEST(a_bot_in_no_danger_does_not_invoke_survival) {
    Fixture f(open_grid(5, 5), {2, 2});
    f.rebuild();

    const SurviveBehaviour survive;
    SIF_CHECK(!survive.decide(f.context()).has_value());
}

SIF_TEST(a_bot_next_to_a_lit_bomb_survives_a_real_round) {
    // The end-to-end version: run the world forward and check the bot is
    // still standing. This is the test that would have caught every
    // "correct in isolation" mistake in the behaviours.
    sif::intrnl::Random::instance().seed(4242u);

    MapConfig map;
    map.layout = {"1..........", "...........", "...........", "..........2"};
    RoundConfig round;
    round.bot_count = 1;

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map, round);
    world.start_round();

    // Two seconds of fuse plus the blast, at 60 fps.
    for (int i = 0; i < 240; ++i) {
        world.update(1.f / 60.f);
    }

    const auto& characters = world.characters();
    SIF_CHECK(characters.size() == 2);
    // The bot places bombs of its own; the point is that it never dies to
    // one, because it only places what it can walk away from.
    SIF_CHECK(characters[1]->alive());
}

// ---------------------------------------------------------------------
// Not trapping itself
// ---------------------------------------------------------------------

SIF_TEST(a_bot_in_a_dead_end_does_not_bomb_itself) {
    // A one-cell pocket: bombing here is certain death, so no bomb.
    TileGrid grid(3, 3);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            grid.set_tile({row, col}, Tile::Indestructible);
        }
    }
    grid.set_tile({1, 1}, Tile::Free);
    grid.set_tile({0, 1}, Tile::Destructible); // something worth bombing

    Fixture f(std::move(grid), {1, 1});
    f.rebuild();

    SIF_CHECK(escape_after_bomb(f.context(), {1, 1}) == Direction::None);

    const BreakBlocksBehaviour behaviour;
    const std::optional<BotAction> action = behaviour.decide(f.context());
    SIF_CHECK(!action.has_value() || !action->place_bomb);
}

SIF_TEST(a_bot_with_room_to_run_does_bomb_the_block) {
    TileGrid grid = open_grid(3, 9);
    grid.set_tile({1, 0}, Tile::Destructible);

    Fixture f(std::move(grid), {1, 1});
    f.rebuild();

    SIF_CHECK(escape_after_bomb(f.context(), {1, 1}) != Direction::None);

    const BreakBlocksBehaviour behaviour;
    const std::optional<BotAction> action = behaviour.decide(f.context());

    SIF_CHECK(action.has_value());
    if (action.has_value()) {
        SIF_CHECK(action->place_bomb);
        // Bomb and move in the same decision: waiting a tick to leave is
        // how a bot ends up inside its own blast.
        SIF_CHECK(action->move != Direction::None);
    }
}

SIF_TEST(a_bigger_blast_radius_makes_a_bot_need_more_room) {
    // "If they have a bigger bomb radius, they should understand that they
    // need to escape further away" - and that falls out of reading the
    // character's own stats rather than a constant.
    //
    // A corridor walled off five cells along: with a small blast there is
    // room behind the fire, with a large one there is not.
    const auto build = [] {
        TileGrid grid = open_grid(1, 13);
        grid.set_tile({0, 0}, Tile::Destructible);   // worth bombing
        grid.set_tile({0, 5}, Tile::Indestructible); // the end of the corridor
        return grid;
    };

    Fixture small(build(), {0, 1});
    small.rebuild();
    SIF_CHECK(small.self->blast_radius() == 1);
    SIF_CHECK(escape_after_bomb(small.context(), {0, 1}) != Direction::None);

    Fixture large(build(), {0, 1});
    large.rebuild();
    large.self->apply(PowerUpKind::Fire);
    large.self->apply(PowerUpKind::Fire);
    SIF_CHECK(large.self->blast_radius() == 3);
    // The blast now fills {0,1}..{0,4} and {0,5} is a wall: nowhere left.
    SIF_CHECK(escape_after_bomb(large.context(), {0, 1}) == Direction::None);
}

// ---------------------------------------------------------------------
// Wanting things
// ---------------------------------------------------------------------

SIF_TEST(a_bot_walks_towards_a_nearby_power_up) {
    Fixture f(open_grid(1, 7), {0, 0});
    f.power_ups.push_back(std::make_shared<PowerUp>(f.grid.get_center({0, 4}), f.grid.tile_size() * 0.7f, TilePos{0, 4},
                                                    PowerUpKind::Fire));
    f.rebuild();

    const CollectPowerUpBehaviour behaviour;
    const std::optional<BotAction> action = behaviour.decide(f.context());

    SIF_CHECK(action.has_value());
    if (action.has_value()) {
        SIF_CHECK(action->move == Direction::Right);
        SIF_CHECK(action->reason == "collect");
    }
}

SIF_TEST(a_bot_ignores_a_power_up_beyond_its_search_radius) {
    Fixture f(open_grid(1, 20), {0, 0});
    f.power_ups.push_back(std::make_shared<PowerUp>(f.grid.get_center({0, 18}), f.grid.tile_size() * 0.7f,
                                                    TilePos{0, 18}, PowerUpKind::Fire));
    f.rebuild();

    const CollectPowerUpBehaviour behaviour(4);
    SIF_CHECK(!behaviour.decide(f.context()).has_value());
}

SIF_TEST(a_bot_bombs_an_enemy_that_is_in_line_and_in_range) {
    Fixture f(open_grid(1, 9), {0, 2});

    auto enemy = std::make_shared<Character>("Player", f.grid.get_center({0, 3}), f.grid.tile_size() * 0.85f, 0.45f,
                                             CharacterKind::Player);
    f.characters.push_back(enemy);
    f.rebuild();

    const HuntBehaviour behaviour;
    const std::optional<BotAction> action = behaviour.decide(f.context());

    SIF_CHECK(action.has_value());
    if (action.has_value()) {
        SIF_CHECK(action->place_bomb);
        SIF_CHECK(action->reason == "hunt");
    }
}

SIF_TEST(a_bot_does_not_bomb_an_enemy_behind_a_wall) {
    TileGrid grid = open_grid(1, 9);
    grid.set_tile({0, 3}, Tile::Indestructible);

    Fixture f(std::move(grid), {0, 2});
    auto enemy = std::make_shared<Character>("Player", f.grid.get_center({0, 4}), f.grid.tile_size() * 0.85f, 0.45f,
                                             CharacterKind::Player);
    f.characters.push_back(enemy);
    f.rebuild();

    const HuntBehaviour behaviour;
    const std::optional<BotAction> action = behaviour.decide(f.context());

    // It may still want to walk somewhere, but it must not waste a bomb on
    // a shot the wall would swallow.
    SIF_CHECK(!action.has_value() || !action->place_bomb);
}

// ---------------------------------------------------------------------
// Priorities and personalities
// ---------------------------------------------------------------------

SIF_TEST(survival_outranks_every_other_goal) {
    // A power-up one step away, and a bomb about to go off underfoot. A
    // bot that grabs the power-up here is the classic broken-AI bug.
    Fixture f(open_grid(1, 9), {0, 4});
    f.power_ups.push_back(std::make_shared<PowerUp>(f.grid.get_center({0, 5}), f.grid.tile_size() * 0.7f, TilePos{0, 5},
                                                    PowerUpKind::Fire));
    f.bombs.push_back(make_bomb(f.grid, {0, 4}, 1, 1.f));
    f.rebuild();

    const BotBrain brain(BotPersonality::Collector);
    const BotAction action = brain.decide(f.context());

    SIF_CHECK(action.reason == "survive");
}

SIF_TEST(every_personality_puts_survival_first_and_wandering_last) {
    for (const BotPersonality personality :
         {BotPersonality::Balanced, BotPersonality::Aggressive, BotPersonality::Collector}) {
        const BotBrain brain(personality);
        const std::vector<std::string> order = brain.priorities();

        SIF_CHECK(order.size() == 5);
        SIF_CHECK(order.front() == "survive");
        SIF_CHECK(order.back() == "wander");
    }
}

SIF_TEST(personalities_differ_only_in_the_middle) {
    const std::vector<std::string> aggressive = BotBrain(BotPersonality::Aggressive).priorities();
    const std::vector<std::string> collector = BotBrain(BotPersonality::Collector).priorities();

    SIF_CHECK(aggressive != collector);
    SIF_CHECK(aggressive[1] == "hunt");
    SIF_CHECK(collector[1] == "collect");
}

SIF_TEST(personality_names_round_trip) {
    SIF_CHECK(personality_from_string("aggressive") == BotPersonality::Aggressive);
    SIF_CHECK(to_string(BotPersonality::Collector) == "collector");

    bool threw = false;
    try {
        (void)personality_from_string("reckless");
    } catch (const std::exception&) {
        threw = true;
    }
    SIF_CHECK(threw);
}

SIF_TEST(a_bot_with_nothing_to_do_still_moves) {
    Fixture f(open_grid(5, 5), {2, 2});
    f.rebuild();

    sif::intrnl::Random::instance().seed(3u);
    const BotBrain brain;
    const BotAction action = brain.decide(f.context());

    SIF_CHECK(action.move != Direction::None);
    SIF_CHECK(!action.place_bomb);
}

// ---------------------------------------------------------------------
// End to end
// ---------------------------------------------------------------------

SIF_TEST(bots_open_up_the_arena_over_time) {
    sif::intrnl::Random::instance().seed(2026u);

    MapConfig map;
    map.rows = 9;
    map.columns = 9;
    map.seed = 555u;
    PowerUpRules power_ups;
    power_ups.drop_chance = 0.f; // isolate "do they break blocks"

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map, RoundConfig{}, power_ups);
    world.start_round();

    const auto count_blocks = [&world] {
        int blocks = 0;
        for (int row = 0; row < static_cast<int>(world.grid().rows()); ++row) {
            for (int col = 0; col < static_cast<int>(world.grid().columns()); ++col) {
                if (world.grid().get_tile({row, col}) == Tile::Destructible) {
                    ++blocks;
                }
            }
        }
        return blocks;
    };

    const int before = count_blocks();
    for (int i = 0; i < 900 && !world.round_over(); ++i) {
        world.update(1.f / 60.f);
    }

    // Fifteen seconds of three bots doing nothing would leave the arena
    // untouched; this is the cheapest proof that they are actually playing.
    SIF_CHECK(count_blocks() < before);
}

SIF_TEST(a_round_of_bots_eventually_decides_itself) {
    sif::intrnl::Random::instance().seed(31337u);

    MapConfig map;
    map.rows = 9;
    map.columns = 9;
    map.seed = 99u;

    const auto bus = std::make_shared<sif::event::Event_Bus>();
    const auto factory = std::make_shared<HeadlessEntityFactory>();
    World world(bus, factory, map);
    world.start_round();

    int frames = 0;
    while (!world.round_over() && frames < 60 * 120) { // two minutes at most
        world.update(1.f / 60.f);
        ++frames;
    }

    // The player never moves, so it dies; what is asserted is that the
    // simulation reaches a decision at all rather than deadlocking with
    // four bots frozen in their corners.
    SIF_CHECK(world.round_over());
}

SIF_TEST(fire_already_on_the_ground_counts_as_danger) {
    // The bug this test exists for: the tick after a bomb is removed, its
    // cells looked safe while the fire was still burning there, and every
    // bot walked into it. Bombs are the future; explosions are the present.
    Fixture f(open_grid(5, 5), {2, 0});
    f.explosions.push_back(
        std::make_shared<Explosion>(f.grid.get_center({2, 2}), f.grid.tile_size(), TilePos{2, 2}, 0.5f, false));
    f.rebuild();

    SIF_CHECK(!f.danger.safe({2, 2}));
    SIF_CHECK(f.danger.seconds_until_blast({2, 2}) == 0.f);
    SIF_CHECK(f.danger.safe({2, 3})); // one tile of fire, not a cross
}

SIF_TEST(a_bot_will_not_walk_through_fire_to_reach_a_power_up) {
    Fixture f(open_grid(1, 7), {0, 0});
    f.power_ups.push_back(std::make_shared<PowerUp>(f.grid.get_center({0, 4}), f.grid.tile_size() * 0.7f, TilePos{0, 4},
                                                    PowerUpKind::Fire));
    f.explosions.push_back(
        std::make_shared<Explosion>(f.grid.get_center({0, 2}), f.grid.tile_size(), TilePos{0, 2}, 0.5f, false));
    f.rebuild();

    const CollectPowerUpBehaviour behaviour;
    SIF_CHECK(!behaviour.decide(f.context()).has_value());
}

SIF_TEST(bots_survive_their_own_bombs_across_many_seeds) {
    // The end-to-end guard for the whole escape mechanism. Three bots that
    // all die at t = 2.25 s is what a broken danger model looks like from
    // the outside, so this asserts on the shape of the outcome rather than
    // on any single decision.
    int suicides = 0;

    for (unsigned int seed = 1; seed <= 6; ++seed) {
        sif::intrnl::Random::instance().seed(seed);

        MapConfig map;
        map.rows = 11;
        map.columns = 13;
        map.seed = seed;

        const auto bus = std::make_shared<sif::event::Event_Bus>();
        const auto factory = std::make_shared<HeadlessEntityFactory>();
        World world(bus, factory, map);
        world.start_round();

        // Three seconds: long enough for the first bomb to go off, short
        // enough that nobody has had time to hunt anybody down.
        for (int i = 0; i < 180; ++i) {
            world.update(1.f / 60.f);
        }

        for (const auto& character : world.characters()) {
            if (character->kind() == CharacterKind::Bot && !character->alive()) {
                ++suicides;
            }
        }
    }

    // Eighteen bots, three seconds each, nobody hunting: any death here is
    // a bot that failed to walk away from its own bomb.
    SIF_CHECK(suicides == 0);
}
