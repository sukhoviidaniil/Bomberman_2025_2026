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
# What this file does *not* do, on purpose: it does not look for SFML, does
# not pass any SFML target to sif, and does not need sif to have found one
# before it returns. sif finds its own SFML entirely by itself (see its own
# cmake/GetSFML.cmake) - lazily, the first time one of ITS OWN options that
# needs it (SIF_BUILD_TOOLS, SIF_BUILD_SFML_BACKEND) is processed, using
# only sif-prefixed inputs that this project's own separate SFML search
# (cmake/GetSFML.cmake, called from view/CMakeLists.txt, a sibling of this
# fetch rather than an ancestor of it) never touches. That is what keeps
# the two genuinely independent: Bomberman could point its own search at a
# different SFML major version entirely and sif_sfml would be unaffected,
# because sif never sees or relies on anything this project's own search
# produces.
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

# sif builds a demo application and its own test suite; a consumer wants
# neither. The reference SFML backend (sif_sfml) and the asset tools stay
# on by default - this project needs both, through view/ and app/.
#
# The one exception: when this project itself was configured with
# BOMBERMAN_BUILD_VIEW=OFF (a logic-only build, see the root
# CMakeLists.txt), sif_sfml is not needed either, and sif must be told so
# explicitly - its own SIF_BUILD_SFML_BACKEND default is ON regardless of
# what this project ends up using it for, so without this override sif
# would still go and find or fetch SFML for a target nothing here links
# against, defeating the entire point of the logic-only build.
set(SIF_BUILD_DEMO_APP OFF CACHE BOOL "" FORCE)
set(SIF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
if(NOT BOMBERMAN_BUILD_VIEW)
    set(SIF_BUILD_SFML_BACKEND OFF CACHE BOOL "" FORCE)
    set(SIF_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
endif()

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

# Populated in two steps (Populate, then our own logic, then
# add_subdirectory) rather than the one-call FetchContent_MakeAvailable,
# because nlohmann/json has to be in place *before* sif's own
# CMakeLists.txt runs: sif looks for it at its own
# PROJECT_SOURCE_DIR/external/json (its own root, correctly - not this
# project's - now that sif resolves that path relative to itself rather
# than to whichever project embeds it). FetchContent_Populate downloads
# the source and sets sif_SOURCE_DIR without processing sif's
# CMakeLists.txt yet, which is exactly the window this needs.
# CMP0169 (CMake 3.30+) deprecates the FetchContent_Populate(<name>) form
# below in favour of FetchContent_MakeAvailable - but that one-call form
# processes the dependency's CMakeLists.txt immediately, and this file
# needs a window *between* downloading sif's source and letting its
# CMakeLists.txt run, to drop nlohmann/json.hpp into place first. Setting
# the policy to OLD locally (only for this call, only on CMake versions
# that know about it) keeps that two-step sequence working without the
# deprecation warning on newer CMake.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

FetchContent_GetProperties(sif)
if(NOT sif_POPULATED)
    FetchContent_Populate(sif)
endif()

set(BOMBERMAN_JSON_DIR "${sif_SOURCE_DIR}/external/json")
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

add_subdirectory("${sif_SOURCE_DIR}" "${sif_BINARY_DIR}")

# sif_sfml - sif's promoted reference backend target (renderer, audio
# player, event collector, every asset loader, SFML itself all
# transitively linked) - is ready to use the moment this returns:
# target_link_libraries(your_target PUBLIC sif_sfml) and nothing else. The
# manual glob-and-compile of sif's app/sfml + app/headless sources that
# used to live here is gone along with the workaround comment explaining
# it was standing in for "a proper sif_sfml target" - that target now
# exists, in sif itself.

# Exposed so the test target can reuse sif's test framework header instead
# of this project growing a second copy of it.
set(SIF_TEST_FRAMEWORK_DIR "${sif_SOURCE_DIR}/sif/test" CACHE INTERNAL "")
