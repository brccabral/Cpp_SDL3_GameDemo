cmake_minimum_required(VERSION 3.21)
project(sdl3-demo)

include(GNUInstallDirs)

# Emscripten does not compile with already compiled libraries (.a, .so, .dll)
# we need the source code
if (EMSCRIPTEN)
    add_subdirectory(${SDL3_SOURCE} SDL)
    add_subdirectory(${SDL3IMAGE_SOURCE} SDL_image)
    add_subdirectory(${SDL3MIXER_SOURCE} SDL_mixer)
    add_subdirectory(${GLM_SOURCE} glm)
    add_subdirectory(${TINYXML2_SOURCE} tinyxml2)
else ()
    find_package(SDL3 REQUIRED)
    find_package(SDL3_image REQUIRED)
    find_package(SDL3_mixer REQUIRED)
    find_package(glm REQUIRED)
    find_package(tinyxml2 REQUIRED)
endif ()

find_package(autorelease REQUIRED)

add_subdirectory(game)
