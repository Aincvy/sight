//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.
#pragma once

#include <future>
#include <map>

#include "sight.h"
#include "shared_queue.h"

#include "v8.h"
#include "v8pp/module.hpp"

#define MODULE_INIT_JS 0
#define MODULE_INIT_UI 1

namespace sight{

    struct SightJsNode;
    struct SightNode;
    struct SightNodePort;
    struct SightNodePortHandle;
    struct SightNodeConnection;
    union SightNodeValue;

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
        Test,
        // try to reload a plugin
        PluginReload,

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
    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise = nullptr, v8::Local<v8::Object> module = {});


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

    /**
     * @brief checkTinyData(), tinyData()
     * 
     * @param isolate 
     * @param context 
     */
    void bindTinyData(v8::Isolate* isolate, const v8::Local<v8::Context>& context);

    void bindBaseFunctions(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8pp::module & module);

    void bindNodeTypes(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8pp::module& module);

    v8::Isolate* getJsIsolate();

    v8::Local<v8::Value> getPortValue(v8::Isolate* isolate, int type, SightNodeValue const& value);

    v8::Local<v8::Value> nodePortValue(v8::Isolate* isolate, SightNodePortHandle portHandle);

    v8::Local<v8::Value> nodePortValue(v8::Isolate* isolate, SightNode* node, const char* portName);

    v8::Local<v8::Value> connectionValue(v8::Isolate* isolate, SightNodeConnection* connection, SightNodePort* selfNodePort = nullptr);

    v8::Local<v8::Function> recompileFunction(v8::Isolate* isolate, std::string sourceCode);

    void registerToGlobal(v8::Isolate* isolate, std::map<std::string, std::string>* map);

    void registerToGlobal(v8::Isolate* isolate, v8::Local<v8::Value> object);

    void registerToGlobal(v8::Isolate* isolate, v8::Local<v8::Object> object, std::map<std::string, std::string>* outFunctionCode = nullptr);

    void registerGlobals(v8::Local<v8::Value> value);

    inline bool isValid(v8::Local<v8::Value> value){
        return !value.IsEmpty() && !value->IsNullOrUndefined();
    }

    inline bool isValid(v8::MaybeLocal<v8::Value> value){
        return !value.IsEmpty() && isValid(value.ToLocalChecked());
    }
}

