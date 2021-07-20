cmake_minimum_required(VERSION 3.10)

# set the project name
project(sight)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(sight program.cpp)

# source
target_include_directories(
        sight PRIVATE
        ${PROJECT_SOURCE_DIR}
)
target_sources(sight PRIVATE
        sight_ui.cpp)

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

#
#find_library(COCOA_LIBRARY REQUIRED Cocoa)

SET(EXTRA_LIBS glfw OpenGL::GL)

#target_link_libraries(sight PRIVATE glfw)
#target_link_libraries(sight PRIVATE OpenGL::GL)
target_link_libraries(sight PRIVATE ${EXTRA_LIBS})

target_link_libraries(sight PRIVATE "-framework IOKit" "-framework CoreVideo" "-framework Cocoa")