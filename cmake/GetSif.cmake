# ---------------------------------------------------------------------------
# Fetches the sif engine on demand.
#
# sif is not vendored into this repository: it is a separate project with
# its own history, CI and test suite, and copying it in would mean every
# fix there has to be re-applied here by hand. FetchContent downloads it
# at configure time and adds it as a normal subproject, so `cmake -S . -B
# build` on a clean clone is all anyone needs.
#
# Two escape hatches, because working on both projects at once is the
# normal case:
#
#   -DSIF_SOURCE_DIR=/path/to/local/sif   use a local checkout instead of
#                                         downloading (edit-compile-run on
#                                         the engine without pushing)
#   -DSIF_GIT_TAG=<branch|tag|sha>        pin a different revision
#
# FetchContent caches the download in the build directory, so this costs
# nothing after the first configure.
# ---------------------------------------------------------------------------

include(FetchContent)

# TODO(daniil): pin SIF_GIT_TAG to a release tag before submitting. A moving branch
#  makes the build non-reproducible, which is exactly what a grader (and
#  CI) will notice first.
set(SIF_GIT_REPOSITORY "https://github.com/sukhoviidaniil/sif.git"
        CACHE STRING "Git repository the sif engine is fetched from")
set(SIF_GIT_TAG "main"
        CACHE STRING "Git tag/branch/commit of the sif engine to build against")
set(SIF_SOURCE_DIR "" CACHE PATH "Local sif checkout to use instead of downloading")

# sif builds an SFML demo application and its own test suite by default.
# A consumer wants neither: they cost build time and pull SFML in through
# a second path.
set(SIF_BUILD_DEMO_APP OFF CACHE BOOL "" FORCE)
set(SIF_BUILD_TESTS OFF CACHE BOOL "" FORCE)

if(SIF_SOURCE_DIR)
    message(STATUS "sif: using local checkout at ${SIF_SOURCE_DIR}")
    FetchContent_Declare(sif SOURCE_DIR "${SIF_SOURCE_DIR}")
else()
    message(STATUS "sif: fetching ${SIF_GIT_REPOSITORY}@${SIF_GIT_TAG}")
    FetchContent_Declare(
            sif
            GIT_REPOSITORY "${SIF_GIT_REPOSITORY}"
            GIT_TAG "${SIF_GIT_TAG}"
            GIT_SHALLOW TRUE
    )
endif()

# nlohmann/json has to be in place *before* sif is configured: sif's
# CMakeLists reads it from ${CMAKE_SOURCE_DIR}/external/json, which -
# once sif is a subproject - resolves to this project's source tree.
#
# TODO(daniil): upstream this. sif should look for json relative to its
#  own CMAKE_CURRENT_SOURCE_DIR (or, better, use find_package/FetchContent
#  itself) instead of the top-level source dir; then this block can go.
set(BOMBERMAN_JSON_DIR "${CMAKE_SOURCE_DIR}/external/json")
if(NOT EXISTS "${BOMBERMAN_JSON_DIR}/json.hpp")
    message(STATUS "nlohmann/json: downloading single-header into ${BOMBERMAN_JSON_DIR}")
    file(DOWNLOAD
            "https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp"
            "${BOMBERMAN_JSON_DIR}/json.hpp"
            SHOW_PROGRESS
            STATUS BOMBERMAN_JSON_STATUS
    )
    list(GET BOMBERMAN_JSON_STATUS 0 BOMBERMAN_JSON_CODE)
    if(NOT BOMBERMAN_JSON_CODE EQUAL 0)
        list(GET BOMBERMAN_JSON_STATUS 1 BOMBERMAN_JSON_MESSAGE)
        message(FATAL_ERROR "Failed to download nlohmann/json: ${BOMBERMAN_JSON_MESSAGE}")
    endif()
endif()

FetchContent_MakeAvailable(sif)

# sif ships a reference SFML backend (renderer, audio player, event
# collector, one asset loader per asset type) under app/, outside the
# library targets. It is exactly the representation layer this project
# needs, and re-typing it here would be the kind of duplication the
# assignment explicitly warns about - so it is compiled from the fetched
# sources instead.
#
# TODO(daniil): ask upstream to promote app/sfml + app/headless into a
#  proper sif_sfml target. Then this whole block collapses into one
#  target_link_libraries(... sif_sfml).
# NOTE the argument order: file(GLOB <variable> [CONFIGURE_DEPENDS]
# <globs...>). CONFIGURE_DEPENDS goes *after* the output variable. Written
# the other way round - file(GLOB CONFIGURE_DEPENDS SIF_BACKEND_SRC ...) -
# CMake takes CONFIGURE_DEPENDS as the variable name and SIF_BACKEND_SRC
# as a glob pattern that matches nothing, so the real variable stays
# empty and the only symptom is "No SOURCES given to target" at generate
# time, several lines further down.
file(GLOB SIF_BACKEND_SRC CONFIGURE_DEPENDS
        "${sif_SOURCE_DIR}/app/sfml/*.cpp"
        "${sif_SOURCE_DIR}/app/headless/*.cpp"
        "${sif_SOURCE_DIR}/app/Graphics_Factory.cpp"
)

# An empty result means the fetched sif does not have the layout this
# block expects - a moved directory, a shallow clone of the wrong tag, or
# a half-populated FetchContent cache. Saying so here beats letting
# add_library fail with a message that names neither sif nor the path.
if(NOT SIF_BACKEND_SRC)
    message(FATAL_ERROR
            "sif: no backend sources found under ${sif_SOURCE_DIR}/app.\n"
            "Expected app/sfml/*.cpp, app/headless/*.cpp and app/Graphics_Factory.cpp "
            "in the fetched engine. Check SIF_GIT_TAG (currently '${SIF_GIT_TAG}') or, "
            "if you are using -DSIF_SOURCE_DIR, that it points at the repository root "
            "rather than at its sif/ subdirectory.")
endif()

# Guard the include order rather than let it fail obscurely: without the
# SFML targets this library still *builds a target*, it just compiles
# without any -I for SFML - so the compiler silently falls back to
# whatever <SFML/...> it finds on the default include path. On a machine
# with SFML 3 in /usr/include and SFML 2.6 in /opt, that means CMake
# reports the right version while the compiler reads the wrong headers.
if(NOT TARGET sfml-graphics)
    message(FATAL_ERROR
            "sif: the SFML targets do not exist yet. Include cmake/GetSFML.cmake "
            "before cmake/GetSif.cmake - sif_sfml_backend has to link them.")
endif()

add_library(sif_sfml_backend STATIC ${SIF_BACKEND_SRC})
target_include_directories(sif_sfml_backend PUBLIC "${sif_SOURCE_DIR}/app")

# sfml-* and not just sif_lib: these sources include <SFML/...> directly,
# so the include directory has to reach *this* target. Linking it only
# into the consumer (bomberman_view) is enough to make the program link,
# which is exactly why the omission survived on a machine where SFML sits
# in the default include path and nothing needed an -I at all.
target_link_libraries(sif_sfml_backend
        PUBLIC
        sif_lib
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
)

# Exposed so the test target can reuse sif's test framework header
# instead of this project growing a second copy of it.
set(SIF_TEST_FRAMEWORK_DIR "${sif_SOURCE_DIR}/sif/test" CACHE INTERNAL "")
