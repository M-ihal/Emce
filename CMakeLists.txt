cmake_minimum_required(VERSION 3.27.5)

project(emce)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}) # All compiled things will be placed in the same folder, so don't need to copy .dlls and stuff

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER cmake_predefined_targets)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
set_property(GLOBAL PROPERTY CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)

add_subdirectory(external/glew/build/cmake)
add_subdirectory(external/SDL2)

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
    source/camera.cpp
    source/camera.h
    source/utils.cpp
    source/utils.h
)

set(source_files_external
)

add_definitions(-DGLEW_STATIC)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${source_files_external})

add_executable(emce ${source_files} ${source_files_external})

target_include_directories(emce
    PUBLIC source
    PUBLIC external
    PUBLIC source/graphics
    PUBLIC external/SDL2/include
    PUBLIC external/glew/include/GL
)

target_link_libraries(emce
    PUBLIC SDL2main
    PUBLIC SDL2
    PUBLIC glew_s
)
