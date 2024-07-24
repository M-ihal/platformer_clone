cmake_minimum_required(VERSION 3.18.0)
project(no_name)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff

set(enable_debug_mode false)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake_predefined_targets)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)

set(exe_source_files
    source/main.cpp
    source/input.cpp
    source/maths.cpp
    source/opengl_abs.cpp
    source/renderer.cpp
    source/audio_player.cpp
    source/game.cpp
    source/data.cpp
    source/level.cpp
    source/entity.cpp
    source/entities/player.cpp
    source/entities/goomba.cpp
    source/entities/koopa.cpp
    source/entities/piranha.cpp
    source/entities/tilemap.cpp
    source/entities/power_up.cpp
    source/entities/camera_region.cpp
    source/entities/background.cpp
    source/entities/fire_bar.cpp
    source/entities/level_entities.cpp
    source/entities/moving_platform.cpp
    source/editor.cpp
    source/save_data.cpp
    source/level_transition.cpp
    source/main_menu.cpp
    source/entities/bowser.cpp

    source/common.h
    source/input.h
    source/maths.h
    source/opengl_abs.h
    source/renderer.h
    source/audio_player.h
    source/game.h
    source/data.h
    source/level.h
    source/entity.h
    source/all_entities.h
    source/entities/player.h
    source/entities/goomba.h
    source/entities/koopa.h
    source/entities/piranha.h
    source/entities/tilemap.h
    source/entities/power_up.h
    source/entities/camera_region.h
    source/entities/background.h
    source/entities/fire_bar.h
    source/entities/level_entities.h
    source/entities/moving_platform.h
    source/editor.h
    source/save_data.h
    source/level_transition.h
    source/main_menu.h
    source/entities/bowser.h
)

set(external_source_files
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp

    external/stb/stb_image.h
    external/stb/stb_rect_pack.h
    external/stb/stb_truetype.h
)

if(${enable_debug_mode}) 
add_executable(no_name ${exe_source_files} ${external_source_files})
else()
add_executable(no_name WIN32 ${exe_source_files} ${external_source_files})
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${external_source_files})

add_executable(no_name_editor ${exe_source_files} ${external_source_files})
target_compile_definitions(no_name_editor PUBLIC -DBUILD_EDITOR)

if(${enable_debug_mode}) 
add_definitions(-D_DEBUG_MODE)
endif()

# https://github.com/Perlmint/glew-cmake
add_definitions(-DGLEW_STATIC)
add_subdirectory(external/glew/build/cmake)

# https://github.com/libsdl-org/SDL/releases/tag/release-2.24.0 / Source code (.zip)
add_subdirectory(external/SDL2)
add_subdirectory(external/SDL_mixer)

target_link_libraries(no_name
    PUBLIC SDL2
    PUBLIC SDL2main
    PUBLIC SDL2_mixer
    PUBLIC glew_s
)

target_include_directories(no_name
    PUBLIC source

    PUBLIC external/glew/include
    PUBLIC external/SDL2/include
    PUBLIC external/imgui
    PUBLIC external/imgui/backends
    PUBLIC external/stb
    PUBLIC external/SDL_mixer/include
)

target_link_libraries(no_name_editor
    PUBLIC SDL2
    PUBLIC SDL2main
    PUBLIC SDL2_mixer
    PUBLIC glew_s
)

target_include_directories(no_name_editor
    PUBLIC source

    PUBLIC external/glew/include
    PUBLIC external/SDL2/include
    PUBLIC external/imgui
    PUBLIC external/imgui/backends
    PUBLIC external/stb
    PUBLIC external/SDL_mixer/include
)