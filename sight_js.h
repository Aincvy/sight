//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.
#pragma once

#include <future>

#include "sight.h"
#include "shared_queue.h"

namespace sight{

    struct SightJsNode;

    enum class JsCommandType {
        JsCommandHolder,
        // run a file and flush node cache
        File,
        // just run a js file.
        RunFile,
        // flush node cache to ui thread.
        FlushNodeCache,
        // destroy js engine
        Destroy,
        //
        ParseGraph,
        InitPluginManager,
        InitParser,
        // this command will nofity ui thread, js thread is init end.
        // do not add this command twice.
        EndInit,

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

    /**
     * run a js file. | only call this function from js thread.
     * @param filepath
     * @return
     */
    int runJsFile(const char *filepath, std::promise<int>* promise = nullptr);

    /**
     * only call this function from js thread.
     * todo a sync flag with ui thread.
     * @param promise
     */
    void flushJsNodeCache(std::promise<int>* promise = nullptr);

    /**
     *
     */
    void clearJsNodeCache();

    /**
     * @brief convert SightJsNode to `addTemplateNode` call
     * todo 
     * @param node 
     * @return std::string 
     */
    std::string serializeJsNode(SightJsNode const& node);

}

