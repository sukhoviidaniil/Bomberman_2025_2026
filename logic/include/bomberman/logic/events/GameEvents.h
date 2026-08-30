/***************************************************************
 * Author:        Sukhovii Daniil
 * Email:       sukhovii.daniil@gmail.com
 * Created:       2026-08-05
 *
 * Disclaimer:
 *   This file is part of Bomberman.
 *   Unauthorized use, reproduction, or distribution is prohibited.
 ***************************************************************/
#ifndef BOMBERMAN_LOGIC_GAMEEVENTS_H
#define BOMBERMAN_LOGIC_GAMEEVENTS_H

#include <cstdint>

#include "bomberman/logic/Direction.h"
#include "bomberman/logic/grid/TilePos.h"

#include "sif/event/Event.h"
#include "sif/math/Point2.h"

namespace bomberman::logic {

/// @brief Which power-up a pick-up grants.
enum class PowerUpKind {
    Fire,      ///< +1 blast radius in each direction
    ExtraBomb, ///< +1 simultaneously placeable bomb
    Skates     ///< +movement speed
};

/// @brief Distinguishes the human player from the three bots.
enum class CharacterKind { Player, Bot };
} // namespace bomberman::logic

/**
 * @file
 *
 * The events every subsystem talks through.
 *
 * The assignment asks for the Observer pattern twice over: views redraw
 * when a model changes, and Score updates when something scoreworthy
 * happens - "These same generic events are at the same time used by the
 * Score class". Both are served by the structs below, so nothing has to
 * call Score directly. (In Pac-Man the World called score->coin_collection()
 * by hand, which is why Score and the game rules could not be changed
 * independently.)
 *
 * All of them carry EventMask::Program, sif's "application level"
 * category - the engine's mask enum has Input/Window/System/Program and
 * no game-specific value.
 *
 * TODO(daniil): if the masks ever need to distinguish gameplay from
 *  other application events (for filtering, or for a replay recorder),
 *  add a Game bit to sif::event::EventMask upstream rather than
 *  overloading Program here.
 *
 * Two scopes:
 *   bomberman::logic::entity_events - published on an entity's own bus,
 *       consumed by that entity's view (moved, died, state changed).
 *   bomberman::logic::game_events   - published on the world bus,
 *       consumed by Score, the HUD and the state machine.
 */
namespace bomberman::logic::entity_events {

/// @brief The entity is now somewhere else.
struct Moved {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    sif::math::Point2 position{};
    Direction direction = Direction::None;
};

/// @brief The entity started/stopped moving; views switch animations.
struct MotionChanged {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    bool moving = false;
    Direction direction = Direction::None;
};

/// @brief The entity died; the view plays its death animation.
struct Died {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
};

/// @brief A bomb entered the last second of its fuse.
struct FuseCritical {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
};
} // namespace bomberman::logic::entity_events

namespace bomberman::logic::game_events {

/// @brief One frame of gameplay elapsed. Score uses it for the survival bonus.
struct Tick {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    float dt = 0.f;
};

/// @brief A character just placed a bomb.
struct BombPlaced {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    TilePos cell{};
    bool by_player = false;
};

/// @brief A bomb went off (once per bomb, not once per burning tile).
struct BombExploded {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    TilePos cell{};
};

/// @brief A destructible block was blown up.
struct BlockDestroyed {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    TilePos cell{};
    bool by_player = false; ///< Only the player's own blasts score.
};

/// @brief A power-up was picked up.
struct PowerUpTaken {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    PowerUpKind kind = PowerUpKind::Fire;
    bool by_player = false;
};

/// @brief A character was caught by a blast.
struct CharacterKilled {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    CharacterKind victim = CharacterKind::Bot;
    bool by_player = false; ///< True when the player's bomb did it.
};

/// @brief The round ended.
struct RoundEnded {
    static constexpr sif::event::EventMask mask = sif::event::EventMask::Program;
    bool player_won = false;
};
} // namespace bomberman::logic::game_events

#endif // BOMBERMAN_LOGIC_GAMEEVENTS_H
