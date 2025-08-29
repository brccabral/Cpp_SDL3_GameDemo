cmake_minimum_required(VERSION 3.21)
project(sdl3-demo)

include(GNUInstallDirs)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(autorelease REQUIRED)

add_subdirectory(game)
