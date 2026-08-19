# ---------------------------------------------------------------------------
# Locates SFML for Bomberman's *own* direct use - entirely independent of,
# and isolated from, whatever SFML sif finds or fetches internally for its
# own reference backend (sif_sfml).
#
# Bomberman does not currently include a single <SFML/...> header itself
# (everything graphical goes through sif's abstractions), so nothing here
# is linked into any target yet. It exists, and is actually run as part of
# every configure, so that the capability is real rather than aspirational:
# Bomberman is free to pick any SFML version for direct use of its own -
# including a different major version than whatever sif_sfml happens to be
# built against - the moment it needs one, with nothing to unwind first.
#
# That independence is not just "this file doesn't call sif's code" - it is
# a structural property of *where* this file is included from. See
# view/CMakeLists.txt: it is included there, a sibling of the sif fetch in
# CMakeLists.txt (both add_subdirectory'd from this project's own root, so
# neither is an ancestor of the other), never from this project's own root.
# CMake's target visibility rule for non-GLOBAL imported targets (which is
# what find_package produces here) is that a scope sees everything an
# *ancestor* scope created, but nothing a *sibling* scope created. Calling
# this from the root - as an earlier version of this project did - would
# make whatever SFML Bomberman finds here an ancestor of sif's own fetch,
# and sif's internal search would then either inherit it (if compatible) or
# have to actively detect and reject it (if not) instead of never being
# exposed to it at all. Calling it from a sibling of the fetch, as this
# version does, means sif's own search is never even aware this file ran.
#
# Bomberman's own choice of SFML version is deliberately kept independent
# of sif's default (2.6.x): this file's own version constraint,
# BOMBERMAN_SFML_MIN_VERSION, defaults to 2.6 today only because Bomberman
# has no direct SFML code yet and 2.6 is what happens to already be
# installed on the reference platform - not because Bomberman is tied to
# sif's own version choice. Pass -DBOMBERMAN_SFML_MIN_VERSION=3.0 (or edit
# the default below) and this file will look for SFML 3.x instead, with no
# change required anywhere else, including inside sif.
# ---------------------------------------------------------------------------

if(TARGET sfml-graphics)
    return()
endif()

set(BOMBERMAN_SFML_MIN_VERSION "2.6" CACHE STRING
        "Minimum SFML version Bomberman looks for, for its own direct use (independent of sif's own SFML)")
option(BOMBERMAN_FETCH_SFML "Build SFML from source when no suitable copy is found" ON)

set(BOMBERMAN_SFML_COMPONENTS graphics window system audio)

# ---- 1. Vendored -----------------------------------------------------------

set(SFML_VENDORED_ROOT "${CMAKE_SOURCE_DIR}/external/SFML-2.6.1")
if(WIN32)
    set(SFML_VENDORED_ROOT "${SFML_VENDORED_ROOT}/windows")
elseif(UNIX)
    set(SFML_VENDORED_ROOT "${SFML_VENDORED_ROOT}/linux")
endif()

if(EXISTS "${SFML_VENDORED_ROOT}/lib/cmake/SFML")
    list(PREPEND CMAKE_PREFIX_PATH "${SFML_VENDORED_ROOT}/lib/cmake/SFML")
    message(STATUS "Bomberman: trying vendored SFML at ${SFML_VENDORED_ROOT}")
endif()

# ---- 2. System --------------------------------------------------------------

find_package(SFML ${BOMBERMAN_SFML_MIN_VERSION} COMPONENTS ${BOMBERMAN_SFML_COMPONENTS} QUIET)

if(SFML_FOUND)
    message(STATUS "Bomberman: using its own SFML ${SFML_VERSION} from ${SFML_DIR}")
    return()
endif()

# ---- 3. Build from source ---------------------------------------------------

if(NOT BOMBERMAN_FETCH_SFML)
    message(STATUS
            "Bomberman: no SFML >= ${BOMBERMAN_SFML_MIN_VERSION} found and BOMBERMAN_FETCH_SFML=OFF - "
            "Bomberman's own sfml-graphics target is left undefined.")
    return()
endif()

message(STATUS "Bomberman: no suitable SFML found - fetching and building SFML 2.6.1 (first configure only)")

include(FetchContent)

set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        BombermanSFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 2.6.1
        GIT_SHALLOW TRUE
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/bomberman-sfml-src"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/bomberman-sfml-build"
)
FetchContent_MakeAvailable(BombermanSFML)
