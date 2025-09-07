add_executable(sdl3-demo)
target_compile_features(sdl3-demo PRIVATE cxx_std_23)
target_sources(sdl3-demo
               PRIVATE
               main.cpp
               timer.cpp
               animation.cpp
               gameobject.cpp
               tmx.cpp
)
target_link_libraries(sdl3-demo PRIVATE
                      autorelease::autorelease
                      SDL3::SDL3
                      SDL3_image::SDL3_image
                      SDL3_mixer::SDL3_mixer
                      glm::glm
                      tinyxml2::tinyxml2
)

set(DATA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(DATA_DEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/data")

file(GLOB_RECURSE DATA_FILES "${DATA_SOURCE_DIR}/*")

add_custom_command(OUTPUT "${DATA_DEST_DIR}/.touch"
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   "${DATA_SOURCE_DIR}"
                   "${DATA_DEST_DIR}"
                   COMMAND ${CMAKE_COMMAND} -E touch "${DATA_DEST_DIR}/.touch"
                   DEPENDS ${DATA_FILES}
)
add_custom_target(copy_data
                  DEPENDS "${DATA_DEST_DIR}/.touch"
)
add_dependencies(sdl3-demo copy_data)

if (EMSCRIPTEN)
    # Option 1 - embed-file with every single file
    foreach (res IN LISTS DATA_FILES)
        get_filename_component(res_name "${res}" NAME)
        get_filename_component(res_dir "${res}" DIRECTORY)
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" RELPATH ${res_dir})
        target_link_options(sdl3-demo PRIVATE "SHELL:--embed-file \"${res}@${RELPATH}/${res_name}\"")
    endforeach ()
    # Option 2 - preload-file - concatenate all files into a single Blob
    #    target_link_options(sdl3-demo PRIVATE --preload-file "${CMAKE_CURRENT_SOURCE_DIR}/data@data")
    set_property(TARGET sdl3-demo PROPERTY SUFFIX ".html")
    target_link_options(sdl3-demo PRIVATE -sALLOW_MEMORY_GROWTH=1)
    target_link_options(sdl3-demo PRIVATE --shell-file "${PROJECT_SOURCE_DIR}/emscripten/shell.html")
endif ()
