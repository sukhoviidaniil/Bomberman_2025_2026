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

# Order matters: GetSif.cmake creates the sif_sfml_backend target, which
# compiles SFML-using sources and therefore has to link the SFML targets -
# so those have to exist first.
include(cmake/GetSFML.cmake)
include(cmake/GetSif.cmake)

add_subdirectory(logic)
add_subdirectory(view)
add_subdirectory(app)

if(BOMBERMAN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
