# Readiness review — Bomberman + sif

**Date of this review:** 2026-08-19. **Deadline:** 22 August 2026 — **3 days from this
review.**
**Scope:** both repositories — `Bomberman_2025_2026` (the submission) and `sif` (the engine
it depends on, fetched via CMake `FetchContent`).
**Method:** every claim below was checked directly, not read off an earlier TODO/PLAN and
trusted — both repositories were built from a clean state (four configurations: sif
engine-only, sif full, Bomberman logic-only, Bomberman full), all test suites were run, the
game was played live under Xvfb with real keyboard input, and the source was read file by
file for the sections below, not sampled.

---

## Verdict

**The project is close to submittable, and the gap is almost entirely documentation, not
code.** The game works, is well architected, uses every required pattern correctly, and has
a real (not decorative) test suite. What is missing — a written report, a repository link in
the README, a pinned engine version — is real but small and entirely within reach in the
three remaining days, *if the report is started today*. The single biggest risk to the grade
is not a bug; it is running out of time to write the report and being unable to fully explain
the AI-heavy parts of the codebase at the defence (see "Defence risk" below).

| Criterion (assignment weight) | Assessment |
|---|---|
| Core Functionality — 20% | Fully implemented and verified live. Report is the only missing piece. |
| Code Quality — 40% | Strong: 175 tests across both repos, four class diagrams, clean patterns, `-Werror`-clean. Two structural mistakes found and fixed during this review (see below) — worth knowing they existed and why, for the defence. |
| Defence — 40% | Cannot be assessed from code. See "Defence risk" — this is where the real risk to the final grade sits. |
| Bonus — 10% | Several bonus items already delivered (bot personalities, sound, generic-ish patterns) without being framed as bonus anywhere. |

---

## What was actually checked (methodology)

Trusting a project's own TODO/PLAN files is exactly how the confusion this review was asked
to resolve arose in the first place, so nothing below is asserted without one of:

1. **A clean build.** Four configurations, all from a state with `external/`, `build/`,
   generated assets and `.git`/`.idea` removed first:
   - sif, engine only (`-DSIF_BUILD_TOOLS=OFF -DSIF_BUILD_SFML_BACKEND=OFF`) — **0
     errors, 0 warnings, 80/80 tests.**
   - sif, full (`-DSIF_BUILD_DEMO_APP=ON`) — **0 errors, 0 warnings**, 88/88 tests
     (after moving two misplaced test files here — see below), headless demo check passes,
     `Asset_GUID_Assignment` and `Asset_Reference_Serialization` both run successfully.
   - Bomberman, full, against the local sif checkout — **0 errors, 0 warnings**,
     87/87 tests.
   - Bomberman, logic-only (`-DBOMBERMAN_BUILD_VIEW=OFF`, a flag added during this
     review) — **0 errors, 0 warnings**, 87/87 tests, and — checked directly — CMake
     never even *configures* `view/`, so SFML is never searched for. This is what the
     assignment's "compile this logic library without having SFML installed" actually
     requires, and the project's own CI job claiming to test this was not, in fact, testing
     it (see "Fixed during this review").
2. **A live run.** The full game was launched under Xvfb, driven with real `xdotool`
   keystrokes (not scripted game-state injection): menu → play → move → place a bomb →
   die → the `GAME OVER` / `SAVE SCORE` / `CHANGE NAME` / `DISCARD` screen, all
   screenshotted and visually confirmed correct.
3. **Reading**, not skimming: `World.cpp`, `Score.cpp`, `Config.cpp`, `BotBehaviour.cpp`,
   `Behaviours.cpp`, `BotBrain.cpp`, every `CMakeLists.txt` in both repositories, both
   `cmake/GetSFML.cmake` files, `States.cpp`, and a full inventory of every `TODO(daniil)`
   comment actually present in the source (six, listed below — cross-checked against what
   `TODO.md` claimed).
4. **Direct verification of two claims that turned out to be false**: that an official
   `.clang-format` exists on the course site (see below — it does not, confirmed by you
   directly), and that `sukhoviidaniil/sif` has a tagged release to pin to (checked directly
   against the live repository — it has none, only a `main` branch).

---

## Fixed during this review

Four real issues were found and corrected, not just noted. All four are verified against a
clean rebuild afterwards.

### 1. sif's own bug regression tests were living in Bomberman's test suite

`test/AssetLifetimeTests.cpp` and `test/AssetThreadingTests.cpp` — 8 tests total — exercise
`sif::asset::AssetDataNode`'s virtual destructor, `AssetHandle::lock()`'s ownership
semantics, and `IAssetLoader::runs_on_main_thread()`. None of this is Bomberman logic; all of
it is regression coverage for bugs that were once found *in sif itself* (a
`new-delete-type-mismatch` caught by AddressSanitizer, and an OpenGL/OpenAL threading race
caught by ThreadSanitizer, both from earlier debugging sessions on this same engine).

This matters for two reasons. First, it is simply mis-filed: a grader reading Bomberman's
test suite and finding tests about `AssetDataNode` will reasonably wonder why. Second, and
worse, it means **sif's own CI never ran these tests** — if someone used sif without
Bomberman, or if Bomberman's test suite were disabled, this coverage would silently vanish.
Moved both files to `sif/sif/test/`, added them to `sif/sif/test/CMakeLists.txt`. sif's suite
is now 88 tests; Bomberman's is 87, all of them about Bomberman.

### 2. `logic/`, `view/` and `test/` had reverted to `file(GLOB...)`

All three `CMakeLists.txt` files were back to globbing their source lists
(`file(GLOB_RECURSE ... "src/*.cpp")`) instead of listing files explicitly. This is not a
style nit: it is **the exact, previously-diagnosed cause of a real build failure** reported
earlier in this project's history — a glob is evaluated at CMake *configure* time, so a
newly-added `.cpp` is silently excluded from the build until something forces a reconfigure,
and the resulting failure is a wall of `undefined reference` linker errors in a target that
looks unrelated to the actual cause. All three were converted back to explicit file lists,
with a comment on each explaining why, specifically so a future edit does not "clean this up"
back into a glob a third time.

### 3. `-Werror` was a TODO comment, not a flag

Both projects' compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion
-Wsign-conversion`) were already clean, verified by every build in this review — the
`-Werror` flag itself was just never turned on. Enabled in both `CMakeLists.txt` files,
re-verified clean across all four build configurations listed above. A new warning now fails
the build immediately, on the commit that introduced it, rather than being something to
notice later.

### 4. The "compiles without SFML" CI job was not actually testing that

`add_subdirectory(view)` in the root `CMakeLists.txt` was unconditional. CMake's *configure*
step walks the entire `CMakeLists.txt` tree regardless of which target is later built — so
even the CI job that specifically installs a toolchain *without* SFML, intending to prove the
logic library needs none, still configured `view/`, which still needed `sif_sfml`, which
still made sif go and **fetch and build a private copy of SFML from source** during that
"no SFML" job. The job did not fail (sif's fetch fallback made sure of that), but it also did
not demonstrate what its own name and comment claimed.

Fixed with a new option, `BOMBERMAN_BUILD_VIEW` (default `ON`), that keeps `view/` and
`app/` out of the configure step entirely when off, and a corresponding change to
`cmake/GetSif.cmake` so sif's own `SIF_BUILD_SFML_BACKEND` is also turned off in that case —
otherwise sif would still search for SFML on its own initiative regardless of what Bomberman
asked for. CI's `logic` job now passes `-DBOMBERMAN_BUILD_VIEW=OFF`; verified directly that
configuring with this flag produces no `sif: SFML ...` message at all and that `view/` does
not appear anywhere in the resulting build tree.

---

## Corrected claims (not bugs — false statements in the project's own docs)

### "Replace `.clang-format` with the official one from Blackboard"

You verified this directly: **no official `.clang-format` exists on the course site for this
assignment.** The old TODO item asked for something that cannot be done. This is worth
flagging on its own, separate from ordinary missing features, because a student who trusts
their own TODO list at 11pm the night before a deadline could genuinely lose real time
searching for a file that was never provided. Reworded in the new TODO.md to say plainly that
no such file exists and that the project's current, consistent style is a legitimate answer
to defend as-is.

### "`SIF_GIT_REPOSITORY` is a placeholder"

This is now **out of date, not wrong** — the URL (`https://github.com/sukhoviidaniil/sif.git`)
is real and was confirmed reachable during this review; it is not a placeholder anymore. What
*is* still accurate: `SIF_GIT_TAG` is `"main"`, a moving branch, and the live repository
currently has **no tags at all** to pin to. This is now correctly split into two facts in the
new TODO.md rather than one slightly-wrong sentence.

---

## Assignment requirements — verified against the code, not the TODO

### Core functionality (section 2.1)

| Requirement | Status | Where |
|---|---|---|
| Startup screen with top-5 scoreboard + Play | ✅ Verified live | `MenuState`, `ScoreBoard` |
| Continuous movement, collision, spawn top-left | ✅ Verified live and in `LogicTests.cpp` | `Actor::move`, `TileGrid` |
| Bombs: place, timed fuse, cross blast, walk-off-then-blocked | ✅ Verified live and by test | `Bomb`, `World::spread_blast` |
| Blast stops at indestructible, absorbed by one destructible | ✅ Tested directly (`the_path_finder_walks_around_walls` and blast tests) | `World::spread_blast` |
| Chain reactions | ✅ Tested (`resolve_detonations` loop, not recursion, so a chain resolves within one frame) | `World.cpp` |
| Power-ups: Fire / Extra Bomb / Skates, 25%-class drop chance | ✅ Configurable, tested, reveal-shielded (a real bug found and fixed earlier: power-ups used to die on the same frame they spawned) | `PowerUpRules`, `PowerUpTests.cpp` |
| Three bots, each with the four required behaviours | ✅ `DangerMap` + five `BotBehaviour`s + `BotBrain` priority chain, 26 AI-specific tests, verified bots do not suicide across six seeded rounds and measurably clear the arena | `logic/ai/` |
| Bots understand power-ups (radius, budget) | ✅ Behaviours read `Character::blast_radius()`/`bomb_budget()` directly rather than duplicating the numbers | `Behaviours.cpp` |
| Scoring: survival time, blocks, power-ups, kills, win/lose bonus | ✅ Verified by `Score.cpp` and its own tests, including the specific float-accumulation bug (`0.99999994f` from 60 frames of `1/60s`) that was caught and fixed | `Score.cpp` |
| Top-5 scoreboard persisted across runs | ✅ `ScoreBoard::save/load`, atomic file write | `ScoreBoard.cpp` |

### Visuals and aesthetics (section 2.2)

| Requirement | Status |
|---|---|
| Walking animation, 4 directions | ✅ `sif_sprite_packer`-built strips, verified live |
| Death animation | ✅ |
| Bomb tick animation | ✅, speeds up on `FuseCritical` |
| Explosion grow/fade per tile | ✅ driven by the model's own `progress()`, not a separate view-side timer |
| Victory animation | Not implemented (explicitly optional in the assignment) |

### Technical requirements (section 3.1)

| Requirement | Status | Note |
|---|---|---|
| Logic library compiles without SFML | ✅ **Now genuinely true and CI-verified** | Was previously untrue in practice — see "Fixed during this review", item 4 |
| `World` as entity controller, no SFML utilities | ✅ | Collision via plain `AABB` intersection, no `sf::` type anywhere in `logic/` |
| `Camera`, normalized `[-1, 1]` world | ✅ (in sif, `sif::rnd::Camera`, reused rather than reimplemented — reasonable given the assignment does not forbid using a shared engine, and the logic/representation split is what is actually graded) | |
| `Score` via `Stopwatch`-driven ticks and Observer | ✅ | Score never touches `World` directly |
| `Random` as a Mersenne Twister singleton, stored as a member | ✅ (in sif, `sif::intrnl::Random`) | Seedable, used for reproducible test runs |
| **MVC** | ✅ `World` = controller, `Entity` subclasses = model, `EntityView` subclasses = view | |
| **Observer** | ✅ Two independent uses, matching the assignment's own description almost exactly: per-entity buses drive views (attached in the concrete factory, exactly as specified), a world bus drives `Score` | |
| **Abstract Factory** | ✅ `IEntityFactory` in the logic library, `SFMLEntityFactory` (and `HeadlessEntityFactory`, used by every test) as concrete implementations | |
| **Singleton** | ✅ `Random`, `Delta_Timer` (in sif) | |
| Smart pointers only, no raw owning pointers | ✅ Checked directly — zero `new`/`delete` in either repository's game code | |
| No `dynamic_cast` | ✅ Checked directly — zero occurrences | |
| Virtual destructors where needed | ✅ Every polymorphic base (`Entity`, `EntityView`, `State`, `BotBehaviour`) has one | |
| `override` used consistently | ✅ | |
| Exception handling | ✅ `main.cpp` top-level catch, `Config`/`ScoreBoard`/`PlayerProfile` all throw on bad input with named, actionable messages | |
| Namespaces dividing modules | ✅ `bomberman::logic`, `bomberman::logic::ai`, `bomberman::view`, `bomberman::view::state`, mirrored in sif | |

---

## Defence risk

This is where the actual risk to the final grade sits, and it cannot be fixed by writing more
code.

The codebase is large, and a substantial fraction of it — the AI behaviour tree, the CMake
isolation work between the two SFML searches, the asset threading fix — was built through an
extended, AI-assisted process across many sessions. The assignment is explicit that **the
defence carries the AI-use conversation directly**: *"you must put an effort into
understanding your code... your defence will carry significant weight and will test your
comprehension of any AI-assisted code."*

Two concrete things worth doing before the defence, not before the deadline:

1. **Walk through `World::update_bots` and one full `BotBrain::decide` call out loud**,
   without notes, tracing exactly which `BotBehaviour` fires and why, for a specific
   scenario (e.g. "bot standing next to a lit bomb with a power-up two cells away"). This is
   almost certainly going to be asked directly, and it is the single densest piece of logic
   in the project.
2. **Be able to explain, in one sentence each, without re-deriving them live**: why the world
   is normalized to `[-1, 1]`; why `Bomb` holds a `weak_ptr` to its owner while `World` holds
   `shared_ptr`s to everything; why state transitions in `StateManager` are deferred to the
   end of the frame instead of applied immediately; what the `tolerance` constant in
   `Score::Score` is for. These are already written down as defence-prep bullet points in the
   history of this project's own PLAN.md — the point is to be able to say them without
   reading them.

Nothing in the code itself suggests a lack of understanding — the comments throughout
consistently explain *why*, not just *what*, and several real bugs were demonstrably found
and fixed with correct, specific reasoning (the float-accumulation bug, the OpenAL threading
race, the power-up reveal-shield bug). That is good evidence of comprehension. The risk is
purely about being able to reproduce that understanding live, under a question, in the room.

---

## Remaining open items, by priority

Given three days:

**Day 1 (today):** Start the report. It is the only Core Functionality item with zero code
work remaining and a hard, non-negotiable minimum size. Add the repository link and a CI
badge to the README while the report is open anyway (five minutes).

**Day 2:** Tag a release on the sif repository and pin `SIF_GIT_TAG` to it. Re-verify both
CI jobs are green on the tagged commit. Do one more `clang-format -i` pass across both trees.

**Day 3:** Defence preparation — the two items under "Defence risk" above. Do not spend this
day on new features; nothing left on the TODO list is worth trading defence-readiness for at
this point.

Everything else on the list (the three remaining un-externalised constants, the redrawn-every-
frame arena, the hand-drawn pause backdrop) is genuinely low priority: none of it affects
correctness, none of it is likely to be asked about unprompted, and all three are the kind of
thing that is fine to mention as "known, deliberately deprioritized" if asked.
