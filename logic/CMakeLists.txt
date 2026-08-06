# ---------------------------------------------------------------------------
# bomberman_logic - the game rules.
#
# The assignment's central structural requirement: "In theory, you should
# be able to compile this logic library without having SFML installed."
# This target links sif_lib and nothing else, so that is enforced by the
# linker rather than by discipline - if an SFML header ever creeps in
# here, the build stops.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE BOMBERMAN_LOGIC_SRC "src/*.cpp")

add_library(bomberman_logic STATIC ${BOMBERMAN_LOGIC_SRC})

target_include_directories(bomberman_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bomberman_logic PUBLIC sif_lib)
