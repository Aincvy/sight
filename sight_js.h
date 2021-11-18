//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
// Js engine and something else.
#pragma once

#include <future>
#include <map>
#include <vector>
#include <string>

#include "sight.h"
#include "shared_queue.h"
#include "sight_node_editor.h"

#include "v8.h"
#include "v8pp/module.hpp"

#define MODULE_INIT_JS 0
#define MODULE_INIT_UI 1

namespace sight{

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

        ProjectBuild,
        ProjectClean,
        ProjectRebuild,

    };

    struct JsCommand {
        JsCommandType type = JsCommandType::JsCommandHolder;
        struct CommandArgs args;
    };

    /**
     * @brief js wrapper
     * 
     */
    struct SightNodePortWrapper {
        SightNodePortHandle portHandle;

        SightNodePortWrapper() = delete;
        SightNodePortWrapper(SightNodePortHandle portHandle);
        ~SightNodePortWrapper();

        uint getId() const;
        const char* getName() const;

        void deleteLinks();

        bool isShow() const;
        void setShow(bool v);

        const char* getErrorMsg() const;
        void setErrorMsg(const char* msg);

        bool isReadonly() const;
        void setReadonly(bool v);

        uint getType() const;
        void setType(uint v);

        void resetType();

        void updatePointer();

        int getKind() const;

    private:
        SightNodePort* pointer = nullptr;

    };

    /**
     * @brief contains generate-related info
     * 
     */
    struct SightNodeGenerateInfo  {
        // the times of generate code function called .
        u_char generateCodeCount = 0;

        SightNodeGenerateInfo() = default;
        ~SightNodeGenerateInfo() = default;

        /**
         * @brief 
         * 
         * @return true 
         * @return false 
         */
        bool hasGenerated() const;
    };

    /**
     * @brief js wrapper | only for read data now.
     * 
     */
    struct SightNodeGraphWrapper {

        SightNodeGraphWrapper() = delete;
        SightNodeGraphWrapper(SightNodeGraph* graph);
        ~SightNodeGraphWrapper();

        void buildNodeCache();

        std::vector<SightNode>& getCachedNodes();

        SightNode findNodeWithId(uint id);

        /**
         * @brief find first 
         * 
         * @param templateAddress 
         * @param filter 
         * @return SightNode 
         */
        SightNode findNodeWithFilter(std::string const& templateAddress, v8::Local<v8::Function> filter);

        /**
         * @brief 
         * 
         * @param nodeId 
         * @param f    node data change function
         * @return true function is executed.
         * @return false function isn't executed.
         */
        bool updateNodeData(uint nodeId, v8::Local<v8::Function> f);

    private:
        SightNodeGraph* graph = nullptr;

        std::vector<SightNode> cachedNodes; 
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
    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise = nullptr, v8::Local<v8::Object> module = {}, v8::Local<v8::Value>* resultOuter = nullptr);

    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise = nullptr, v8::Local<v8::Value>* resultOuter = nullptr);

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

    int parseGraph(const char* filename, std::string& source);

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
    
    SightJsNode& registerEntityFunctions(SightJsNode& node);

    inline bool isValid(v8::Local<v8::Value> value){
        return !value.IsEmpty() && !value->IsNullOrUndefined();
    }

    inline bool isValid(v8::MaybeLocal<v8::Value> value){
        return !value.IsEmpty() && isValid(value.ToLocalChecked());
    }
}

