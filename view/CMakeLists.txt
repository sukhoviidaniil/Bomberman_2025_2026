# ---------------------------------------------------------------------------
# bomberman_view - the representation layer.
#
# Everything that knows what the game looks like: the concrete entity
# factory (which is what attaches a view to each model), the views
# themselves, the state machine and the SFML-facing Game class.
#
# Links sif's reference SFML backend (sif_sfml) instead of re-implementing
# a renderer, an audio player and six asset loaders - the whole point of
# sif_sfml being a promoted target rather than something every consumer
# glob-compiles for itself. SFML itself comes along transitively; this file
# does not link sfml-graphics/window/system/audio directly, and does not
# need to - none of bomberman_view's own sources #include <SFML/...>.
# ---------------------------------------------------------------------------

# Bomberman's own, independent SFML search - see the file for why this is
# called from here (a sibling of the sif fetch) rather than from this
# project's root, and why it is entirely separate from whatever SFML
# sif_sfml itself is built against. Nothing below currently links its
# result: Bomberman has no direct SFML usage of its own yet, so this only
# proves the capability is there, ready, the moment something needs it -
# see cmake/GetSFML.cmake's own header for the reasoning.
include("${CMAKE_SOURCE_DIR}/cmake/GetSFML.cmake")

# Sources are listed, not globbed.
#
# A glob is evaluated at *configure* time, so adding a file leaves an
# existing build directory unaware of it until CMake happens to
# re-run - and the failure that follows is a pile of undefined
# references in an unrelated target (the linker step), which points
# nowhere near the actual cause (a source file that was never compiled
# in the first place). This has already happened once during
# development; converting every GLOB in this project to an explicit
# list is what fixed it, so if this reverts again, it is a real
# regression, not a style preference.
add_library(bomberman_view STATIC
        src/ArenaView.cpp
        src/AudioDirector.cpp
        src/EntityView.cpp
        src/EntityViews.cpp
        src/Game.cpp
        src/GameAssets.cpp
        src/SFMLEntityFactory.cpp
        src/UiScene.cpp
        src/state/StateManager.cpp
        src/state/States.cpp
)

target_include_directories(bomberman_view PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bomberman_view
        PUBLIC
        bomberman_logic
        sif_sfml
)
