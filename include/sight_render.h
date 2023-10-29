#pragma once

#include "sight_defines.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <functional>

namespace sight {

    int initWindowBackend();

    void* initWindow(const char* title, int width, int height, std::function<void(ImGuiIO&)> initImgui, bool noTitleBar = false);

    void mainLoopWindow(void* window,  bool& exitFlag, bool mainWindow,
                        std::function<int()> beforeRenderFunc, std::function<void()> render_func);

    void cleanUpWindow(void* window);

    void terminateBackend();
    
}