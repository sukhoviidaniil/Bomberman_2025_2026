# Bomberman - Advanced Programming 2025-2026 - Project Report

**Contents:** [Front matter](#0-front-matter) · [Architecture](#1-architecture-overview) ·
[Design patterns](#2-design-patterns) · [Bot AI](#3-the-bot-ai) ·
[Technical decisions](#4-notable-technical-decisions) · [Testing](#5-testing) ·
[Use of AI](#6-use-of-ai) · [Limitations](#7-known-limitations) · [Bonus](#8-bonus-features-if-claiming-any)

---

## 0. Front matter

| | |
|---|---|
| **Author** | Daniil Sukhovii |
| **Student number** | s0240228 |
| **Bomberman repository** | [Bomberman_2025_2026](https://github.com/sukhoviidaniil/Bomberman_2025_2026) |
| **sif repository** | [sif](https://github.com/sukhoviidaniil/sif) |

This report covers two repositories: the Bomberman game itself, and sif - a small engine
(asset system, XML-oriented UI, render pipeline, audio) that was developed alongside the
game and is used by it as a dependency. Both are discussed here because a significant part
of the architectural work happened in sif itself, not only in the game.

---

## 1. Architecture overview

I chose to split the code into two parts: the logic/ part and the view/ part.
The logic/ part dictates how the game runs, its rules, and bot behaviour.
The logic/ part never dictates how the game is represented, how the user sees it, or how
they input a given step of their character.

In contrast, the view/ part knows nothing about the specific rules of the game, or how the
pieces move on the board. The view/ part only dictates what the current active state of
the program is, i.e. what the game looks like to the user. The view/ part also formally
knows nothing about SFML - all interaction with SFML itself is done by sif, and view/ only
uses sif. This directly makes it possible to replace not only the SFML version, but to
port the project to a different engine entirely, without any change in either part.

![Class diagram of logic/ - World, entities, ScoreBoard, IEntityFactory, TileGrid](uml/logic.png)

*Classes of logic/: `World` as the controller, the `Entity` hierarchy, `IEntityFactory`, and `Score`/`ScoreBoard`.*

![Class diagram of view/ - State, EntityView, SFMLEntityFactory, Game](uml/view.png)

*Classes of view/: the state stack (`StateManager`), the `EntityView` hierarchy, and the concrete factory.*

A direct fact, not just an intention: Bomberman has a separate build option,
`BOMBERMAN_BUILD_VIEW`. When it is off (`-DBOMBERMAN_BUILD_VIEW=OFF`), CMake does not enter
view/ during configuration at all - not merely "does not build" it. This matters precisely
because CMake processes the whole CMakeLists.txt tree at the configuration stage,
regardless of which target is later asked to be built - so the only way to prove logic/'s
independence from SFML for certain is for SFML to be impossible to find or load at all in
that configuration. This is checked by a separate CI job (`logic`), which sets up a system
with no SFML at all and configures the project with exactly this flag - if the build passes
even once and SFML is never even mentioned in the logs, that is the proof.

I chose to create a separate repository for the System Infrastructure Framework (it is
subsequently referred to as SIF or sif), for the following reasons:

- extracting the infrastructure layer into a general-purpose library, which makes it
  possible to use this layer as a base for other projects, not only for Bomberman;
- isolating the graphics engine from the core code;
- generalising the way assets are obtained, independent of the graphics library or the
  audio library;
- generalising the way the environment presents assets;
- clear control over the activation, transfer, and lifetime of assets;
- creating a simple way to create new scenes and modify existing ones - an XML format.

---

## 2. Design patterns

![Diagram of the four patterns - MVC, Observer, Abstract Factory, Singleton - with participant roles](uml/patterns.png)
*All four patterns together, showing only the participants and their roles.*

### MVC

`logic::World` plays the role of the controller - it is the one that creates and destroys
entities, decides what happens when a bomb collides with a character, and moves the bots
through AI. `World` never draws anything and does not even know that the view layer exists
- it does not have a single field of type `sf::Something`, or even `sif::rnd::Something`.
The model is the subclasses of `logic::Entity` (`Character`, `Bomb`, `Explosion`,
`PowerUp`) - they hold data (position, state, statistics) and have methods to change it,
but have no notion of how they look on screen. The View is the subclasses of
`view::EntityView` (`CharacterView`, `BombView`, `ExplosionView`, `PowerUpView`) - they
only know how to draw the current state of the model, and they receive that state through
Observer (below), rather than polling the model directly every frame.

### Observer

Specifically in this project, the most significant use is a trio of classes:
`view::EntityView`, `logic::Score`, `view::AudioDirector`.
`view::EntityView` observes the specific `sif::event::Event_Bus` that every `logic::Entity`
has, to track things such as direction, object state, and the seconds remaining until a
bomb explodes.
`logic::Score` and `view::AudioDirector`, in contrast, observe the `sif::event::Event_Bus`
of the whole `logic::World` - they are not tied to any specific object in the game, and
instead observe the entire game.

A real example of this use is the chain of events when one of the bots dies.
On death, the bot throws the fact of its own death onto its own `sif::event::Event_Bus`.
The matching `view::EntityView` catches this event and starts playing the death animation -
the bot itself has no idea it is being shown graphically.
`logic::World`, in turn, also throws a generalised event about the bot's death onto its own
`sif::event::Event_Bus`.
This event is caught by `logic::Score` and `view::AudioDirector`.
`logic::Score` accordingly computes the number of points awarded for this event in
`logic::World` - `logic::World` has no idea it is being "scored".
`view::AudioDirector` accordingly chooses to play the death sound - `logic::World` has no
idea the world even has sound effects.

### Abstract Factory

The interface `logic::IEntityFactory` lives in logic/ and declares make_character/make_bomb/
make_explosion/make_power_up. The concrete implementation, `view::SFMLEntityFactory`, lives
in view/ - it creates the model (for example `logic::Bomb`) and immediately creates the
matching `view::BombView`, subscribing it to the `sif::event::Event_Bus` of the model just
created. `World` receives an already-complete model and has no idea that a view was
created alongside it. This is exactly the property that makes it possible to have a second,
"headless" implementation - `logic::HeadlessEntityFactory` - which creates only models with
no view at all. Thanks to this, every test in test/ can run a whole round of the game
(`World::update`, collisions, explosions, bot AI) without opening a window and without
SFML - `HeadlessEntityFactory` is used in all 87 tests.

### Singleton

This pattern plays its largest role specifically inside sif - `sif::intrnl::Random` and
`sif::intrnl::Delta_Timer`. `sif::intrnl::Random` in particular is especially important,
because random numbers are generated at many levels of the project. Thanks to this pattern,
we are also able to tie every call to `sif::intrnl::Random` to one single key. This is
especially important for testing, because the `sif::intrnl::Random` key is what makes
expected, repeatable
tests possible.

---

## 3. The bot AI

The bot AI is built as a priority chain (`BotBrain` + `BotBehaviour`), rather than as a
finite state machine. The reason lies in how the assignment itself is worded: "if a bomb is
going to blow them up, they should run to safety", "if any power-ups are in their range,
try to pick them up", and so on - this is a list of conditions that have to be re-checked
every time, not a list of states with clearly defined transitions between them. In an FSM,
one would have to explicitly describe the transition from every state into every other
state (what should happen if a bot is "collecting a power-up" and a bomb suddenly appears
nearby?) - in a priority chain this is not a problem: `BotBrain` simply asks each behaviour
in turn (`SurviveBehaviour`, then depending on the bot's personality - `HuntBehaviour` or
`CollectPowerUpBehaviour` - then `BreakBlocksBehaviour`, and always `WanderBehaviour` last)
and takes the first answer that is not nullopt. A bot's "personality" (`BotPersonality`:
balanced/aggressive/collector) is simply a different ordering of these same behaviours,
with no new code at all - this is exactly the bonus described in the assignment ("give each
bot their own personality").

`DangerMap` is a structure that is rebuilt every tick from the live bombs and answers the
question "in how many seconds will this cell catch fire" (infinity, if never). It matters
that `DangerMap` accounts not only for bombs, but also for explosions already burning -
initially I only counted bombs, and this is exactly what caused the bug described below.

A concrete scenario: a bot is standing on a cell, next to which a bomb will explode in a
second, and a power-up lies two cells away. `SurviveBehaviour` is checked first and sees
that the bot's own cell is unsafe (`danger.safe(self_cell) == false`) - it searches for the
nearest cell the fire will not reach, and returns the direction there.
`CollectPowerUpBehaviour` never even gets a chance to fire, because `BotBrain` stops at the
first non-null answer. Had the bot run for the bonus instead, that would have looked like a
clear AI bug - and I did in fact observe exactly that behaviour once, until I fixed
`DangerMap` (below).

**Two real bugs found while developing the AI:**

> **Bug 1 - bots were killing themselves.** Bots were killing themselves with their own
> bombs literally within the first seconds of every round. At that point, `DangerMap`
> counted as dangerous only the cells within a bomb's radius that were still ticking down -
> but not the cells where fire was already burning. A bot would place a bomb, believe it
> had escaped (because `DangerMap` showed its new cell as safe), when in reality that cell
> was already on fire from a blast that had happened milliseconds earlier in that same
> frame. **The fix** - add live `logic::Explosion` entities to `DangerMap` as well, not
> only `logic::Bomb`.

> **Bug 2 - the escape route ran straight through fire.** After the first fix, bots still
> sometimes died - this time because the route to a "safe" cell found by `PathFinder` could
> pass through a cell that would have caught fire by the time the bot actually arrived there
> (the path was judged safe only at the moment it was searched for, not at the moment it
> was actually walked). **The fix** - `escape_after_bomb` now computes not simply "is this
> cell safe right now", but "will this cell still be safe after as many steps as it will
> actually take the bot to get there", using the bot's own speed.

![Diagram of the bot AI - DangerMap, BotBehaviour, five behaviours, BotBrain](uml/ai.png)
*`BotBrain` as a priority chain over five `BotBehaviour` implementations, and `DangerMap`, which all of them consult.*

---

## 4. Notable technical decisions

**A normalized world.** The game world is normalized to [-1, 1] coordinates instead of
pixels. `sif::rnd::Camera` separately projects these coordinates onto screen pixels using
the Fit policy, so changing the window's resolution, or even its aspect ratio, has no
effect on the game's logic at all - no speed or size constant is tied to a specific tile
size in pixels.

**`weak_ptr` versus `shared_ptr`.** `logic::Bomb` holds a `std::weak_ptr` to the character
that placed it, whereas `logic::World` holds a `std::shared_ptr` to every entity it owns.
The reason - a bomb can outlive its owner (a player can blow themselves up with their own
bomb after already being dead), and `weak_ptr` here does not let the bomb artificially
extend the lifetime of a character that no longer exists. `World`, by contrast, genuinely
owns every entity - if `World` ceases to exist, every entity must disappear along with it,
so `shared_ptr` there is the correct choice, not an arbitrary one.

**Deferred state transitions.** Transitions between states (`view::StateManager`: push/pop)
are not applied immediately at the moment they are requested; instead they are queued and
applied at the end of the frame. If, for example, `LevelState` decides to push
`SaveScoreState` from directly inside its own `update()`, and the transition happened
immediately - `LevelState` could be destroyed (or have its own call stack corrupted) before
its own `update()` had finished executing. The deferred queue avoids this whole class of
bugs.

**A float precision bug in `Score`.** `logic::Score` has a `tolerance` constant used when
counting whole seconds of survival. The reason is purely numerical: sixty frames of 1/60 of
a second do not sum to exactly 1.0, but to 0.99999994f, due to float precision. Without
`tolerance`, the player would lose one second of scoring roughly once every minute of
play - a bug that at first looked like "the score sometimes lags", and turned out to be a
classic case of float accumulation error.

---

## 5. Testing

The Bomberman project itself contains at least 87 tests. This directly covers
configuration, power-ups, bot behaviour, world-coordinate normalisation, and scoring.
Overall, both unit-level behaviour and end-to-end guards are covered.

Some tests were created specifically because a bug, or an unfinished piece of code, was
found. One example of this is one of the tests in `PowerUpTests.cpp`: I forgot to add a
shield protecting a power-up from the very explosion that created it by breaking the wall
it came out of. Without that test, the problem looked completely different - it seemed as
if power-ups simply never appeared out of blocks at all. A similar bug happened with
`DangerMap` - fire was not counted as a danger, and bots would decide to kill themselves.

---

## 6. Use of AI

Artificial Intelligence was used in this project in many areas. Its probably most important
role was played specifically BEFORE work on this project even began - analysing the full
codebase of the previous project, Pac-Man. This made it possible to see which points had
been missed, which points were redundant, and let me, as a programmer, draw conclusions not
only about the problems of the Pac-Man project, but also about the problems (now fixed) of
the SIF project, before work on the Bomberman project had even started.
Artificial Intelligence helped analyse bugs I had found myself, such as SFML linkage issues
and race conditions inside the SIF system (debugging with sanitizer tools), after I
specifically asked for that.
Artificial Intelligence helped rewrite scene descriptions from C++ into XML more quickly by
following a template - simply re-expressing the same idea in a different format.
Artificial Intelligence helped find asset options for the game - such a search could have
taken a very long time on its own, since it means methodically going through many websites
looking for specific assets.

Significant mistakes were found by me, the programmer, specifically, and were not caught by
the AI on its own:

- incorrect loading of the SFML version on a system that has both the 2.X and 3.X versions
  installed at once;
- a CI problem being silently papered over - a missing json dependency for SIF worked
  around with `nlohmann-json3-dev`;
- the problem of power-ups disappearing in an explosion being silently missed - the tests
  only checked the fact of creation and the fact of pickup SEPARATELY, not together.

There were also some attempts by the AI that I, as the programmer, rejected or reworked:

- developing asset tools under the umbrella of the Bomberman project - this directly
  violated SIF's architectural goal, the Single Responsibility Principle;
- an attempt to "reduce" the workload by copying asset files - duplication with no logical
  purpose;
- an attempt to make SIF's CMake depend on an SFML version supplied by the user - a
  complete hole for errors in SIF's own tools, which depend on the specifics of one
  particular SFML version;
- mistakes in the CMake logic for locating a specific SFML version - found, fixed, and
  documented as two distinct traps.

This difference, in my opinion, clearly shows that AI was used as a tool for analysis,
rephrasing, searching, and routine work.

---

## 7. Known limitations

**Overlapping elements in sif.** One noticeable limitation - inside SIF itself: the layout
engine cannot stack elements on top of one another, so the backdrop for the pause screen
and the round-end screen is still drawn manually as a plain `sif::rnd::Rectangle`, rather
than being part of a `*.ui.xml` scene.

**Constants living outside the config.** A few balance constants (for example, how often a
bot re-plans its behaviour) still live directly in C++ rather than in
`assets/config.json` - unlike almost everything else (the map, power-ups, round timings,
score weights), which has already been moved into the config.

**World size affects the Actors' relative movement speed.** This follows directly from the
world's normalization - if the number of cells in the world is increased without also
reducing the Actors' movement speed, the Actors will visually appear to move faster. This
is not a bug, since Actor speed is already part of the configuration, and can accordingly
be tuned by the user to their own preference or need.

**Redrawing the arena every frame.** The arena is redrawn from scratch every frame, even
though it only actually changes when a block disappears - this is purely a performance
question, invisible at this game's scale, and does not affect correctness.

---

## 8. Bonus features, if claiming any

**Bot personalities.** `logic::ai::BotPersonality` (balanced/aggressive/collector) is
exactly the bonus described in the assignment ("give each bot their own personality"): it
is the very same priority chain of behaviours, just in a different order for each type.

**Sound.** `view::AudioDirector` reacts to gameplay events (explosion, power-up pickup,
death, win/loss) through the very same Observer bus that `logic::Score` uses - no direct
calls from the game logic into the audio code anywhere.
