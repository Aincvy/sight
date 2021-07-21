// Main window ui
//
#pragma once
#include "imgui.h"

namespace sight {

    struct UIStatus {
        bool needInit = false;
        const ImGuiIO &io;
    };

    /**
     * init window function.
     * @return
     */
    int showMainWindow();


}
