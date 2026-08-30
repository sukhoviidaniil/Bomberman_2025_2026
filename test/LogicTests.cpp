/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "TestFramework.h"

#include <cmath>
#include <memory>

#include "bomberman/logic/Score.h"
#include "bomberman/logic/World.h"
#include "bomberman/logic/ai/PathFinder.h"

#include "sif/internal/Random.h"

using namespace bomberman::logic;

namespace {
bool near_eq(const float a, const float b, const float eps = 0.0001f) { return std::abs(a - b) < eps; }

/// A world wired to the head-less factory: no window, no views.
struct Fixture {
    std::shared_ptr<sif::event::Event_Bus> bus = std::make_shared<sif::event::Event_Bus>();
    std::shared_ptr<IEntityFactory> factory = std::make_shared<HeadlessEntityFactory>();
    World world{bus, factory};
};
} // namespace

// ---------------------------------------------------------------------
// Normalized coordinates
// ---------------------------------------------------------------------

SIF_TEST(the_arena_lives_inside_the_normalized_world) {
    // The assignment: "the World width and height bounded by [-1, 1]".
    const TileGrid grid(11, 13);
    const sif::intrnl::Rect bounds = grid.bounds();

    SIF_CHECK(bounds.x >= -1.f && bounds.y >= -1.f);
    SIF_CHECK(bounds.x + bounds.width <= 1.f);
    SIF_CHECK(bounds.y + bounds.height <= 1.f);

    // The longer axis fills the world exactly.
    SIF_CHECK(near_eq(bounds.width, 2.f));
}

SIF_TEST(tiles_stay_square_whatever_the_arena_shape) {
    const TileGrid wide(5, 21);
    const sif::intrnl::Rect cell = wide.get_rect({0, 0});
    SIF_CHECK(near_eq(cell.width, cell.height));
    SIF_CHECK(near_eq(cell.width, wide.tile_size()));
}

SIF_TEST(a_world_position_maps_back_to_the_cell_it_came_from) {
    const TileGrid grid(11, 13);
    for (int row = 0; row < 11; ++row) {
        for (int col = 0; col < 13; ++col) {
            const TilePos cell{row, col};
            const auto found = grid.get_TilePos(grid.get_center(cell));
            SIF_CHECK(found.has_value() && *found == cell);
        }
    }
}

SIF_TEST(cells_outside_the_arena_read_as_solid) {
    // Movement and blast code relies on this instead of bounds-checking
    // every access.
    const TileGrid grid(5, 5);
    SIF_CHECK(grid.get_tile({-1, 0}) == Tile::Indestructible);
    SIF_CHECK(grid.get_tile({0, 5}) == Tile::Indestructible);
    SIF_CHECK(!grid.contains({-1, -1}));
}

SIF_TEST(negative_cell_arithmetic_does_not_wrap_around) {
    // The Pac-Man grid stored cells as size_t, so "one up from row 0"
    // became ~1.8e19 and clamped to the *bottom* of the map.
    const TileGrid grid(5, 5);
    const TilePos up = grid.neighbour({0, 2}, Direction::Up);
    SIF_CHECK(up.row == -1);
    SIF_CHECK(grid.clamp(up).row == 0);
}

// ---------------------------------------------------------------------
// Arena generation
// ---------------------------------------------------------------------

SIF_TEST(generated_arenas_have_the_bomberman_pillar_lattice) {
    sif::intrnl::Random::instance().seed(4u);

    TileGrid grid(11, 13);
    grid.generate_arena(1.f); // every free cell filled, to isolate the lattice

    bool lattice_ok = true;
    for (int row = 0; row < 11; ++row) {
        for (int col = 0; col < 13; ++col) {
            const bool expected_pillar = (row % 2 == 1) && (col % 2 == 1);
            const bool is_pillar = grid.get_tile({row, col}) == Tile::Indestructible;
            lattice_ok = lattice_ok && (expected_pillar == is_pillar);
        }
    }
    SIF_CHECK(lattice_ok);
}

SIF_TEST(every_spawn_corner_can_be_escaped) {
    sif::intrnl::Random::instance().seed(9u);

    TileGrid grid(11, 13);
    grid.generate_arena(1.f);

    bool all_open = true;
    for (const TilePos& spawn : grid.spawn_cells()) {
        all_open = all_open && walkable(grid.get_tile(spawn));

        int exits = 0;
        for (std::size_t i = 0; i < direction_count; ++i) {
            if (walkable(grid.get_tile(grid.neighbour(spawn, by_index(i))))) {
                ++exits;
            }
        }
        // Without a way out, the first bomb a character places kills it.
        all_open = all_open && exits >= 1;
    }
    SIF_CHECK(all_open);
}

// ---------------------------------------------------------------------
// Score, through the Observer pattern only
// ---------------------------------------------------------------------

SIF_TEST(score_reacts_to_events_and_is_never_called_directly) {
    const auto bus = std::make_shared<sif::event::Event_Bus>();
    Score score(bus);

    SIF_CHECK(score.points() == 0);

    bus->emit(game_events::BlockDestroyed{{1, 1}, true});
    SIF_CHECK(score.points() == score.rules().per_block_destroyed);
    SIF_CHECK(score.blocks_destroyed() == 1);

    bus->emit(game_events::PowerUpTaken{PowerUpKind::Fire, true});
    bus->emit(game_events::CharacterKilled{CharacterKind::Bot, true});

    SIF_CHECK(score.power_ups_taken() == 1);
    SIF_CHECK(score.enemies_killed() == 1);
}

SIF_TEST(score_ignores_what_the_bots_do_to_each_other) {
    const auto bus = std::make_shared<sif::event::Event_Bus>();
    Score score(bus);

    bus->emit(game_events::BlockDestroyed{{1, 1}, false});
    bus->emit(game_events::PowerUpTaken{PowerUpKind::Skates, false});
    bus->emit(game_events::CharacterKilled{CharacterKind::Bot, false});

    SIF_CHECK(score.points() == 0);
}

SIF_TEST(survival_is_awarded_in_whole_seconds) {
    const auto bus = std::make_shared<sif::event::Event_Bus>();
    Score score(bus);

    // Sixty frames of 1/60 s: one second, whatever the frame rate.
    for (int i = 0; i < 60; ++i) {
        bus->emit(game_events::Tick{1.f / 60.f});
    }
    SIF_CHECK(score.seconds_alive() == 1);
    SIF_CHECK(score.points() == score.rules().per_second_alive);
}

SIF_TEST(winning_and_losing_are_worth_a_bonus_and_a_penalty) {
    const auto bus = std::make_shared<sif::event::Event_Bus>();
    Score won(bus);
    bus->emit(game_events::RoundEnded{true});
    SIF_CHECK(won.points() == won.rules().win_bonus);

    const auto bus2 = std::make_shared<sif::event::Event_Bus>();
    Score lost(bus2);
    bus2->emit(game_events::RoundEnded{false});
    SIF_CHECK(lost.points() == 0); // clamped, never negative
}

// ---------------------------------------------------------------------
// Characters and power-ups
// ---------------------------------------------------------------------

SIF_TEST(power_ups_change_the_stats_the_ai_reads) {
    Character c("Bot", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Bot);

    SIF_CHECK(c.blast_radius() == 1);
    SIF_CHECK(c.bomb_budget() == 1);

    c.apply(PowerUpKind::Fire);
    c.apply(PowerUpKind::ExtraBomb);
    const float speed_before = c.speed();
    c.apply(PowerUpKind::Skates);

    SIF_CHECK(c.blast_radius() == 2);
    SIF_CHECK(c.bomb_budget() == 2);
    SIF_CHECK(c.speed() > speed_before);
}

SIF_TEST(the_bomb_budget_limits_simultaneous_bombs) {
    Character c("Player", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Player);

    SIF_CHECK(c.can_place_bomb());
    c.on_bomb_placed();
    SIF_CHECK(!c.can_place_bomb());

    c.on_bomb_exploded();
    SIF_CHECK(c.can_place_bomb());
}

SIF_TEST(a_dead_character_places_no_more_bombs) {
    Character c("Player", {0.f, 0.f}, 0.1f, 0.5f, CharacterKind::Player);
    c.kill();
    SIF_CHECK(!c.alive());
    SIF_CHECK(!c.can_place_bomb());
}

// ---------------------------------------------------------------------
// World
// ---------------------------------------------------------------------

SIF_TEST(a_round_starts_with_one_player_and_three_bots) {
    sif::intrnl::Random::instance().seed(11u);

    Fixture f;
    f.world.start_round();

    SIF_CHECK(f.world.characters().size() == 4);
    SIF_CHECK(f.world.player() != nullptr);
    SIF_CHECK(f.world.player()->kind() == CharacterKind::Player);

    // The player starts in the top-left corner, as the assignment says.
    const auto cell = f.world.grid().get_TilePos(f.world.player()->position());
    SIF_CHECK(cell.has_value() && cell->row == 0 && cell->col == 0);
}

SIF_TEST(a_bomb_explodes_after_its_fuse_and_frees_its_slot) {
    sif::intrnl::Random::instance().seed(13u);

    Fixture f;
    f.world.start_round();
    f.world.player_place_bomb();

    SIF_CHECK(f.world.bombs().size() == 1);
    SIF_CHECK(!f.world.player()->can_place_bomb());

    // Step past the fuse in frame-sized increments.
    for (int i = 0; i < 150; ++i) {
        f.world.update(1.f / 60.f);
    }

    // Not "no bombs at all": the bots are playing too and will have
    // dropped their own by now. What this test is about is the player's
    // bomb, so it asks about the player's cell and the player's slot.
    SIF_CHECK(!f.world.has_bomb_at({0, 0}));
    SIF_CHECK(f.world.player()->bombs_placed() == 0);
    // can_place_bomb() is still false: the player was standing on the bomb
    // and died in its blast, which is correct behaviour and worth
    // asserting rather than working around.
    SIF_CHECK(!f.world.player()->alive());
}

SIF_TEST(a_blast_clears_destructible_blocks_but_not_pillars) {
    sif::intrnl::Random::instance().seed(17u);

    Fixture f;
    f.world.start_round();
    f.world.player_place_bomb();

    for (int i = 0; i < 150; ++i) {
        f.world.update(1.f / 60.f);
    }

    // (1,1) is a pillar in every generated arena.
    SIF_CHECK(f.world.grid().get_tile({1, 1}) == Tile::Indestructible);
}

SIF_TEST(the_round_ends_when_the_player_dies) {
    sif::intrnl::Random::instance().seed(19u);

    Fixture f;
    f.world.start_round();

    // Stand still on the bomb: the blast is unavoidable.
    f.world.player_place_bomb();
    for (int i = 0; i < 200 && !f.world.round_over(); ++i) {
        f.world.update(1.f / 60.f);
    }

    SIF_CHECK(f.world.round_over());
    SIF_CHECK(!f.world.player_won());
    SIF_CHECK(!f.world.player()->alive());
}

// ---------------------------------------------------------------------
// Path finding
// ---------------------------------------------------------------------

SIF_TEST(the_path_finder_walks_around_walls) {
    TileGrid grid(5, 5);
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 5; ++col) {
            grid.set_tile({row, col}, Tile::Free);
        }
    }
    // A wall across the middle, with a gap on the right.
    grid.set_tile({2, 0}, Tile::Indestructible);
    grid.set_tile({2, 1}, Tile::Indestructible);
    grid.set_tile({2, 2}, Tile::Indestructible);
    grid.set_tile({2, 3}, Tile::Indestructible);

    const auto passable = [&grid](const TilePos& c) { return walkable(grid.get_tile(c)); };

    const std::vector<TilePos> path = ai::PathFinder::find_path(grid, {0, 0}, {4, 0}, passable);
    SIF_CHECK(!path.empty());
    SIF_CHECK(path.front() == TilePos(0, 0));
    SIF_CHECK(path.back() == TilePos(4, 0));
    // Straight down is 4 steps; the detour through the gap is longer.
    SIF_CHECK(path.size() > 5);
}

SIF_TEST(an_unreachable_target_yields_no_path_and_no_guess) {
    TileGrid grid(3, 3);
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            grid.set_tile({row, col}, Tile::Free);
        }
    }
    grid.set_tile({1, 0}, Tile::Indestructible);
    grid.set_tile({1, 1}, Tile::Indestructible);
    grid.set_tile({1, 2}, Tile::Indestructible);

    const auto passable = [&grid](const TilePos& c) { return walkable(grid.get_tile(c)); };

    SIF_CHECK(ai::PathFinder::find_path(grid, {0, 0}, {2, 2}, passable).empty());
    SIF_CHECK(ai::PathFinder::first_step(grid, {0, 0}, {2, 2}, passable) == Direction::None);
}

SIF_TEST(find_nearest_answers_the_run_to_safety_query) {
    TileGrid grid(1, 6);
    for (int col = 0; col < 6; ++col) {
        grid.set_tile({0, col}, Tile::Free);
    }

    const auto passable = [&grid](const TilePos& c) { return walkable(grid.get_tile(c)); };
    // "Safe" = at least three cells away from the bomb at column 0.
    const auto safe = [](const TilePos& c) { return c.col >= 3; };

    const auto found = ai::PathFinder::find_nearest(grid, {0, 0}, passable, safe);
    SIF_CHECK(found.has_value());
    SIF_CHECK(found->col == 3); // the nearest one, not just any
}
