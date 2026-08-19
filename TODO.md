# TODO and migration notes

> The ordered, dated version of this list is **[PLAN.md](PLAN.md)** — read that
> first. This file is the raw inventory.

Every item below also exists as a `TODO(daniil)` comment at the place in the code where the
work has to happen, so CLion's TODO tool window lists them.

---

## 1. Gameplay still to implement

| # | What | Where |
|---|------|-------|
| ~~1~~ | ~~Bot AI~~ — **done**: `logic/ai/` (DangerMap, BotBehaviour + five behaviours, BotBrain, personalities), driven from `World::update_bots`. 26 tests. | |
| ~~21~~ | ~~SFML asset loaders ran on worker threads~~ — **fixed upstream**: OpenGL and OpenAL both race when touched from a background thread while the main thread renders or plays. `IAssetLoader::runs_on_main_thread()` + `AssetRegistry::pump()` route those loads to the frame loop. | |
| ~~19~~ | ~~`AssetDataNode` had no virtual destructor~~ — **fixed upstream**: every parsed node is owned as `unique_ptr<AssetDataNode>` while holding a derived object, so deleting one was undefined behaviour. AddressSanitizer caught the asset tool freeing 200 of 232 bytes; the corrupted allocator then crashed something unrelated later. | |
| ~~20~~ | ~~`AssetHandle::get()` returned a pointer into a temporary~~ — **fixed upstream**: added `lock()`, which returns an owning `shared_ptr`, and every site that keeps the pointer beyond one expression (renderer, audio player, entity views) now uses it. | |
| ~~18~~ | ~~Strip packing belongs in sif~~ — **done**: `sif_sprite_packer` now ships with the engine (`sif/tools/`), is built by CMake and is run by this project's `bomberman_assets` target. The Python script is gone. | |
| ~~2~~ | ~~Sprites and animations~~ — **done**: walk per direction, idle, death, ticking bomb, growing explosion, tile art. Built by `sif_sprite_packer` from `assets/sprites.pack.json`. | |
| ~~3~~ | ~~Sound~~ — **done**: `view::AudioDirector` turns gameplay events into sound. Still missing: menu navigation sound and background music. | `view/src/AudioDirector.cpp` |
| ~~4~~ | ~~HUD as a real UI tree~~ — **done**: all five screens are `*.ui.xml` scenes under `assets/scenes/`, serialized at build time. | |
| 4b | **A `<Stack>` container in sif.** The overlay backdrop is still drawn from C++ because the layout engine stacks children in a line and has nothing that puts one element on top of another. | Still hand-placed labels with an approximate centring formula, though they now show score, blast radius, bomb budget and speed. Replace them with a `*.ui.xml` scene through sif's layout engine, which measures text with the real font metrics. | `view/src/state/States.cpp` |
| ~~5~~ | ~~Player name entry~~ — **done**: `SettingsState` plus `logic::PlayerProfile`, persisted to `assets/player.json`. Reachable from the menu and from the save screen. | |
| 6 | **Tuning values into JSON.** `WorldConfig`, `ScoreRules` and the constants in `Character.cpp` are struct literals. sif's asset pipeline reads arbitrary JSON already. | `logic/include/bomberman/logic/World.h`, `logic/src/entity/Character.cpp` |
| 7 | **Static tiles in `constant_items`.** The arena is rebuilt every frame although it only changes when a block is destroyed. `sif::rnd::FrameContext` carries a `redrawing` flag that nothing reads yet. | `view/src/ArenaView.cpp` |

## 2. Build and repository

| # | What | Where |
|---|------|-------|
| 8 | **Real sif repository URL and a pinned tag.** `SIF_GIT_REPOSITORY` is a placeholder and `SIF_GIT_TAG` is `main`; a moving branch makes the build non-reproducible. | `cmake/GetSif.cmake` |
| ~~9~~ | ~~Upstream fix: sif reads nlohmann/json from the consumer's source tree~~ — **done upstream**: sif now uses `PROJECT_SOURCE_DIR` (tracks the most recent `project()` call, i.e. sif's own root even when fetched as a subproject) instead of `CMAKE_SOURCE_DIR`. This project still downloads the header into `sif_SOURCE_DIR/external/json` before `add_subdirectory`, since sif does not fetch json itself. | |
| ~~10~~ | ~~Upstream fix: promote the SFML backend to a real target~~ — **done upstream**: `app/sfml` + `app/headless` + `Graphics_Factory.*` moved into `sif/backends/`, building `sif_sfml`. `cmake/GetSif.cmake` no longer globs or compiles a single line of sif's sources itself. | |
| 11 | **`-Werror` in CI** once the tree has been warning-clean for a while. | `CMakeLists.txt` |
| 12 | **Official `.clang-format`.** The file here is sif's; replace it with the one from Blackboard before submitting. | `.clang-format` |
| 13 | **Sprite sheet.** Add the Bomberman sheet linked from the assignment under `assets/graphics/sprites/`. Do **not** reuse the Pac-Man project's `Broforce_boss_sprites` or the Cowboy Bebop image — commercial art in a public repository. | `assets/` |
| ~~17~~ | ~~Upstream: SFML linked into the demo executable only~~ — **done upstream**, folded into item 10: `sif_sfml` itself does `target_link_libraries(sif_sfml PUBLIC sfml-graphics sfml-window sfml-system sfml-audio)`, so every consumer gets SFML transitively and cannot compile against a mismatched copy on the default include path. | |
| ~~16~~ | ~~Upstream: sif's own SFML search accepted SFML 3~~ — **done upstream**: sif now has its own `cmake/GetSFML.cmake`, structurally identical in spirit to this project's, deliberately rejecting SFML 3. It is genuinely isolated from *this* project's own SFML choice, not merely "also correct" - see item 18. | |
| 18 | **Isolation between this project's own SFML search and sif's internal one**, now that sif finds SFML for itself. Two failure modes had to be closed, not just one: (a) a shared variable name (`SFML_DIR`) letting one project's choice silently become the other's, and (b) `find_package`'s Config-mode lookup consulting `<PackageName>_DIR` *before* any `PATHS`/`NO_DEFAULT_PATH` argument is even considered - verified by deliberately reproducing it (an ancestor scope's `SFML_DIR` pointing at a fake SFML 3.0.0 was still picked up by a `PATHS ... NO_DEFAULT_PATH`-only search). sif's fix: read only `SIF_SFML_DIR` (never the generic `SFML_DIR`/`SFML_ROOT`), and shadow `SFML_DIR` with a **local, non-cache** variable for the duration of its own search, which CMake prefers over a same-named cache entry within that scope and its descendants without ever touching the cache entry an embedding project may have set for itself. This project's own, independent search (`cmake/GetSFML.cmake`, called from `view/CMakeLists.txt`) is deliberately kept out of the root `CMakeLists.txt` for the same reason: a scope that is an *ancestor* of where sif gets fetched can still leak into sif's search (CMake target names are visible to descendant scopes by design) - a sibling scope cannot. Verified directly: a sibling project's own `find_package(SFML 3...)` and sif's own search were run in the same configure, with the generic `SFML_DIR` cache variable already pointing at the 3.x install; sif still resolved its own 2.6.1 correctly and the sibling's `SFML_DIR` cache entry came out untouched. | `cmake/GetSFML.cmake`, `view/CMakeLists.txt` |
| ~~15~~ | ~~Upstream fix: `Point2`'s constexpr default constructor defined out of line~~ — **done upstream**: now `constexpr Point2() = default;` directly in the header. The `AABB` workaround (explicit member initialisers instead of `= default`) has been reverted; `AABB()` is `constexpr = default` again. | |
| 14 | **Report** (diagrams done: `uml/logic.puml`, `uml/view.puml`, `uml/ai.puml`, `uml/patterns.puml`). The retake grades "the diagrams of your class structure" as part of code quality. sif's `uml/` shows the format. | new `uml/`, `report/` |

## 3. Defence preparation

The retake weights the oral defence at **40%**, with explicit questions about how AI was used
and whether the code is understood. Worth being able to explain without notes:

- why `IEntityFactory` is in the logic library and its implementation is not;
- what exactly is pushed through the Observer pattern and what is pulled, and why that split;
- why the world is normalized to `[-1, 1]` and what breaks if it is not;
- why bombs hold a `weak_ptr` to their owner and characters hold `shared_ptr`;
- why transitions in `StateManager` are deferred to the end of the frame;
- what the `tolerance` constant in `Score::run` is doing and how it was found.

---

## 4. What was taken from the Pac-Man project, and what was dropped

### Kept and reworked

| Pac-Man | Here | Changes |
|---|---|---|
| `model::Entity` | `logic::Entity` | Polymorphic `HitBox` replaced by a plain `AABB`; gained an event bus |
| `model::entity::Actor` | `logic::Actor` | Same tile-snapping continuous movement; emits `Moved`/`MotionChanged` instead of being polled; `Direction::Any` removed |
| `model::TileGrid` | `logic::TileGrid` | Normalized `[-1, 1]` coordinates, signed cells, out-of-range reads as solid, Bomberman arena generator |
| `model::TilePos` | `logic::TilePos` | `size_t` → `int` (the unsigned wrap-around bug) |
| `infra::math::Direction` | `logic::Direction` | `Any` dropped (it made `equal()` asymmetric); gained `to_vector` |
| `model::Tile` | `logic::Tile` | Bomberman terrain; `GhostSpawn`/`Barrier` gone — entities are not terrain |
| `infra::Score` | `logic::Score` | Now genuinely an Observer; the `% max` wrap that made every tenth coin worth zero is gone |
| `infra::ScoreBord` | `logic::ScoreBoard` | Spelling fixed; capacity 5, not 6; atomic save |
| `core::Stage`/`Stage_Manager` | `view::State`/`StateManager` | Menu is no longer popped on entering a level; deferred transitions; no public data members |
| `model::ai::*PathFinder` | `logic::ai::PathFinder` | One BFS with a predicate, replacing three implementations of an `Optimize` parameter that two of them ignored and the third treated as a no-op |
| `core::Game` | `view::Game` | Uses sif's backend and `Camera`; top-level exception handling |

### Dropped as duplicated by sif

All of `lib/infra` — `Event_Bus`, `Observer`, `Event_Store`, `Random`, `Delta_Timer`,
`Logger`, `Vector2`/`Point2`/`Rect`/`Color`/`Size`, the JSON IO layer and the AST/DTO
structs — and all of `lib/view`: the layout engine, the render-node tree, the SFML renderer,
the sprite/animation handling and the asset loaders.

### Dropped as dead or broken

- `HitBox`, `HitBox_Circle`, `HitBox_Shape`, `Separating_Axis_Theorem`,
  `World_Collision_Manager` — three were never instantiated; `HitBox_Rectangle` stored a full
  width in a field documented as a half-width; `HitBox_Shape::get_aabb` started its maximum
  search from `FLT_MIN` (the smallest *positive* float); the SAT code computed an axis vector
  and discarded it.
- `PresentationModel.h` — a second class with the same fully qualified name as `ModelView`,
  referencing members that do not exist.
- `Graphics_Factory::SFML_View` — a non-void function with no `return`.
- `GhostMode_To_Status`, `Model::remove_power_pellet` (empty body), `infra::ast::Event_Collector`.
- The Doxygen `REQUIRED` dependency, the duplicated `add_subdirectory(app)`, and the
  `CMAKE_PREFIX_PATH` overwrite that made the project unbuildable without a vendored SFML.
