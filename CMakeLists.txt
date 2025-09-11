cmake_minimum_required(VERSION 3.21)
project(sdl3-demo)

include(GNUInstallDirs)

# Emscripten does not compile with shared libraries (.so, .dll)
# or we add the source code, or we need to install static libraries
# in $EMSDK/emscripten/main/cache/sysroot
#    add_subdirectory(${SDL3_SOURCE} SDL)
#    add_subdirectory(${SDL3IMAGE_SOURCE} SDL_image)
#    add_subdirectory(${SDL3MIXER_SOURCE} SDL_mixer)
#    add_subdirectory(${GLM_SOURCE} glm)
#    add_subdirectory(${TINYXML2_SOURCE} tinyxml2)
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_mixer REQUIRED)
find_package(glm REQUIRED)
find_package(tinyxml2 REQUIRED)
find_package(autorelease REQUIRED)

add_subdirectory(game)
