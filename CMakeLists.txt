cmake_minimum_required(VERSION 3.10)
SET(CMAKE_MODULE_PATH APPEND "${CMAKE_SOURCE_DIR}/cmake/" )

# set the project name
project(sight)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_EXPORT_COMPILECOMMANDS ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set( CMAKE_VERBOSE_MAKEFILE ON )

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

target_compile_definitions(sight PRIVATE V8PP_HEADER_ONLY=1)

target_sources(sight PRIVATE
        dependencies/tree-sitter-javascript/src/parser.c
        dependencies/tree-sitter-javascript/src/scanner.c
        )



if(WIN32)
        MESSAGE(STATUS "WIN32 Env")
        target_compile_definitions(sight PRIVATE DirectX11)

        target_sources(sight PRIVATE sight_render_directx11.cpp)

        # v8
        SET(V8_DIR "F:/Library/v8")


        # V8_INCLUDE_DIR
        SET(V8_INCLUDE_DIR "${V8_DIR}/include")

        # V8_LIBRARIES
        SET(V8_LIBRARIES
                # "${V8_DIR}/lib/Release/third_party_abseil-cpp_absl.dll.lib"
                # "${V8_DIR}/lib/Release/third_party_icu_icui18n.dll.lib"
                # "${V8_DIR}/lib/Release/third_party_zlib.dll.lib"
                "${V8_DIR}/lib/Release/v8_libbase.dll.lib"
                "${V8_DIR}/lib/Release/v8_libplatform.dll.lib"
                "${V8_DIR}/lib/Release/v8.dll.lib"
        )
        
        # libuv
        SET(LIBUV_VIR "F:/Library/libuv")
        SET(LIBUV_INCLUDE_DIRS "${LIBUV_VIR}/include")
        SET(LIBUV_LIBRARIES 
                "${LIBUV_VIR}/lib/x64/Release/libuv.lib"
        )

        # tree-sitter
        # SET(TREE_SITTER_LIBRARY "F:\\Library\\Other\\libtree-sitter-cpp.dll")
        SET(TREE_SITTER_LIBRARY "F:\\Library\\tree-sitter\\lib\\build\\\\Debug\\tree-sitter.lib")
        SET(TREE_SITTER_INCLUDE_DIR "F:\\Library\\tree-sitter\\lib\\include")
        target_include_directories(sight PRIVATE ${TREE_SITTER_INCLUDE_DIR})

        # yaml-cpp
        SET(yaml-cpp-dir "F:/Library/yaml-cpp")
        SET(YAML_CPP_INCLUDE_DIR "${yaml-cpp-dir}/include")
        
        # 设置 Debug 模式下的库文件
        SET(YAML_CPP_DEBUG_LIB "${yaml-cpp-dir}/lib/yaml-cppd.lib")

        # 设置 Release 模式下的库文件
        SET(YAML_CPP_RELEASE_LIB "${yaml-cpp-dir}/lib/yaml-cpp.lib")

        IF(CMAKE_BUILD_TYPE MATCHES "Debug")
                SET(YAML_CPP_LIBRARIES ${YAML_CPP_DEBUG_LIB})
        ELSE()
                SET(YAML_CPP_LIBRARIES ${YAML_CPP_RELEASE_LIB})
        ENDIF()
        
else()
  target_compile_definitions(sight PRIVATE OPENGL NOT_WIN32)
  target_sources(sight PRIVATE sight_render_opengl.cpp)

  # V8
  SET(V8_DIR "/opt/v8")
  find_package(V8 REQUIRED)

  # libuv
  find_package(LibUV REQUIRED)

  # yaml-cpp
  find_package(yaml-cpp REQUIRED)

  # tree-sitter
  find_library(TREE_SITTER_LIBRARY "tree-sitter" HINTS "/usr/local/lib")
endif()

target_include_directories(sight PRIVATE ${V8_INCLUDE_DIR})
target_compile_definitions(sight PRIVATE V8_COMPRESS_POINTERS V8_31BIT_SMIS_ON_64BIT_ARCH)

# tree-sitter

MESSAGE(STATUS "TREE_SITTER_LIBRARY: ${TREE_SITTER_LIBRARY}")

# libuv
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

target_include_directories(sight PRIVATE ${YAML_CPP_INCLUDE_DIR})
target_link_libraries(sight PRIVATE ${YAML_CPP_LIBRARIES})
MESSAGE(STATUS "Found yaml-cpp at: ${YAML_CPP_INCLUDE_DIR}")

# custom commands 
add_custom_target(copyFiles)
add_custom_command(TARGET copyFiles PRE_BUILD
        COMMAND echo "copy plugins folder ..." && cp -fR "${CMAKE_CURRENT_LIST_DIR}/plugins" "${CMAKE_BINARY_DIR}"
        COMMAND echo "copy config files ..." && cp -fR "${CMAKE_CURRENT_LIST_DIR}/keybindings.yaml" "${CMAKE_BINARY_DIR}"
        )
