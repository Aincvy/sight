#pragma once

#include "sight_defines.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <functional>
#include <uv.h>

namespace sight {

    int initWindowBackend();

    void* initWindow(const char* title, int width, int height, std::function<void(ImGuiIO&)> initImgui);

    void mainLoopWindow(void* window, uv_loop_t* uvLoop, bool& exitFlag,
                        std::function<int()> beforeRenderFunc, std::function<void()> render_func);

    void cleanUpWindow(void* window);

    void terminateBackend();
    
}