cmake_minimum_required(VERSION 3.20)
project(bomberman LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Warnings are treated as information the compiler is giving us for free.
# The Pac-Man project shipped with 39 unique warnings, three of which were
# pointing straight at real bugs (an unused loop variable that hid a
# copy-paste error, an always-false unsigned comparison, an ignored
# parameter). This project starts at zero and stays there.
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror)
# -Werror was added once every configuration (this project standalone, and
# together with sif as a subproject) verified warning-clean under these
# exact flags. Note this also applies to sif's own sources when it is
# fetched below: add_compile_options here runs before the fetch, and a
# directory scope inherits everything set in its parent, so sif is held
# to the same standard rather than silently exempted from it.

option(BOMBERMAN_BUILD_TESTS "Build the Bomberman test suite" ON)

# Declared before including cmake/GetSif.cmake below, which reads this to
# decide whether sif needs to build sif_sfml (and therefore find or fetch
# SFML) at all - see the comment on the option's use, further down, for
# why the ordering here matters.
option(BOMBERMAN_BUILD_VIEW "Build the representation layer (needs sif_sfml, hence SFML)" ON)

# sif finds its own SFML entirely by itself (see cmake/GetSFML.cmake in
# sif's own repository) - this project never needs to locate or pass in
# an SFML target for sif's sake, and does not do so here. This project's
# *own*, separate SFML search (cmake/GetSFML.cmake, for Bomberman's own
# potential direct use, independent of whatever version sif_sfml is built
# against) intentionally does not run from this root scope either - see
# the comment at the top of that file for why it is called from
# view/CMakeLists.txt instead: calling it here, an ancestor of the sif
# fetch below, would be exactly the kind of leak the isolation is meant
# to prevent.
include(cmake/GetSif.cmake)

add_subdirectory(logic)

# Gated so that a build for just the logic library and its tests
# (BOMBERMAN_BUILD_VIEW=OFF) can configure and build without CMake ever
# processing view/ - and therefore without ever touching sif_sfml or SFML
# at all, from a package manager or otherwise. That is the literal claim
# the assignment asks a submission to be able to demonstrate ("you should
# be able to compile this logic library without having SFML installed"),
# and it only holds if *configuring* the project does not reach into
# view/ - building only the bomberman_tests target afterwards is not
# enough on its own, since CMake's configure step walks the whole
# CMakeLists.txt tree regardless of which target is later requested.
if(BOMBERMAN_BUILD_VIEW)
    add_subdirectory(view)
    add_subdirectory(app)
endif()

if(BOMBERMAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
