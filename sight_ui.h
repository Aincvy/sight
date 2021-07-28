// Main window ui
//
#pragma once
#include "imgui.h"
#include "uv.h"

#include "sight.h"

namespace sight {

    enum class UICommandType{
        UICommandHolder,

        // common ui part
        COMMON = 100,

        // node editor part
        AddNode = 200,
    };

    struct UICommand {
        UICommandType type = UICommandType::UICommandHolder;

        CommandArgs args;
    };

    struct UIStatus {
        bool needInit = false;
        const ImGuiIO &io;
        bool closeWindow = false;


        uv_loop_t *uvLoop = nullptr;
        uv_async_t* uvAsync = nullptr;
    };

    /**
     * init window function.
     * @return
     */
    int showMainWindow();

    /**
     * add a ui command
     * @param type
     * @param argInt
     */
    void addUICommand(UICommandType type, int argInt = 0);

    /**
     *
     * @param type
     * @param data
     * @param needFree
     * @return
     */
    int addUICommand(UICommandType type, void *data, size_t dataLength = 0, bool needFree = true);

    /**
     *
     * @param command
     * @return
     */
    int addUICommand(UICommand & command);

}
