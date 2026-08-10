# PLAN — from here to a submittable project

Where the project stands today and, step by step, what is left. Every step says
what is done, where, and how you will know it is finished.

The retake grades four things:

| | Weight | What it covers |
|---|---|---|
| Core functionality | 20% | working game, documentation, the required design patterns |
| Code quality | 40% | **your tests**, **your class diagrams**, good design practice |
| Defence | 40% | an oral exam on your code, your process, and your use of AI |
| Bonus | 10% | extra mechanics, polish, extra patterns |

Note the shape of that: **the game itself is worth 20%, and being able to explain
it is worth 40%.** Do not spend the last week polishing gameplay at the expense
of understanding what is already there.

Deadline: **22 August**.

---

## Where the project is now

Done and verified:

- **Structure** — `bomberman_logic` links sif and nothing else, so "the logic
  library compiles without SFML" is enforced by the linker; CI has a job that
  builds it on a machine with no SFML installed.
- **Coordinates** — the world is normalized to `[-1, 1]`, `sif::rnd::Camera`
  projects to pixels, tiles stay square in any window.
- **Patterns** — Abstract Factory (`logic::IEntityFactory` +
  `view::SFMLEntityFactory`), Observer (per-entity buses for views, a world bus
  for `Score` and `AudioDirector`), Singleton (`Delta_Timer`, `Random` in sif),
  MVC, plus a State stack for the four screens.
- **Rules** — bombs, chain reactions, cross-shaped blasts stopped by walls and
  absorbed by one destructible block, power-ups (Fire / ExtraBomb / Skates),
  lives, win/lose, scoring by survival time, blocks, pick-ups and kills.
- **Presentation** — real sprites and animations (walk per direction, idle,
  death, ticking bomb, growing explosion), tile art, and sound driven by
  gameplay events.
- **Configuration** — everything is data: `assets/config.json` carries the
  global seed, the map (generated from a seed *or* written as a character
  matrix), balance, window and audio.
- **Tests** — 35 cases, no window required.

Missing: **bot AI**, a report, class diagrams, and a handful of smaller items
listed in `TODO.md`.

---

## Step 1 — Bot AI  *(the one real gap; 2–3 days)*

Without it there is no game: three motionless bots is not "the Player competes
against three computer-controlled characters". This is the only step that is
genuinely blocking.

The assignment lists four behaviours. Each is already a query the existing
pieces answer — the hook is the `TODO(daniil)` comment in `World::update`.

**1.1 A danger map.** New `logic/ai/DangerMap.h/.cpp`: given the grid and the
live bombs, mark every cell a blast will reach and after how long. Everything
else is a predicate over this.

```cpp
bool safe(const TilePos& c) const;          // no bomb reaches it
float seconds_until_blast(const TilePos&) const;
```

*Done when:* a test places a bomb of radius 2 in a corridor and asserts exactly
which cells are unsafe, including that a wall shortens the line.

**1.2 Survival.** If the bot's own cell is unsafe, walk to the nearest safe cell:
`PathFinder::find_nearest(grid, here, passable, safe)`. This takes priority over
everything else.

*Done when:* a bot dropped next to a lit bomb survives it in a test that runs the
world forward two seconds.

**1.3 Collect.** If a power-up is within N tiles and the path is safe, go get it.

**1.4 Open up the playfield.** If no power-up is worth chasing, path to a cell
adjacent to a `Destructible` tile, place a bomb, then rule 1.2 takes over and
makes it run. Note the bot must check it *has* an escape before placing —
otherwise it kills itself, which reads as a bug even though it is "correct".

**1.5 Attack.** When no breakable walls remain, or an enemy is within a few
tiles, path towards the nearest living character and bomb it.

**1.6 Understanding power-ups.** Read `character.blast_radius()` when deciding
how far "away" is, and `character.bomb_budget()` when deciding whether a second
bomb can be spared. This is free if 1.1–1.5 use the character's own stats rather
than constants — do not hard-code `2`.

**Design note for the defence:** make each behaviour a small object behind one
interface (`BotBrain`) and pick between them by priority, rather than one
`if/else` cascade. Then "give each bot its own personality" (a listed bonus)
becomes a different priority list, not new code.

---

## Step 2 — Diagrams and report  *(1 day; part of the graded 40% + 20%)*

The retake explicitly grades "the diagrams of your class structure" and the
report. This is the cheapest mark in the whole project and the easiest to
forget.

**2.1** `uml/logic.puml` — entities, World, Score, IEntityFactory, the grid.
**2.2** `uml/view.puml` — EntityView hierarchy, SFMLEntityFactory, states, Game.
**2.3** `uml/patterns.puml` — one diagram showing *only* the four required
patterns and who plays which role. This is the one you will be asked about.
**2.4** `report/report.md`, about two pages: the logic/representation split, why
the world is normalized, what travels by event and what is pulled, and how sif
is consumed. Do not repeat the assignment text.

Use sif's `uml/*.puml` as the format reference.

---

## Step 3 — Finish the presentation layer  *(1–2 days)*

**3.1 HUD from a scene file.** The labels are placed by hand with an approximate
centring formula (`States.cpp`). Replace them with a `*.ui.xml` scene through
sif's layout engine, which measures text with real font metrics. Show: score,
lives, blast radius, bomb budget.
**3.2 Lives.** The rules give a character one life; the assignment's Bomberman
is last-man-standing, so this is arguably correct — but decide deliberately and
say so in the report.
**3.3 Player name entry** on the game-over screen (`ScoreBoard` already stores a
name per entry; it is hard-coded to `"player"`).
**3.4 Menu sound.** `sfx_menu` is loaded and never played — hook it to menu
navigation.
**3.5 Static tiles in `constant_items`.** The arena is rebuilt every frame
though it only changes when a block is destroyed. `sif::rnd::FrameContext`
already carries a `redrawing` flag that nothing reads.

---

## Step 4 — Quality pass  *(1 day)*

**4.1** Replace `.clang-format` with the official one from Blackboard and run it
over everything.
**4.2** Add `-Werror` to the CI build. The tree is warning-clean today; keep it
that way mechanically rather than by discipline.
**4.3** Fix the two upstream sif issues (items 15–17 in `TODO.md`) — they are
small, they are yours, and "I found and fixed a bug in my own engine" is a good
answer at a defence.
**4.4** Run under valgrind once (`valgrind --leak-check=full ./bomberman`). The
assignment names it explicitly; smart pointers make a clean run likely, but
"likely" is not a result.
**4.5** Grow the test suite towards the AI: the danger map, the escape decision,
the "does not bomb itself into a corner" case. These are the tests that are
worth talking about, because they test behaviour rather than getters.

---

## Step 5 — Submission mechanics  *(half a day, do not leave it last)*

**5.1** Pin `SIF_GIT_TAG` to a release tag, not `main`. A moving branch makes
the build non-reproducible, and the grader builds it after you stop touching it.
**5.2** Verify a clean clone builds on the reference platform: Ubuntu 24.04,
GCC 13, **SFML 2.6.1** (`cmake/GetSFML.cmake` refuses SFML 3 deliberately).
**5.3** Green CI on the final commit — this is an explicit requirement, not a
nicety.
**5.4** `README.md`: name, student number, repository link, build and run
instructions, controls.
**5.5** Attribution: `assets/ATTRIBUTION.md` covers the art and sound packs and
the font. Keep it accurate; the repository is public.
**5.6** Commit history: frequent, small, with messages that say why. It is
visible evidence of process, and process is on the defence rubric.

---

## Step 6 — Prepare for the defence  *(40% of the grade — do not skip)*

Write out your own answers, in your own words, to:

- Why is `IEntityFactory` in the logic library and its implementation not?
- What travels by event and what is read directly each frame, and why that split?
- Why is the world normalized to `[-1, 1]`? What breaks without it?
- Why do bombs hold a `weak_ptr` to their owner while the World holds `shared_ptr`?
- Why are state transitions deferred to the end of the frame?
- What is the `tolerance` constant in `Score` doing, and how was it found?
- Why does `TileGrid::generate_arena(chance, seed)` reseed the *shared* generator
  instead of keeping a private one, and what does that cost?
- How do you know the logic library really does not depend on SFML?

Then the AI question, which will be asked: be able to say which parts you
generated, what you changed afterwards and why, and walk through any file you
kept. **Anything you cannot explain out loud, simplify until you can.** A
smaller design you own beats a larger one you are quoting.

---

## Suggested order

| Days | Work |
|---|---|
| 1–3 | Step 1 (bot AI) — the only blocking gap |
| 4 | Step 2 (diagrams + report) |
| 5–6 | Step 3 (HUD, name entry, polish) |
| 7 | Step 4 (quality, valgrind, more tests) |
| 8 | Step 5 (submission mechanics), then Step 6 continuously |

Steps 2, 5 and 6 are worth more per hour than anything in step 3. If time runs
short, ship a plain HUD and a written report rather than a beautiful HUD and no
diagrams.
