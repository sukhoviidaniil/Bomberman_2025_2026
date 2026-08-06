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

Requirements: CMake ≥ 3.20, a C++20 compiler, **SFML 2.6.x** (with the `audio`
component), and network access on the first configure (sif and nlohmann/json are
downloaded).

> **SFML 3 will not work.** It is not source-compatible with SFML 2 — `sf::Event` became a
> variant, `sf::Rect` swapped `left/top/width/height` for `position/size`, `sf::Text` and
> `sf::Sound` take their resource in the constructor, `pollEvent` returns an `optional`, and
> so on. `cmake/GetSFML.cmake` therefore inspects every SFML installation it can find and
> picks a 2.x one; if both versions are installed it will *not* silently take the newer one.
> If the wrong copy is chosen, point at the right one explicitly:
>
> ```bash
> cmake -S . -B build -DSFML_DIR=/path/to/SFML-2.6.1/lib/cmake/SFML
> ```
>
> With no SFML 2.x installed at all, the build fetches and compiles 2.6.1 itself
> (`-DBOMBERMAN_FETCH_SFML=OFF` to disable).

Verified on the reference platform: **Ubuntu 24.04, GCC 13, SFML 2.6.1**.

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

Warnings are on everywhere (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`)
and this project's own code produces none. One warning does repeat once per translation unit —
`sif::math::Point2::Point2()` is declared `constexpr` in a header but defined in a `.cpp`, so no
consumer can ever see its definition. It is an upstream defect, tracked as item 15 in
[TODO.md](TODO.md).

---

## Controls

| Key | Action |
|-----|--------|
| Arrows / WASD | Move |
| Space | Place a bomb |
| Esc | Pause (in a level) / quit (in the menu) |
| Enter | Play, resume, confirm |
| S (while paused) | Give up and return to the menu |

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

**Collision.** Plain axis-aligned boxes, as the assignment permits. `AABB` stores a centre and
a *half*-size, and that is the only meaning it has anywhere.

---

## Asset pipeline

Asset descriptors (`*.asset.json`) sit next to the data they describe. The registry is
**generated at build time** by sif's own tool rather than committed, so it cannot drift:

```bash
Asset_GUID_Assignment assets/ assets/bin/registry.rgst.json
```

(the `bomberman_assets` target does this automatically).

---

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
