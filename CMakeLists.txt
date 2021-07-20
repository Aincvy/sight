cmake_minimum_required(VERSION 3.10)
SET(CMAKE_MODULE_PATH APPEND "${CMAKE_SOURCE_DIR}/cmake/" )

# set the project name
project(sight)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE DEBUG)

add_executable(sight program.cpp)

# source
target_include_directories(
        sight PRIVATE
        ${PROJECT_SOURCE_DIR}
)
target_sources(sight PRIVATE
        sight_ui.cpp
        sight_node_editor.cpp
        sight_js.cpp
)

# depend
target_include_directories(
    sight PRIVATE
        dependencies/imgui
        dependencies/imgui/backends
        dependencies/imgui/examples/libs/gl3w
)

target_sources(sight PRIVATE
        dependencies/imgui/imgui.cpp
        dependencies/imgui/imgui_draw.cpp
        dependencies/imgui/imgui_widgets.cpp
        dependencies/imgui/imgui_tables.cpp
        dependencies/imgui/backends/imgui_impl_glfw.cpp
        dependencies/imgui/backends/imgui_impl_opengl3.cpp
        dependencies/imgui/examples/libs/gl3w/GL/gl3w.c
        )

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
find_package(GLFW3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_LIBRARY}")

# V8
SET(V8_DIR "/usr/local/opt/v8/libexec/")
find_package(V8 REQUIRED)

target_include_directories(sight PRIVATE ${V8_INCLUDE_DIR})
add_definitions(-DV8_COMPRESS_POINTERS -DV8_31BIT_SMIS_ON_64BIT_ARCH)

SET(EXTRA_LIBS glfw OpenGL::GL ${V8_LIBRARIES})
target_link_libraries(sight PRIVATE ${EXTRA_LIBS})
target_link_libraries(sight PRIVATE "-framework IOKit" "-framework CoreVideo" "-framework Cocoa")