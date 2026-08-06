# TODO and migration notes

Every item below also exists as a `TODO(daniil)` comment at the place in the code where the
work has to happen, so CLion's TODO tool window lists them.

---

## 1. Gameplay still to implement

| # | What | Where |
|---|------|-------|
| 1 | **Bot AI.** All four behaviours the assignment lists are queries the existing pieces already answer — see the block comment in `World::update`. | `logic/src/World.cpp` |
| 2 | **Sprites and animations.** The four views draw flat coloured boxes. Needed: a walking animation per direction, a death animation, a ticking bomb, a growing-and-fading explosion. `sif::ui::Animation` already does the frame timing, so this is authoring `*.asset.json` descriptors, not writing animation code. | `view/src/EntityViews.cpp` |
| 3 | **Sound.** `sif::audio::AudioPlayer` is created and passed around but nothing plays yet: explosion, death, victory, background music. | `view/src/Game.cpp`, states |
| 4 | **HUD as a real UI tree.** The labels are placed by hand with an approximate centring formula. Replace them with a `*.ui.xml` scene through sif's layout engine, which measures text with the real font metrics. | `view/src/state/States.cpp` |
| 5 | **Player name entry** on the game-over screen. `ScoreBoard` already stores a name per entry; it is hard-coded to `"player"`. | `view/src/state/States.cpp` |
| 6 | **Tuning values into JSON.** `WorldConfig`, `ScoreRules` and the constants in `Character.cpp` are struct literals. sif's asset pipeline reads arbitrary JSON already. | `logic/include/bomberman/logic/World.h`, `logic/src/entity/Character.cpp` |
| 7 | **Static tiles in `constant_items`.** The arena is rebuilt every frame although it only changes when a block is destroyed. `sif::rnd::FrameContext` carries a `redrawing` flag that nothing reads yet. | `view/src/ArenaView.cpp` |

## 2. Build and repository

| # | What | Where |
|---|------|-------|
| 8 | **Real sif repository URL and a pinned tag.** `SIF_GIT_REPOSITORY` is a placeholder and `SIF_GIT_TAG` is `main`; a moving branch makes the build non-reproducible. | `cmake/GetSif.cmake` |
| 9 | **Upstream fix:** sif reads nlohmann/json from `${CMAKE_SOURCE_DIR}/external/json`, which resolves to the *consumer's* source tree once sif is a subproject. It should look relative to its own directory (or fetch json itself). Until then this project downloads the header into that path. | `cmake/GetSif.cmake` |
| 10 | **Upstream fix:** sif's SFML backend lives in `app/sfml` + `app/headless`, outside the library targets. It is compiled here straight from the fetched sources; promoting it to a `sif_sfml` target would reduce this to one `target_link_libraries`. | `cmake/GetSif.cmake` |
| 11 | **`-Werror` in CI** once the tree has been warning-clean for a while. | `CMakeLists.txt` |
| 12 | **Official `.clang-format`.** The file here is sif's; replace it with the one from Blackboard before submitting. | `.clang-format` |
| 13 | **Sprite sheet.** Add the Bomberman sheet linked from the assignment under `assets/graphics/sprites/`. Do **not** reuse the Pac-Man project's `Broforce_boss_sprites` or the Cowboy Bebop image — commercial art in a public repository. | `assets/` |
| 17 | **Upstream:** sif's `app/CMakeLists.txt` links SFML into the demo executable only. Any consumer that compiles `app/sfml/*.cpp` itself (as this project does) has to link the SFML targets into *that* library, or it compiles against whatever `<SFML/...>` happens to be on the default include path. A `sif_sfml` target with `target_link_libraries(... PUBLIC sfml-graphics ...)` would make this impossible to get wrong. | upstream `sif/app/CMakeLists.txt` |
| 16 | **Upstream:** sif's own `app/CMakeLists.txt` uses `find_package(SFML 2.6 ...)`, which accepts SFML 3 and then fails to compile. It should select 2.x deliberately, the way `cmake/GetSFML.cmake` here does — or the backend should be ported to SFML 3. | upstream `sif/app/CMakeLists.txt` |
| 15 | **Upstream fix:** `sif::math::Point2` declares its default constructor `constexpr` but defines it in `Point2.cpp`. A `constexpr` function must be defined in every translation unit that uses it, so every consumer gets a "used but never defined" warning and cannot use `Point2{}` in a constant expression. `AABB` works around it by initialising the members explicitly. | upstream `sif/include/sif/math/Point2.h` |
| 14 | **Report and class diagrams.** The retake grades "the diagrams of your class structure" as part of code quality. sif's `uml/` shows the format. | new `uml/`, `report/` |

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
