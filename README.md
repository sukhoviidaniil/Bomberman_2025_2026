# Bomberman — Advanced Programming 2025-2026

**Author:** Daniil Sukhovii
**Student number:** s0240228

A Bomberman battle-mode game in C++20, built on top of the [sif](cmake/GetSif.cmake) engine
(asset system, UI layout engine, render pipeline, audio interface, event bus), which is
fetched automatically at configure time.

This repository is the **starting state** of the project: the build system, the coordinate
system, the entity model, the event/observer plumbing, the factories, the state machine and
the test suite are in place and working. The gameplay pieces that are still missing are
marked with `TODO(daniil)` in the code and listed in [TODO.md](TODO.md).

---

## Layout

```
logic/          bomberman_logic — the game rules. Links sif only; no SFML anywhere.
  Direction, TileGrid/TilePos/Tile, AABB, Entity/Actor/Character/Bomb/Explosion/PowerUp,
  World (the entity controller), Score, ScoreBoard, IEntityFactory, ai/PathFinder

view/           bomberman_view — the representation. Everything that knows what things look like.
  EntityView + the four concrete views, SFMLEntityFactory (the concrete abstract factory),
  ArenaView, Game, state/ (State, StateManager, Menu/Level/Paused/GameOver)

app/            main.cpp and the asset-registry build step
assets/         font, asset descriptors, generated registry
test/           bomberman_tests — logic tests, no window required
cmake/          GetSif.cmake, GetSFML.cmake
```

The split is the one the assignment requires: `bomberman_logic` links `sif_lib` and nothing
else, so "the logic library compiles without SFML installed" is enforced by the linker, not
by good intentions. CI has a job that proves it on a machine with no SFML package.

---

## Building

Requirements: CMake ≥ 3.20, a C++20 compiler, and network access on the first configure
(sif and nlohmann/json are downloaded). SFML itself does not have to be prepared or passed
in — see below.

Verified on the reference platform: **Ubuntu 24.04, GCC 13, SFML 2.6.1**.

### Two independent SFML searches, on purpose

This project's actual gameplay code (`logic/`, `view/`) never includes an `<SFML/...>`
header — it draws entirely through `sif_sfml`, the reference backend sif ships, which
**sif finds SFML for itself**. Nothing here has to locate, build, or hand SFML to sif; it
is not this project's concern, and passing it one is exactly what could break sif's own
demo or asset tools for someone else who fetches sif on its own (see below).

`cmake/GetSFML.cmake` in *this* repository is a second, entirely separate search — this
project's own, for whatever direct SFML use it might have of its own one day. It is
deliberately included from `view/CMakeLists.txt`, a sibling of where sif is fetched, never
from the root `CMakeLists.txt`: a CMake scope that is an *ancestor* of the sif fetch can
leak its own SFML choice into sif's internal search (CMake target visibility flows
downward to descendant scopes), while a sibling scope cannot. That is what makes it
possible, in principle, for this project to target **SFML 3.x** for its own code while
`sif_sfml` keeps using 2.6.x internally, in the very same build, with neither one aware of
the other. (Today this project has no direct SFML usage of its own, so the search runs and
succeeds but nothing links its result yet — the point is that the capability, and the
isolation, are both there and verified, not that anything currently depends on it.)

> **`sif_sfml` needs SFML 2.6.x specifically** — its C++ source uses the 2.6 API directly
> (`sf::Event.type`, `sf::Rect.left/top/width/height`, ...), which SFML 3 changed
> incompatibly. sif's own `cmake/GetSFML.cmake` rejects SFML 3 for exactly this reason, and
> does so using only its own, sif-prefixed inputs (`SIF_SFML_DIR`, not the generic
> `SFML_DIR`) so this project's *own* SFML choice - whatever it is - can never be mistaken
> for sif's. If you need to point sif at a specific SFML 2.6.x install:
> ```bash
> cmake -S . -B build -DSIF_SFML_DIR=/path/to/SFML-2.6.1/lib/cmake/SFML
> ```
> With no SFML 2.6.x found at all, sif fetches and builds it itself
> (`-DSIF_FETCH_SFML=OFF` to disable).
>
> This project's *own* search (`cmake/GetSFML.cmake`, unrelated to sif's) uses the
> conventional `SFML_DIR`/`SFML_ROOT` and `-DBOMBERMAN_FETCH_SFML=OFF`, since it is not
> shared with anything else.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/app/bomberman            # data dir defaults to ../assets/
./build/test/bomberman_tests
```

Working on the engine at the same time:

```bash
cmake -S . -B build -DSIF_SOURCE_DIR=/path/to/sif
```

Source files are **listed** in the `CMakeLists.txt` files rather than globbed.
A glob is evaluated at configure time, so adding a file leaves an existing build
directory unaware of it until CMake happens to re-run — and the failure that
follows is a pile of undefined references in an unrelated target, pointing
nowhere near the cause. If a build ever does look stale, reconfigure from
scratch (CLion: *Reset Cache and Reload Project*).

Warnings are on everywhere (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`)
and this project's own code produces none. One warning does repeat once per translation unit —
`sif::math::Point2::Point2()` is declared `constexpr` in a header but defined in a `.cpp`, so no
consumer can ever see its definition. It is an upstream defect, tracked as item 15 in
[TODO.md](TODO.md).

---

## Configuration

Everything tunable lives in `assets/config.json` — no rebuild required:

```jsonc
{
  "random_seed": 12345,          // pins the shared RNG: the whole session replays
  "map": {
    "rows": 11, "columns": 13,
    "destructible_chance": 0.75,
    "power_up_chance": 0.25,
    "seed": 20260806,            // pins only the arena
    "layout": null               // or an explicit character matrix (see below)
  },
  "round":  { "character_speed": 0.45, "bomb_fuse_seconds": 2.0, "bot_count": 3 },
  "score":  { "per_enemy_killed": 200, "win_bonus": 1000 },
  "window": { "width": 960, "height": 720, "fps": 60 },
  "audio":  { "enabled": true, "master_volume": 0.7 }
}
```

Every key is optional; anything absent keeps the built-in default, so a file that
only sets a seed is valid. Anything *present but wrong* — a ragged layout, a
negative speed, a probability above 1 — is reported by name at start-up instead
of turning into strange behaviour later.

**Three ways to get an arena**, in priority order:

1. `map.layout` — an explicit character matrix, used verbatim:

   ```
   #  indestructible wall      +  destructible block (* also works)
   .  free floor (or a space)  1-4  spawn cells, in digit order
   ```

   `assets/config.handmade.json` is a complete example. Run it with:

   ```bash
   ./build/app/bomberman assets/ assets/config.handmade.json
   ```

2. `map.seed` — procedural, but reproducible: the same seed and size always
   produce the same arena, whatever happened before it.
3. neither — a different arena every run.

`random_seed` and `map.seed` are separate on purpose. The first makes an entire
session deterministic (arena, power-up drops, every later draw); the second pins
only the map, which is what you want when reproducing *"this map plays badly"*
rather than *"this exact run crashed"*.

## Controls

| Key | Action |
|-----|--------|
| Arrows / WASD | Move |
| Space | Place a bomb |
| Up / Down | Move through a menu |
| Enter | Confirm |
| Esc | Pause (in a level), back (in settings), discard (on the save screen), quit (in the menu) |
| S (while paused) | Give up and return to the menu |

## Screens

```
Menu ──PLAY──────► Level ──round ends──► Save score ──SAVE / DISCARD──► Menu
  │                  │                        │
  │                  └──Esc──► Paused         └──CHANGE NAME──► Settings ──► back
  └──SETTINGS──► Settings ──► back
```

Every screen is a `*.ui.xml` file in `assets/scenes/`, loaded through sif's
layout engine. Settings is reachable from two places and simply pops itself when
done, so whoever pushed it comes back with the new name already applied — that
is the whole reason the state machine is a stack rather than a set of
transitions.

The score is **not** recorded automatically. A player who had a bad round should
not have to delete it afterwards, and one who mistyped their name should be able
to fix it before it is written rather than after, so the save screen offers
*save*, *change name* and *discard*, and tells you in advance whether the score
would make the top five.

---

## Design notes

**Normalized world.** The arena maps itself into `[-1, 1]` on both axes and `sif::rnd::Camera`
projects that to pixels with a `Fit` aspect policy, so tiles stay square in any window and no
speed constant is secretly tied to a tile size. Resizing the window re-projects; it does not
stretch.

**Abstract Factory.** `logic::IEntityFactory` lives in the logic library and declares
`make_character/make_bomb/make_explosion/make_power_up`. `view::SFMLEntityFactory` implements
it by building the model, building the matching view, subscribing that view to the model's
event bus, and handing the model back. The `World` never learns that views exist, which is
what makes a second front-end a matter of writing one more class. `HeadlessEntityFactory`
does the same without views and is what the tests run against.

**Observer, twice.** Each entity owns an `Event_Bus`; its view subscribes to it and reacts to
state changes (direction, death, a fuse going critical) — those are exactly the inputs an
animation needs. The `World` publishes gameplay events (`BlockDestroyed`, `PowerUpTaken`,
`CharacterKilled`, `RoundEnded`, `Tick`) on a second bus, and `Score` is an observer of it.
Nothing in the rules mentions `Score`, so the scoring formula can change on its own.

Positions are read from the model each frame rather than pushed: they change every tick
anyway, so an event per entity per frame would carry no extra information.

**State stack.** Pausing pushes `PausedState` on top of the level, so the stack really holds
`Menu → Level → Paused` and resuming is a single pop with the level untouched. Transitions
are queued and applied at the end of the frame, so a state can push its successor from inside
its own `update()` without being destroyed mid-call.

**UI from data.** Every screen is a `*.ui.xml` scene measured and laid out by
sif's layout engine, not a list of hand-tuned pixel offsets. Labels used to be
placed with an approximate centring formula, which meant every layout change was
a rebuild and every label was centred by eye; the engine measures text with the
real font metrics, so the same scene is correct at any window size. Scenes are
authored with `asset_name="UI"` and rewritten to numeric GUIDs by
`Asset_Reference_Serialization` at build time, so renaming an asset fails the
build instead of silently breaking a screen.

**Bot AI.** A priority chain, not a state machine. The assignment describes the
AI as a list of conditions — *if a bomb is going to blow them up…*, *if any
power-ups are in their range…* — which is a priority of goals re-evaluated every
decision. Each goal is a `BotBehaviour`; `BotBrain` asks them in order and takes
the first answer. A *personality* is therefore just a different order of the
middle three (survival is always first, wandering always last), which is the
listed bonus for no new code — configure it with `round.bot_personalities`.

Everything the behaviours ask is a question about one shared `DangerMap`:
where the fire will be, and in how long. It is rebuilt once per tick from the
bombs **and** the explosions already burning, and `with_bomb()` produces the
hypothetical version that answers the most important question a Bomberman AI
has: *if I drop a bomb here, can I still get out?* A bot reads its own
`blast_radius()` and `bomb_budget()` when answering, so picking up Fire
automatically makes it flee further and take longer shots.

**Power-ups.** Destroying a block rolls `power_ups.drop_chance`; if it hits, the
kind is drawn from three weights, so setting one to `0` removes that power-up
from the game without touching code. Every effect is permanent and capped
(`max_blast_radius`, `max_bomb_budget`, `max_speed`) — an uncapped blast
eventually covers the arena and an uncapped speed outruns the grid snapping.

A pick-up is revealed *by* a blast, in the cell that blast is burning, so it
carries a shield for exactly the explosion's lifetime: it appears once the fire
clears. Without it every pick-up was destroyed on the frame it was created, and
none ever reached a player — a bug a test found rather than an eye.

Bots need no special case to "understand" power-ups: `CollectPowerUpBehaviour`
walks to nearby ones, and every other behaviour reads the character's own
`blast_radius()`, `bomb_budget()` and speed, so a bot that picks up Fire starts
fleeing further and taking longer shots by itself.

**Collision.** Plain axis-aligned boxes, as the assignment permits. `AABB` stores a centre and
a *half*-size, and that is the only meaning it has anywhere.

---

## Assets

Assets come in three flavours, and only one of them needs any processing.

**Used as-is.** Tiles, power-up icons and sound effects are single files that
the engine can load directly, so their descriptors are committed under
`assets/descriptors/` and name the file inside the vendored pack:

```json
{ "type": "Sound", "asset_name": "sfx_explosion",
  "source": "sfx/SFX- The Ultimate 2017 8 bit sound Mini pack/Explosion1/Wav/Explosion1__003.wav" }
```

The `*.asset.json` *is* the indirection — copying the file next to it would
duplicate bytes to say something the JSON already says.

**Packed.** The art pack ships one PNG per animation frame, while sif addresses
frames as rectangles inside a single texture. `sif_sprite_packer` — a tool that
ships with the engine — builds both the strip and its descriptor from
`assets/sprites.pack.json`, so the pixel rectangles cannot drift out of step
with the image:

```bash
sif_sprite_packer assets/ assets/sprites.pack.json
```

**The registry** is then written by `Asset_GUID_Assignment`, also from sif,
which scans every `*.asset.json`:

```bash
Asset_GUID_Assignment assets/ assets/bin/registry.rgst.json
```

The build runs both automatically (target `bomberman_assets`), and neither
output is committed: `assets/graphics/sprites/` and `assets/bin/` are
reproducible from their inputs, and both tools are deterministic — fixed GUID
base, fixed frame order — so a clean checkout regenerates them byte for byte.

Sources and licences: `assets/ATTRIBUTION.md`.

## Asset pipeline

Asset descriptors (`*.asset.json`) sit next to the data they describe. The registry is
**generated at build time** by sif's own tool rather than committed, so it cannot drift:

```bash
Asset_GUID_Assignment assets/ assets/bin/registry.rgst.json
```

(the `bomberman_assets` target does this automatically).

---

## Asset lifetimes

`sif::asset::AssetHandle` offers two accessors and the difference matters:
`get()` returns a non-owning pointer that is only valid for the expression that
produced it, while `lock()` returns an owning `shared_ptr`. Anything that keeps
the pointer beyond one statement — the renderer while it draws, a voice while it
plays, a view reading frame rectangles — uses `lock()`.

## Asset loading and threads

Loads run on a small pool of background threads by default, but a loader can
declare `runs_on_main_thread()`; those are queued and executed by
`AssetRegistry::pump()`, which the frame loop calls once per frame.

Every loader in the SFML backend does so, and not as a precaution.
`sf::Texture` and `sf::Font` need an OpenGL context and take SFML's
`TransientContextLock`, which races with the main thread's rendering;
`sf::SoundBuffer` goes through OpenAL, which races with its own mixer thread.
ThreadSanitizer catches both inside the libraries. What a player sees is heap
corruption — *double free or corruption*, *malloc(): unaligned tcache chunk
detected*, or a segfault whose stack has nothing to do with assets — and
pressing a key during start-up is enough to trigger it, because that plays a
sound while the rest of the assets are still decoding.

The headless backend keeps the parallelism: `sf::Image` and
`sf::InputSoundFile` are pure CPU decoding and open no device.

## Tests

```bash
./build/test/bomberman_tests
```

21 cases covering the normalized coordinate mapping, arena generation (pillar lattice, escapable
spawns), `Score` reacting to events only, power-up effects and bomb budgeting, a full round in
the `World` (fuse → blast → block destruction → round end), and the path finder including the
"no path" and "nearest safe cell" cases.

Two real bugs were found by these tests while writing them: sixty frames of `1/60 s` sum to
`0.99999994f` (one lost second of score per minute), and the state stack was still empty when
the first frame checked it. Both are fixed, with the reason recorded in the code.

---

## Attribution

`assets/graphics/font/DejaVuSans.ttf` — DejaVu Fonts License, see
`assets/LICENSE-DejaVuSans.txt`.

The sprite sheet linked from the assignment has not been added yet; see `TODO.md`. No
third-party commercial art is committed to this repository.
