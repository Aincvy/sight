//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.
#pragma once

#include "shared_queue.h"

namespace sight{

    enum JsCommandType {
        None,
        // run a file
        File,
        // destroy js engine
        Destroy,

    };

    struct JsCommand {
        JsCommandType type = JsCommandType::None;

        // string arg.
        const char * argString{};
        int argStringLength{};
        bool argStringNeedFree = false;

        int argInt{};

        /**
         * dispose command
         */
        void dispose(){

        }
    };

    void testJs(char *arg1);

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
    int addJsCommand(JsCommandType type,const char *argString, int argStringLength = 0, bool argStringNeedFree = false);

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
