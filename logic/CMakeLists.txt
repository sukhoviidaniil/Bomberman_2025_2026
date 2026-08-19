# ---------------------------------------------------------------------------
# bomberman_logic - the game rules.
#
# The assignment's central structural requirement: "In theory, you should
# be able to compile this logic library without having SFML installed."
# This target links sif_lib and nothing else, so that is enforced by the
# linker rather than by discipline - if an SFML header ever creeps in
# here, the build stops.
# ---------------------------------------------------------------------------

# Sources are listed, not globbed - see the comment on the equivalent
# line in view/CMakeLists.txt for why a glob here has already caused a
# real, confusing build failure once (CLion's incremental build did not
# always notice a new .cpp needed a CMake reconfigure, so the file was
# silently left out of the link and the *error* showed up as "undefined
# reference" in an unrelated target).
add_library(bomberman_logic STATIC
        src/Config.cpp
        src/IEntityFactory.cpp
        src/PlayerProfile.cpp
        src/PowerUpRules.cpp
        src/Score.cpp
        src/ScoreBoard.cpp
        src/World.cpp
        src/ai/Behaviours.cpp
        src/ai/BotBehaviour.cpp
        src/ai/BotBrain.cpp
        src/ai/DangerMap.cpp
        src/ai/PathFinder.cpp
        src/entity/Actor.cpp
        src/entity/Bomb.cpp
        src/entity/Character.cpp
        src/entity/Entity.cpp
        src/entity/Explosion.cpp
        src/entity/PowerUp.cpp
        src/grid/TileGrid.cpp
)

target_include_directories(bomberman_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bomberman_logic PUBLIC sif_lib)
