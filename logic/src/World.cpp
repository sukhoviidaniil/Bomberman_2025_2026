/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2025-12-16
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/

#include "bomberman/logic/World.h"

#include <algorithm>
#include <utility>

#include "bomberman/logic/events/GameEvents.h"

#include "sif/internal/Random.h"

namespace bomberman::logic {

World::World(std::shared_ptr<sif::event::Event_Bus> bus, std::shared_ptr<IEntityFactory> factory, MapConfig map,
             RoundConfig round, PowerUpRules power_ups)
    : bus_(std::move(bus)), factory_(std::move(factory)), map_(std::move(map)), round_(round), power_ups_(power_ups) {

    if (bus_ == nullptr || factory_ == nullptr) {
        throw std::invalid_argument("World: an event bus and an entity factory are required");
    }
}

void World::start_round() {
    characters_.clear();
    bots_.clear();
    bombs_.clear();
    explosions_.clear();
    power_ups_entities_.clear();
    player_.reset();
    round_over_ = false;
    player_won_ = false;

    // Three ways to get an arena, in priority order: an explicit
    // layout is used verbatim, a seed makes generation reproducible,
    // and neither means a fresh arena every run.
    if (!map_.layout.empty()) {
        grid_ = TileGrid::from_layout(map_.layout);
    } else {
        grid_ = TileGrid(map_.rows, map_.columns);
        if (map_.seed.has_value()) {
            grid_.generate_arena(map_.destructible_chance, *map_.seed);
        } else {
            grid_.generate_arena(map_.destructible_chance);
        }
    }

    spawn_characters();
}

void World::spawn_characters() {
    const std::vector<TilePos> spawns = grid_.spawn_cells();
    const float size = grid_.tile_size() * round_.character_size;

    // The player takes the top-left corner, as the assignment
    // specifies; the bots fill the remaining three.
    player_ =
        factory_->make_character(CharacterKind::Player, grid_.get_center(spawns[0]), size, round_.character_speed);
    characters_.push_back(player_);
    teach_obstacles(player_);

    // A hand-written layout may declare fewer spawns than there are
    // bots; spawning two characters on one cell would kill them both
    // on the first bomb, so the count follows the map.
    const std::size_t bots = spawns.empty() ? 0 : std::min(round_.bot_count, spawns.size() - 1);
    for (std::size_t i = 0; i < bots; ++i) {
        std::shared_ptr<Character> bot =
            factory_->make_character(CharacterKind::Bot, grid_.get_center(spawns[i + 1]), size, round_.character_speed);
        characters_.push_back(bot);
        teach_obstacles(bot);

        // Personalities cycle through whatever the configuration
        // listed, so three bots with two personalities configured get
        // the first one twice rather than none at all.
        const ai::BotPersonality personality = round_.bot_personalities.empty()
                                                   ? ai::BotPersonality::Balanced
                                                   : round_.bot_personalities[i % round_.bot_personalities.size()];

        bots_.push_back(BotSlot{bot, ai::BotBrain(personality), TilePos{-1, -1}, 0.f});
    }
}

void World::update(const float dt) {
    if (round_over_) {
        return;
    }

    bus_->emit(game_events::Tick{dt});

    // The danger map is rebuilt once per tick and shared by every bot:
    // it is the same question for all of them, and deriving blast rays
    // three times would be three times the work for one answer.
    danger_.rebuild(grid_, bombs_, explosions_);
    update_bots(dt);

    for (const auto& character : characters_) {
        if (character->alive()) {
            character->move(dt, grid_);
        }
    }

    update_bomb_passability();

    for (const auto& bomb : bombs_) {
        bomb->update(dt);
    }
    for (const auto& explosion : explosions_) {
        explosion->update(dt);
    }
    for (const auto& power_up : power_ups_entities_) {
        power_up->update(dt);
    }

    resolve_detonations();
    resolve_collisions();
    remove_expired();
    check_round_over();
}

void World::set_player_direction(const Direction direction) {
    if (player_ != nullptr && player_->alive()) {
        player_->set_direction(direction);
    }
}

void World::player_place_bomb() {
    if (player_ != nullptr) {
        place_bomb_for(player_);
    }
}

void World::teach_obstacles(const std::shared_ptr<Character>& character) const {
    // A bomb blocks movement, except for the character still standing
    // on the one they just placed - "After moving out of the bomb, the
    // player can no longer go through it". The rule lives in the World
    // because only the World knows where the bombs are; the character
    // only has to be told how to ask.
    //
    // Capturing `this` is safe: the World owns every character and
    // outlives them all.
    character->set_obstacle_check([this](const TilePos& cell) { return has_bomb_at(cell); });
    character->set_power_up_rules(power_ups_);
}

void World::update_bots(const float dt) {
    // How often a bot re-decides when nothing else prompts it.
    constexpr float decision_interval = 0.15f;

    for (BotSlot& slot : bots_) {
        const std::shared_ptr<Character>& bot = slot.character;
        if (bot == nullptr || !bot->alive()) {
            continue;
        }

        const auto cell = grid_.get_TilePos(bot->position());
        if (!cell.has_value()) {
            continue;
        }

        slot.seconds_since_decision += dt;

        const bool entered_new_cell = *cell != slot.last_cell;
        const bool in_danger = !danger_.safe(*cell);
        const bool timer_expired = slot.seconds_since_decision >= decision_interval;

        if (!entered_new_cell && !in_danger && !timer_expired) {
            continue;
        }

        slot.last_cell = *cell;
        slot.seconds_since_decision = 0.f;

        const ai::BotContext ctx{grid_, danger_, *bot, *cell, characters_, power_ups_entities_,
                                 [this](const TilePos& c) { return has_bomb_at(c); }, round_.bomb_fuse_seconds,
                                 // Tiles per second: the bot reasons in cells, the world
                                 // moves in world units, and the tile size is the bridge.
                                 grid_.tile_size() > 0.f ? bot->speed() / grid_.tile_size() : 0.f};

        const ai::BotAction action = slot.brain.decide(ctx);

        if (action.place_bomb) {
            place_bomb_for(bot);
        }
        if (action.move != Direction::None) {
            bot->set_direction(action.move);
        }
    }
}

void World::place_bomb_for(const std::shared_ptr<Character>& character) {
    if (character == nullptr || !character->can_place_bomb()) {
        return;
    }

    const auto cell = grid_.get_TilePos(character->position());
    if (!cell.has_value() || has_bomb_at(*cell)) {
        return;
    }

    bombs_.push_back(factory_->make_bomb(grid_.get_center(*cell), grid_.tile_size(), *cell, character,
                                         character->blast_radius(), round_.bomb_fuse_seconds));

    character->on_bomb_placed();
    bus_->emit(game_events::BombPlaced{*cell, character == player_});
    // The character is standing on it, so it stays passable for them
    // until they step off - "After moving out of the bomb, the player
    // can no longer go through it".
    character->allow_leaving(*cell);
}

bool World::has_bomb_at(const TilePos& cell) const {
    return std::any_of(bombs_.begin(), bombs_.end(),
                       [&cell](const std::shared_ptr<Bomb>& b) { return b->cell() == cell; });
}

void World::update_bomb_passability() {
    for (const auto& character : characters_) {
        const auto cell = grid_.get_TilePos(character->position());
        if (!cell.has_value() || !character->may_pass(*cell)) {
            character->forget_leaving();
        }
    }
}

void World::resolve_detonations() {
    // A loop, not recursion: a blast can set off further bombs, whose
    // blasts can set off more. Iterating until nothing new detonates
    // resolves the whole chain inside one frame, which is what the
    // assignment asks for ("Explosions can trigger other bombs in
    // their radius, causing a chain reaction").
    bool progressed = true;
    while (progressed) {
        progressed = false;

        for (const auto& bomb : bombs_) {
            if (!bomb->detonated() || bomb->expired()) {
                continue;
            }

            bus_->emit(game_events::BombExploded{bomb->cell()});
            spread_blast(*bomb);
            bomb->expire();

            if (const auto owner = bomb->owner().lock()) {
                owner->on_bomb_exploded();
            }
            progressed = true;
        }
    }
}

void World::spread_blast(const Bomb& bomb) {
    const bool from_player = player_ != nullptr && bomb.owner().lock() == player_;

    const float tile = grid_.tile_size();

    const auto ignite = [&](const TilePos& cell) {
        explosions_.push_back(
            factory_->make_explosion(grid_.get_center(cell), tile, cell, round_.explosion_seconds, from_player));

        // Any bomb caught by the fire goes off too; detonate() is
        // idempotent, so overlapping blasts are harmless.
        for (const auto& other : bombs_) {
            if (other->cell() == cell) {
                other->detonate();
            }
        }
    };

    ignite(bomb.cell());

    for (std::size_t i = 0; i < direction_count; ++i) {
        const Direction dir = by_index(i);

        for (unsigned int step = 1; step <= bomb.radius(); ++step) {
            const TilePos cell{bomb.cell().row + static_cast<int>(to_vector(dir).y) * static_cast<int>(step),
                               bomb.cell().col + static_cast<int>(to_vector(dir).x) * static_cast<int>(step)};

            const Tile tile_kind = grid_.get_tile(cell);

            if (tile_kind == Tile::Indestructible) {
                break; // the blast stops dead
            }

            ignite(cell);

            if (tile_kind == Tile::Destructible) {
                grid_.set_tile(cell, Tile::Free);
                bus_->emit(game_events::BlockDestroyed{cell, from_player});

                // A destroyed block may reveal a pick-up. It is shielded
                // for as long as this blast burns: it appears in the very
                // cell that is on fire, and without the shield the next
                // line of resolve_collisions would destroy it on the frame
                // it was created - which is exactly what used to happen,
                // so no power-up ever reached the player.
                if (sif::intrnl::rand_chance(power_ups_.drop_chance)) {
                    const PowerUpKind kind = power_ups_.roll_kind();
                    power_ups_entities_.push_back(factory_->make_power_up(grid_.get_center(cell), tile * 0.7f, cell,
                                                                          kind, round_.explosion_seconds));
                }

                break; // "only through one destructible block at a time"
            }
        }
    }
}

void World::resolve_collisions() {
    for (const auto& explosion : explosions_) {
        // An explosion that finished this frame has stopped burning;
        // letting it still destroy things gives it one extra frame of
        // reach, which is exactly long enough to eat a power-up whose
        // reveal shield ran out on the very same tick.
        if (explosion->expired()) {
            continue;
        }

        // Characters caught in the fire.
        for (const auto& character : characters_) {
            if (!character->alive() || !intersects(character->box(), explosion->box(), 0.f)) {
                continue;
            }

            character->kill();
            bus_->emit(
                game_events::CharacterKilled{character->kind(), explosion->from_player() && character != player_});
        }

        // Pick-ups burn as well.
        for (const auto& power_up : power_ups_entities_) {
            if (!power_up->expired() && !power_up->shielded() && power_up->cell() == explosion->cell()) {
                power_up->expire();
            }
        }
    }

    // Characters walking over pick-ups.
    for (const auto& power_up : power_ups_entities_) {
        if (power_up->expired()) {
            continue;
        }
        for (const auto& character : characters_) {
            if (!character->alive() || !intersects(character->box(), power_up->box(), 0.f)) {
                continue;
            }

            character->apply(power_up->kind());
            power_up->expire();
            bus_->emit(game_events::PowerUpTaken{power_up->kind(), character == player_});
            break;
        }
    }
}

void World::remove_expired() {
    const auto drop = [](auto& container) { std::erase_if(container, [](const auto& e) { return e->expired(); }); };
    drop(bombs_);
    drop(explosions_);
    drop(power_ups_entities_);
    // Dead characters are kept: their view still has a death
    // animation to play, and the round-over check counts them.
}

void World::check_round_over() {
    if (round_over_) {
        return;
    }

    const bool player_alive = player_ != nullptr && player_->alive();
    const auto living_bots =
        std::count_if(characters_.begin(), characters_.end(), [](const std::shared_ptr<Character>& c) {
            return c->alive() && c->kind() == CharacterKind::Bot;
        });

    if (player_alive && living_bots == 0) {
        round_over_ = true;
        player_won_ = true;
    } else if (!player_alive) {
        round_over_ = true;
        player_won_ = false;
    }

    if (round_over_) {
        bus_->emit(game_events::RoundEnded{player_won_});
    }
}

const TileGrid& World::grid() const { return grid_; }
const std::shared_ptr<Character>& World::player() const { return player_; }
const std::vector<std::shared_ptr<Character>>& World::characters() const { return characters_; }
const std::vector<std::shared_ptr<Bomb>>& World::bombs() const { return bombs_; }
const std::vector<std::shared_ptr<Explosion>>& World::explosions() const { return explosions_; }
const std::vector<std::shared_ptr<PowerUp>>& World::power_ups() const { return power_ups_entities_; }
bool World::round_over() const { return round_over_; }
bool World::player_won() const { return player_won_; }
} // namespace bomberman::logic
