cmake_minimum_required(VERSION 3.10)
SET(CMAKE_MODULE_PATH APPEND "${CMAKE_SOURCE_DIR}/cmake/" )

# set the project name
project(sight)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILECOMMANDS ON)

add_executable(sight program.cpp)

# source
target_include_directories(
        sight PRIVATE
        ${PROJECT_SOURCE_DIR}
)
target_sources(sight PRIVATE
        sight_ui.cpp
        sight_ui_node_editor.cpp
        sight_ui_project.cpp
        sight_nodes.cpp
        sight_js.cpp
        sight_js_parser.cpp
        sight_project.cpp
        sight.cpp
        sight_plugin.cpp
        sight_widgets.cpp
        sight_keybindings.cpp
        sight_external_widgets.cpp
        sight_network.cpp
        sight_undo.cpp
        sight_object.cpp
        sight_terminal.cpp
        sight_colors.cpp
        sight_code_set.cpp
        # dependencies/imgui/imgui_demo.cpp           # for debug
)

# depend
target_include_directories(
    sight PRIVATE
        dependencies/v8pp
        dependencies/stb
        dependencies/IconFontCppHeaders
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

# libuv
find_package(LibUV REQUIRED)
MESSAGE(STATUS "LIBUV_INCLUDE_DIRS: ${LIBUV_INCLUDE_DIRS}")
MESSAGE(STATUS "LIBUV_LIBRARIES: ${LIBUV_LIBRARIES}")
target_include_directories(sight PRIVATE ${LIBUV_INCLUDE_DIRS})

# imgui and others library.
INCLUDE(dependencies/CMakeLists.txt)

# nativefiledialog-extended
# SET(NFD_LIBRARIES "nfd")

SET(EXTRA_LIBS ${V8_LIBRARIES} ${TREE_SITTER_LIBRARY} ${LIBUV_LIBRARIES} imgui imgui_node_editor sight-util)
target_link_libraries(sight PRIVATE ${EXTRA_LIBS})

# backward-cpp
target_include_directories(sight PRIVATE dependencies/backward-cpp)

# ImTerm
target_include_directories(sight PRIVATE dependencies/ImTerm/include)

# yaml-cpp
find_package(yaml-cpp REQUIRED)
target_include_directories(sight PRIVATE YAML_CPP_INCLUDE_DIR)
target_link_libraries(sight PRIVATE ${YAML_CPP_LIBRARIES})
MESSAGE(STATUS "Found yaml-cpp at: ${YAML_CPP_INCLUDE_DIR}")

# custom commands 
add_custom_target(copyFiles)
add_custom_command(TARGET copyFiles PRE_BUILD
        COMMAND echo "copy plugins folder ..." && cp -fR "${CMAKE_CURRENT_LIST_DIR}/plugins" "${CMAKE_BINARY_DIR}"
        COMMAND echo "copy config files ..." && cp -fR "${CMAKE_CURRENT_LIST_DIR}/keybindings.yaml" "${CMAKE_BINARY_DIR}"
        )
