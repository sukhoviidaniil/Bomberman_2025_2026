# TODO

**Legend** — every item has exactly one status, at the start of the line:

- ✅ **DONE** — verified working (built, tested, or both — stated per item). Nothing to do.
- 🔲 **OPEN** — genuinely not done. Actionable, with a concrete next step.
- 🟡 **BLOCKED** — cannot be finished without a decision or action only *you* can make
  (an external account, a judgement call, something outside this repository).
- ❌ **NOT POSSIBLE AS WRITTEN** — an earlier version of this list described something that
  turned out to be false, unactionable, or already contradicted by the code. Kept here,
  struck through, so it stops getting re-read as a live task.

Each item also exists as a `TODO(daniil)` comment at the point in the code it refers to
(search for that string), for the six items small enough to leave in place rather than
describe here.

Read **[REVIEW.md](REVIEW.md)** for the full analysis this list was produced from — what was
checked, how, and the reasoning behind every entry below.

---

## Before you can submit at all

🔲 **Repository link is missing from the README.** The assignment requires it in the report
("make sure to mention the link to the repo"); right now `README.md` names you and your
student number but contains no URL at all, and neither does `TODO.md`/`PLAN.md`. Add the
GitHub URL near the top of `README.md`, and add a CircleCI status badge next to it — the
badge is also the easiest way to satisfy "the final commit must show a successful build."
*(5 minutes.)*

🟡 **`SIF_GIT_TAG` is `"main"`, a moving branch — and `sukhoviidaniil/sif` currently has no
tags at all** (checked directly against the live repository: no tags exist). Pinning to a
release means, first, tagging a release *on the sif repository itself*
(`git tag v1.0 && git push --tags`), then setting `SIF_GIT_TAG` in
`Bomberman/cmake/GetSif.cmake` to that tag. Two repositories, in order — only you can do the
first half. *(15 minutes once you're at a computer with push access to both repos.)*

🔲 **No `report/` document exists yet.** The assignment grades this as part of Core
Functionality (20%) and expects "an overview of your design choices and the diagrams of
your class structure" — not a repeat of the assignment text. The diagrams already exist
(`uml/logic.puml`, `uml/view.puml`, `uml/ai.puml`, `uml/patterns.puml` — open them with a
PlantUML renderer, or paste into <https://www.plantuml.com/plantuml/uic>); the report is
prose *around* them, not a new drawing task. Budget half a day; see REVIEW.md for a
suggested outline.

---

## ~~Official `.clang-format`~~ ❌ NOT POSSIBLE AS WRITTEN

The old wording said: *"The file here is sif's; replace it with the one from Blackboard
before submitting."* You checked Blackboard directly — **no such file exists there**, for
this course, this year. That instruction cannot be completed as written; searching for it
further only costs time.

🔲 **What to actually do:** the current `.clang-format` is a reasonable, standard C++20
style (LLVM base, 4-space indent, 120 columns) and both this project and sif already
conform to it. Either:
- keep it as-is and say so in the report — "no official style file was provided, so I
  adopted one and applied it consistently across both repositories" is a complete,
  defensible answer if asked at the defence — or
- run `clang-format -i` across both trees once more right before submitting, just to be
  certain nothing has drifted, and note the command you used.

Do **not** spend time searching for a file that isn't there.

---

## Gameplay and design — genuinely open

🔲 **Balance constants still live as C++ literals**, not in `assets/config.json`:
`decision_interval` (0.15s bot re-think rate) in `World.cpp`, `arrival_margin_seconds` and
`escape_search_depth` in `BotBehaviour.cpp`. Everything *player-facing* (map, power-ups,
round timing, score weights) is already externalised — these three are AI-internal tuning
knobs, lower priority, but "everything is data" is not yet 100% true. *(~1 hour if you want
it; not blocking.)*

🔲 **The arena is redrawn from scratch every frame** (`ArenaView.cpp`) even though it only
changes when a block is destroyed. `sif::rnd::FrameContext` already carries a `redrawing`
flag for exactly this; nothing reads it yet. Pure performance, invisible at this game's
scale — safe to leave, worth mentioning if asked about performance at the defence.

🔲 **The pause/game-over backdrop is still a hand-drawn `Rectangle`** in `States.cpp`, not
part of the `*.ui.xml` scene, because sif's layout engine has no container that stacks one
element on top of another (everything lays out in a line). Documented as a `TODO(daniil)`
in `States.cpp`. Cosmetic only.

---

## Fixed during this review (verified, not just claimed)

✅ **DONE — sif-internal regression tests were living in Bomberman's test suite.**
`AssetLifetimeTests.cpp` and `AssetThreadingTests.cpp` test sif's own `AssetDataNode`,
`AssetHandle::lock()` and `IAssetLoader::runs_on_main_thread()` — none of it is Bomberman
logic. Moved to `sif/sif/test/`, where sif's own CI now runs them (sif: 80 → 88 tests).
Bomberman's suite is 87 tests of **only** Bomberman logic, which is what "the tests you
wrote" should show a grader.

✅ **DONE — `logic/`, `view/` and `test/` had reverted to `file(GLOB...)`** for their source
lists, silently undoing an earlier fix for a real, previously-hit bug (an IDE's incremental
build not always triggering a CMake reconfigure when a `.cpp` was added, producing a
confusing "undefined reference" linker error nowhere near the actual cause). Converted back
to explicit file lists in all three. **If this reverts a third time, treat it as a real
regression, not a style choice** — it has cost real debugging time twice already.

✅ **DONE — `-Werror` is now on**, in both this project's and sif's root `CMakeLists.txt`.
Verified warning-clean in all four build configurations: sif standalone (engine-only and
full), Bomberman standalone (logic-only and full, both against the local sif checkout).

✅ **DONE — the "no SFML installed" CI job was not actually testing that.**
`add_subdirectory(view)` ran unconditionally, so CMake's *configure* step always reached
`sif_sfml` and always searched for (or, without `libsfml-dev`, fetched and built from
source) SFML — regardless of which target was later built. Added
`-DBOMBERMAN_BUILD_VIEW=OFF`, which keeps `view/` and `app/` out of the configure step
entirely, and taught `cmake/GetSif.cmake` to turn off sif's own `SIF_BUILD_SFML_BACKEND` in
that case too. CI's `logic` job now uses this flag and genuinely never touches SFML.
Verified: configuring with the flag produces no `sif: SFML ...` message at all, `view/` does
not appear in the build tree, and `bomberman_tests` still links and passes.

---

## Everything else — see REVIEW.md for the full accounting

✅ **DONE** — sif's own SFML search (`sif/cmake/GetSFML.cmake`) is isolated from this
project's own (`cmake/GetSFML.cmake`), including against `find_package`'s `<Pkg>_DIR`
lookup bypassing `NO_DEFAULT_PATH`. See REVIEW.md, "Build system" section, for how this was
verified (an ancestor-scope leak was deliberately reproduced and closed).

✅ **DONE** — Bot AI (`logic/ai/`), power-ups with reveal-shielding, the full save-score
flow, the `*.ui.xml` UI, and the asset pipeline (`sif_sprite_packer` + `Asset_GUID_Assignment`
+ `Asset_Reference_Serialization`, all three sif tools, none of it hand-rolled) are all
built, tested and verified live (screenshots and details in REVIEW.md). Not repeated here.

---

## What was taken from the Pac-Man project, and what was dropped

Unchanged from the previous version of this list — kept for the defence, where you may be
asked what carried over and why.

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
