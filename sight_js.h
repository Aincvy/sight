//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.
#pragma once

#include <future>

#include "sight.h"
#include "shared_queue.h"

namespace sight{

    enum class JsCommandType {
        JsCommandHolder,
        // run a file
        File,
        // destroy js engine
        Destroy,

    };

    struct JsCommand {
        JsCommandType type = JsCommandType::JsCommandHolder;
        struct CommandArgs args;
    };

    /**
     *
     * @param type
     * @param argInt
     * @return
     */
    int addJsCommand(JsCommandType type, int argInt = 0);

    /**
     *
     * @param type
     * @param argString
     * @param argStringLength
     * @param argStringNeedFree
     * @return
     */
    int addJsCommand(JsCommandType type,const char *argString, int argStringLength = 0, bool argStringNeedFree = false, std::promise<int>* promise = nullptr);

    /**
     *
     * @param command
     * @return
     */
    int addJsCommand(JsCommand &command);

    /**
     * js thread run function.
     */
    void jsThreadRun(const char * exeName);

}

