cmake_minimum_required(VERSION 3.27.5)

project(emce)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "-g2")
set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake_predefined_targets)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)

add_subdirectory(external/glew/build/cmake)
add_subdirectory(external/SDL)

set(source_files
	source/main.cpp
    source/window.cpp
    source/window.h
    source/input.cpp
    source/input.h
    source/math_types.cpp
    source/math_types.h
    source/graphics/shader.cpp
    source/graphics/shader.h
    source/graphics/vertex_array.cpp
    source/graphics/vertex_array.h
    source/graphics/texture.cpp
    source/graphics/texture.h
    source/graphics/cubemap.cpp
    source/graphics/cubemap.h
    source/graphics/font.cpp
    source/graphics/font.h
    source/graphics/simple_draw.cpp
    source/graphics/simple_draw.h
    source/graphics/text_batcher.cpp
    source/graphics/text_batcher.h
    source/graphics/framebuffer.cpp
    source/graphics/framebuffer.h
    source/graphics/texture_array.cpp
    source/graphics/texture_array.h
    source/camera.cpp
    source/camera.h
    source/utils.cpp
    source/utils.h
    source/chunk.cpp
    source/chunk.h
    source/world.cpp
    source/world.h
    source/console.cpp
    source/console.h
    source/game.cpp
    source/game.h
    source/player.cpp
    source/player.h
)

set(source_files_external
)

add_definitions(-DBUILD_TYPE=\"${CMAKE_BUILD_TYPE}\")

add_definitions(-DGLEW_STATIC)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files_external})

add_executable(emce ${source_files} ${source_files_external})

target_include_directories(emce
    PUBLIC ${PROJECT_SOURCE_DIR}/external/string_viu
    PUBLIC ${PROJECT_SOURCE_DIR}/source
    PUBLIC ${PROJECT_SOURCE_DIR}/source/graphics
    PUBLIC ${PROJECT_SOURCE_DIR}/external/SDL/include
    PUBLIC ${PROJECT_SOURCE_DIR}/external/glew/include/GL
	PUBLIC ${PROJECT_SOURCE_DIR}/external/stb
    PUBLIC ${PROJECT_SOURCE_DIR}/external/meh_hash
    PUBLIC ${PROJECT_SOURCE_DIR}/external/meh_dynamic_array
    PUBLIC ${PROJECT_SOURCE_DIR}/external/PerlinNoise
)

target_link_libraries(emce
    PRIVATE SDL3::SDL3
    PUBLIC glew_s
)
