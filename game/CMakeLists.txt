add_executable(sdl3-demo)
target_sources(sdl3-demo
    PRIVATE main.cpp
)
target_link_libraries(sdl3-demo PRIVATE SDL3::SDL3 autorelease::autorelease)

add_custom_command(TARGET sdl3-demo
                   PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/data
                       ${CMAKE_CURRENT_BINARY_DIR}/data
)
