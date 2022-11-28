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
)

# macos  
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(
                imgui PUBLIC
                "-framework IOKit"
                "-framework CoreVideo"
                "-framework Cocoa"
                "-framework AppKit"
        )
endif()

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
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_language.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_address.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_util.cpp
        ${CMAKE_CURRENT_LIST_DIR}/sight-util/sight_memory.cpp
)

add_library(sight-util SHARED ${SIGHT_UTIL_SOURCES})

target_include_directories(
        sight-util PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/sight-util
)

# macro
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "in debug mode, add defines")
    target_compile_definitions(sight-util PUBLIC SIGHT_DEBUG)
ELSE()
    message(STATUS "in release mode, add defines")
    target_compile_definitions(sight-util PUBLIC SIGHT_RELEASE)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

# abseil
find_package(absl REQUIRED)
target_link_libraries(
        sight-util PUBLIC
        absl::base
        absl::strings
        absl::hash
        absl::flat_hash_map
        absl::flat_hash_set
        absl::btree       
)

# nativefiledialog-extended
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nativefiledialog-extended)
target_link_libraries(sight-util PUBLIC nfd)

