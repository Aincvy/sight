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
        sight_js_parser.cpp
        sight_project.cpp
        sight.cpp
)

# depend
target_include_directories(
    sight PRIVATE
        dependencies/v8pp/v8pp
)

target_sources(sight PRIVATE
        dependencies/tree-sitter-javascript/src/parser.c
        dependencies/tree-sitter-javascript/src/scanner.c
        )

# V8
SET(V8_DIR "/usr/local/opt/v8/libexec/")
find_package(V8 REQUIRED)
target_include_directories(sight PRIVATE ${V8_INCLUDE_DIR})
target_compile_definitions(sight PRIVATE V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)

# tree-sitter
find_library(TREE_SITTER_LIBRARY "tree-sitter" HINTS "/usr/local/lib")
MESSAGE(STATUS "TREE_SITTER_LIBRARY: ${TREE_SITTER_LIBRARY}")

# imgui
INCLUDE(dependencies/CMakeLists.txt)

SET(EXTRA_LIBS ${V8_LIBRARIES} ${TREE_SITTER_LIBRARY} imgui imgui_node_editor)
target_link_libraries(sight PRIVATE ${EXTRA_LIBS})