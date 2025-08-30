add_executable(sdl3-demo)
target_sources(sdl3-demo
               PRIVATE main.cpp timer.cpp animation.cpp gameobject.cpp
)
target_link_libraries(sdl3-demo PRIVATE autorelease::autorelease SDL3::SDL3 SDL3_image::SDL3_image glm::glm)

add_custom_command(TARGET sdl3-demo
                   PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/data
                   ${CMAKE_CURRENT_BINARY_DIR}/data
)
