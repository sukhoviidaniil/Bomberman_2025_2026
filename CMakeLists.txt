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
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)

# TODO(daniil): once the code base is warning-clean in CI, add -Werror here
#  so a new warning fails the build instead of scrolling past.

option(BOMBERMAN_BUILD_TESTS "Build the Bomberman test suite" ON)

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
add_subdirectory(view)
add_subdirectory(app)

if(BOMBERMAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
