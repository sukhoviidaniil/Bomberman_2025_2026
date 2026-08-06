# ---------------------------------------------------------------------------
# bomberman_view - the representation layer.
#
# Everything that knows what the game looks like: the concrete entity
# factory (which is what attaches a view to each model), the views
# themselves, the state machine and the SFML-facing Game class.
#
# It links sif's reference SFML backend (see cmake/GetSif.cmake) instead
# of re-implementing a renderer, an audio player and six asset loaders.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE BOMBERMAN_VIEW_SRC "src/*.cpp")

add_library(bomberman_view STATIC ${BOMBERMAN_VIEW_SRC})

target_include_directories(bomberman_view PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(bomberman_view
        PUBLIC
        bomberman_logic
        sif_sfml_backend
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
)
