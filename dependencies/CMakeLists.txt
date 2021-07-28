# build dependencies ..

# imgui
MESSAGE(STATUS "glfw: ${GLFW3_INCLUDE}")

SET(
        IMGUI_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/backends/imgui_impl_opengl3.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui/examples/libs/gl3w/GL/gl3w.c
)

add_library(imgui SHARED ${IMGUI_SOURCES})

# OpenGL
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 REQUIRED)
message(STATUS "Found GLFW3 in ${GLFW3_LIBRARY}")

target_include_directories(imgui PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/imgui
        ${CMAKE_CURRENT_LIST_DIR}/imgui/backends
        ${CMAKE_CURRENT_LIST_DIR}/imgui/examples/libs/gl3w
)

target_link_libraries(
        imgui PUBLIC
        glfw
        OpenGL::GL
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Cocoa"
)


# imgui-node-editor

SET(
        IMGUI_NODE_EDITOR_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/crude_json.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_bezier_math.inl
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_canvas.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_extra_math.inl
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_node_editor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_node_editor_api.cpp
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor/imgui_node_editor_internal.inl
)

add_library(imgui_node_editor SHARED ${IMGUI_NODE_EDITOR_SOURCES})

target_include_directories(
        imgui_node_editor PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/imgui-node-editor
)

target_link_libraries(
        imgui_node_editor PRIVATE
        imgui
)


# sight-util

SET(
        SIGHT_UTIL_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_log.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_queue.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/shared_queue.cpp
)

add_library(sight-util SHARED ${SIGHT_UTIL_SOURCES})

target_include_directories(
        sight-util PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/sight-util
        ${CMAKE_CURRENT_LIST_DIR}/dbg-macro
)