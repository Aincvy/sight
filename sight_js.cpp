//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <map>
#include <ostream>
#include <stdio.h>
#include <sstream>
#include "IconsMaterialDesign.h"
#include "functional"
#include <string>
#include <string_view>
#include <sys/cdefs.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <thread>
#include <chrono>
#include <vector>

#include "sight.h"
#include "sight_defines.h"
#include "sight_js.h"
#include "shared_queue.h"
#include "sight_log.h"
#include "sight_nodes.h"
#include "sight_ui.h"
#include "sight_util.h"
#include "sight_js_parser.h"
#include "sight_project.h"
#include "sight_plugin.h"
#include "sight_code_set.h"

#include "v8.h"
#include "libplatform/libplatform.h"

#include "v8pp/call_v8.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include "v8pp/module.hpp"
#include "v8pp/class.hpp"
#include "v8pp/property.hpp"
#include "v8pp/object.hpp"
#include "v8pp/json.hpp"

#ifdef __linux__
#define isnumber isdigit
#endif

#define GENERATE_CODE_DETAILS 1

using namespace v8;

using PersistentFunction = Persistent<Function, CopyablePersistentTraits<Function>>;

namespace sight {
    namespace {
        struct V8Runtime;
    }
}
static sight::V8Runtime *g_V8Runtime = nullptr;

// cache for js script, it will send to ui thread once
static std::vector<sight::SightNode> g_NodeCache;
//
static std::vector<sight::SightNodeTemplateAddress> g_TemplateNodeCache;

namespace sight {

    std::string runGenerateFunction(PersistentFunction const& persistent, Isolate* isolate, SightNode* node, int reverseActivePort = -1);

    /**
     *
     * @from https://github.com/danbev/learning-v8/blob/master/run-script.cc
     * @param isolate
     * @param name
     * @return
     */
    v8::MaybeLocal<v8::String> readFile(v8::Isolate *isolate, const char *name) {
        FILE *file = fopen(name, "rb");
        if (file == NULL) {
            return {};
        }

        fseek(file, 0, SEEK_END);
        size_t size = ftell(file);
        rewind(file);

        char *chars = new char[size + 1];
        chars[size] = '\0';
        for (size_t i = 0; i < size;) {
            i += fread(&chars[i], 1, size - i, file);
            if (ferror(file)) {
                fclose(file);
                return {};
            }
        }
        fclose(file);
        v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(isolate,
                                                                    chars,
                                                                    v8::NewStringType::kNormal,
                                                                    static_cast<int>(size));
        delete[] chars;
        return result;
    }

    int kindString(std::string_view str){
        int kind = -1;
        if (str == "input") {
            kind = static_cast<int>(NodePortType::Input);
        } else if (str == "output") {
            kind = static_cast<int>(NodePortType::Output);
        } else if (str == "both") {
            kind = static_cast<int>(NodePortType::Both);
        } else if (str == "field") {
            kind = static_cast<int>(NodePortType::Field);
        }
        return kind;
    }

    Local<Value> getPortValue(Isolate* isolate, int type, SightNodeValue const& value) {
        switch (type) {
        case IntTypeProcess:
            break;
        case IntTypeFloat:
            return v8pp::to_v8(isolate, value.u.f);
        case IntTypeDouble:
            return v8pp::to_v8(isolate, value.u.d);
        case IntTypeString:
            return v8pp::to_v8(isolate, (const char*)value.u.string);
        case IntTypeInt:
        case IntTypeLong:
            return v8pp::to_v8(isolate, value.u.i);
        case IntTypeBool:
            return v8pp::to_v8(isolate, value.u.b);
        case IntTypeLargeString:
            return v8pp::to_v8(isolate, (const char*)value.u.largeString.pointer);
        default:
        {
            if (!isBuiltInType(type)) {
                auto [typeInfo, find] = currentProject()->findTypeInfo(type);
                if (find) {
                    auto& render = typeInfo.render;
                    switch (render.kind) {
                    case TypeInfoRenderKind::Default:
                        break;
                    case TypeInfoRenderKind::ComboBox:
                    {
                        auto object = Object::New(isolate);
                        v8pp::set_const(isolate, object, "index", value.u.i);
                        v8pp::set_const(isolate, object, "name", render.data.comboBox.list->at(value.u.i));
                        return object;
                    }
                    }
                }
            }
            logDebug("unhandled type error: $0", type);
            break;
        }
        }

        return v8::Undefined(isolate);
    }

    bool setPortValue(Isolate* isolate, SightBaseNodePort* port, Local<Value> value) {
        if (!port) {
            return false;
        }

        bool flag = true;
        switch (port->getType()) {
        case IntTypeProcess:
            break;
        case IntTypeFloat:
            port->value.u.f = v8pp::from_v8<float>(isolate, value);
            break;
        case IntTypeDouble:
            port->value.u.d = v8pp::from_v8<double>(isolate, value);
            break;
        case IntTypeString:
        {
            std::string tmp = v8pp::from_v8<std::string>(isolate, value);
            auto size = std::size(port->value.u.string);
            if (tmp.size() >= size) {
                logDebug("Warning!!  string is too large. It will be truncated.");
            }
            snprintf(port->value.u.string, size, "%s", tmp.c_str());
            break;
        }
        case IntTypeInt:
        case IntTypeLong:
            port->value.u.i = v8pp::from_v8<int>(isolate, value);
            break;
        case IntTypeBool:
            port->value.u.b = v8pp::from_v8<bool>(isolate, value);
            break;
        case IntTypeLargeString:
        {
            // todo make a limit for large string ?
            std::string tmp = v8pp::from_v8<std::string>(isolate, value);
            port->value.stringCopy(tmp);
            break;
        }
        default:
            if (isBuiltInType(port->getType())) {
                auto [typeInfo, find] = currentProject()->findTypeInfo(port->getType());
                if (find) {
                    auto& render = typeInfo.render;
                    switch (render.kind) {
                    case TypeInfoRenderKind::ComboBox:
                    {
                        auto& data = render.data.comboBox;
                        if (IS_V8_STRING(value)) {
                            std::string v = v8pp::from_v8<std::string>(isolate, value);
                            auto find = std::find(data.list->begin(), data.list->end(), v);
                            if (find != data.list->end()) {
                                // has one
                                port->value.u.i = static_cast<int>(std::distance(data.list->begin(), find));
                            } else {
                                flag = false;
                            }
                        } else if (IS_V8_NUMBER(value)) {
                            auto selected = v8pp::from_v8<int>(isolate, value);
                            if (selected < 0 || selected >= data.list->size()) {
                                flag = false;
                            } else {
                                port->value.u.i = selected;
                            }
                        }
                        break;
                    }
                    default:
                        flag = false;
                        break;
                    }
                }
            } else {
                flag = false;
            }
            if (!flag) {
                logDebug("unhandled type error: $0", port->getType());
            }
            break;
        }

        return flag;
    }

    Local<Value> nodePortValue(Isolate* isolate, SightNodePortHandle portHandle) {
        if (!portHandle) {
            return Undefined(isolate);
        }

        // auto object = Object::New(isolate);
        auto object = v8pp::class_<SightNodePortWrapper>::import_external(isolate, new SightNodePortWrapper(portHandle));
        auto context = isolate->GetCurrentContext();

        auto get = [portHandle, isolate]() -> Local<Value> {
            auto port = portHandle.get();
            if (!port) {
                return v8::Undefined(isolate);
            }

            // get value
            return getPortValue(isolate, port->getType(), port->value);
        };
        auto set = [portHandle, isolate](Local<Value> arg) {
            return setPortValue(isolate, portHandle.get(), arg);
        };

        object->Set(context, v8pp::to_v8(isolate, "get"), v8pp::wrap_function(isolate, "get", get)).ToChecked();
        object->Set(context, v8pp::to_v8(isolate, "set"), v8pp::wrap_function(isolate, "set", set)).ToChecked();

        return object;
    }

    Local<Value> nodePortValue(Isolate* isolate, SightNode* node, const char* portName) {
        auto portHandle = node->findPort(portName);
        if (!portHandle) {
            return Undefined(isolate);
        }

        return nodePortValue(isolate, portHandle);
    }

    Local<Value> connectionValue(v8::Isolate* isolate, SightNodeConnection* connection, SightNodePort* selfNodePort){
        if (!connection) {
            return v8::Undefined(isolate);
        }

        auto object = Object::New(isolate);
        auto context = isolate->GetCurrentContext();
        auto g = currentGraph();

        auto left =  nodePortValue(isolate, g->findPortHandle(connection->left));
        auto right = nodePortValue(isolate, g->findPortHandle(connection->right));
        v8pp::set_const(isolate, object, "id", connection->connectionId);
        v8pp::set_const(isolate, object, "left", left);
        v8pp::set_const(isolate, object, "right", right);
        if (!selfNodePort) {
        } else {
            if (connection->left == selfNodePort->id) {
                v8pp::set_const(isolate, object, "self", left);
                v8pp::set_const(isolate, object, "target", right);
            } else {
                v8pp::set_const(isolate, object, "self", right);
                v8pp::set_const(isolate, object, "target", left);
            }
        }

        return object;
    }

    namespace {

        struct GenerateOptions {
            bool noCode = false;
            bool appendLineEnd = true;
        };

        /**
         * @brief one parsing link.
         * 
         */
        struct ParsingLink {
            int index = 0;
            std::vector<SightNode*> link;
            std::stringstream sourceStream{};
        };

        struct SightNodeGenerateHelper {
            std::string varName;
            uint nodeId;

            const char* getTemplateNodeName() const;
        };

        struct ParsingGraphData {
            SightNode* currentNode = nullptr;
            SightNodeGraph* graph = nullptr;
            Local<Object> graphObject {};
            std::vector<ParsingLink> list;
            uint lastUsedIndex = 0;
            std::map<uint, SightNodeGenerateInfo> generateInfoMap{};
            std::string connectionCodeTemplate;

            SightNode* component = nullptr;

            struct {
                std::string msg{};
                uint nodeId = 0;
                uint portId = 0;
                bool hasError = false;
            } errorInfo;

            void reset();
            bool empty() const;
            bool hasError() const;

            SightNodeGenerateInfo& getGenerateInfo(uint nodeId);

            void addNewLink(SightNode* node);

            std::stringstream& currentUsedSourceStream();

        };

        void ParsingGraphData::reset() {
            this->currentNode = nullptr;
            this->graph = nullptr;
            this->graphObject = {};
            this->list.clear();
            this->lastUsedIndex = 0;
            this->generateInfoMap.clear();

            errorInfo.hasError = false;
            errorInfo.msg.clear();
            errorInfo.nodeId = errorInfo.portId = 0;
        }

        inline bool sight::ParsingGraphData::hasError() const {
            return errorInfo.hasError;
        }

        inline std::stringstream& sight::ParsingGraphData::currentUsedSourceStream() {
            return this->list[this->lastUsedIndex].sourceStream;
        }

        inline void sight::ParsingGraphData::addNewLink(SightNode* node) {
            list.push_back({});
            auto& back = list.back();
            back.link.push_back(node);
        }

        inline SightNodeGenerateInfo& sight::ParsingGraphData::getGenerateInfo(uint nodeId) {
            return this->generateInfoMap[nodeId];
        }

        inline bool sight::ParsingGraphData::empty() const {
            return list.empty();
        }

        /**
         * @brief Then `generateCodeWork` and `onReverseActive` function.
         * 
         */
        struct SightEntityFunctions{
            ScriptFunctionWrapper::Function generateCodeWork;
            ScriptFunctionWrapper::Function onReverseActive;

            Local<Function> getGenerateCodeWork() const {
                return generateCodeWork.Get(Isolate::GetCurrent());
            }
            void setGenerateCodeWork(Local<Function> f){
                generateCodeWork = ScriptFunctionWrapper::Function(Isolate::GetCurrent(), f);
            }

            Local<Function> getOnReverseActive() const {
                return onReverseActive.Get(Isolate::GetCurrent());
            }
            void setOnReverseActive(Local<Function> f) {
                onReverseActive = ScriptFunctionWrapper::Function(Isolate::GetCurrent(), f);
            }

            void reset(){
                this->generateCodeWork.Reset();
                this->onReverseActive.Reset();
            }
        };

        struct V8Runtime {
            std::unique_ptr<v8::Platform> platform;
            v8::Isolate *isolate = nullptr;
            v8::ArrayBuffer::Allocator *arrayBufferAllocator = nullptr;
            std::unique_ptr<v8::Isolate::Scope> isolateScope;
            SharedQueue<JsCommand> commandQueue;
            ParsingGraphData parsingGraphData;
            SightEntityFunctions entityFunctions;
            // key: name,
            std::map<std::string, CodeTemplateFunc> codeTemplateMap;
            // key: name
            std::map<std::string, CommonOperation> connectionCodeTemplateMap;
        };

        struct GenerateFunctionStatus {
            int paramCount = 0;
            bool shouldEval = false;
            bool need$ = false;
            bool need$$ = false;
        };

        /**
         * @brief `generateCodeWork` and `onReverseActive` function's arg `$$`
         * 
         */
        struct GenerateArg$$ {
            Local<Object> helper;
            Local<Object> component;
            
            void errorReport(const char* msg, uint nodeId, uint portId){
                auto& errorInfo = g_V8Runtime->parsingGraphData.errorInfo;
                errorInfo.msg = msg;
                errorInfo.nodeId = nodeId;
                errorInfo.portId = portId;
                errorInfo.hasError = true;
            }

        };

        struct ProjectWrapper {
            Project* project = nullptr;

            ProjectWrapper() = delete;
            ProjectWrapper(Project* p) : project(p) {

            }

            void parseAllGraphs() const {
                project->parseAllGraphs();
            }
        };

        inline const char* SightNodeGenerateHelper::getTemplateNodeName() const {
            auto& data = g_V8Runtime->parsingGraphData;
            auto node = data.graph->findNode(this->nodeId);
            if (node && node->templateNode) {
                return node->templateNode->nodeName.c_str();
            }
            return "";
        }

        //
        //    some util functions.
        //

        Local<Object> getNodeHelper(SightNode* node, Isolate* isolate) {
            uint nodeId = node->getNodeId();
            auto& map = g_V8Runtime->parsingGraphData.generateInfoMap;
            auto iter = map.find(nodeId);
            if (iter == map.end()) {
                map[nodeId] = {};
            }

            auto& info = map[nodeId];
            if (!info.helper.IsEmpty()) {
                return info.helper;
            }

            // make new one
            SightNodeGenerateHelper* helper = new SightNodeGenerateHelper();
            helper->varName = node->nodeName;
            helper->nodeId = node->getNodeId();
            info.helper = v8pp::class_<SightNodeGenerateHelper>::import_external(isolate, helper);
            return info.helper;
        }
        
        /**
         * @brief same as jsFunctionToString
         * 
         * @return std::string 
         */
        std::string functionProtoToString(Isolate* isolate, Local<Context> context, Local<Function> function){
            if (function.IsEmpty()) {
                return {};
            }
            auto maySource = function->FunctionProtoToString(context);
            if (maySource.IsEmpty()) {
                return {};
            }
            std::string sourceCode = v8pp::from_v8<std::string>(isolate, maySource.ToLocalChecked().As<String>());
            return sourceCode;
        }

        std::string jsFunctionToString(Local<Function> function) {
            auto isolate = g_V8Runtime->isolate;
            auto context = isolate->GetCurrentContext();

            auto toString = function->Get(context, v8pp::to_v8(isolate, "toString")).ToLocalChecked();
            auto toStringFunc = toString.As<Function>();
            auto str = toStringFunc->Call(context, function, 0, nullptr).ToLocalChecked();

            if (IS_V8_STRING(str)) {
                return v8pp::from_v8<std::string>(isolate, str);
            } else {
                logDebug("toString's result not a string");
                return {};
            }
        }

        /**
         * @brief Do not convert object. 
         * It likes js code `String(value)`
         * 
         * @param value 
         * @return std::string 
         */
        std::string jsObjectToString(Local<Value> value, Isolate* isolate){
            auto context = isolate->GetCurrentContext();
            auto code = v8pp::to_v8(isolate, "return String(arg)");
            ScriptCompiler::Source source(code);
            auto arg = v8pp::to_v8(isolate, "arg");
            Local<String> args[] = {arg};
            auto function = ScriptCompiler::CompileFunctionInContext(context, &source, 1, args, 0, nullptr).ToLocalChecked();
            Local<Value> argv[] = {value};
            auto result = function->Call(context, Object::New(isolate), 1, argv).ToLocalChecked();
            return v8pp::from_v8<std::string>(isolate, result);
        }


        //
        //   register functions to v8
        //

        Local<Value> v8GetGenerateInfo(uint id){
            auto & map = g_V8Runtime->parsingGraphData.generateInfoMap;
            auto iter = map.find(id);
            auto isolate = v8::Isolate::GetCurrent();
            if (iter != map.end()) {
                // return v8pp::class_<SightNodeGenerateInfo>::import_external(isolate, new SightNodeGenerateInfo(iter->second));
                return v8pp::class_<SightNodeGenerateInfo>::create_object(isolate, iter->second);
            }
            
            return Undefined(isolate);
        }

        void v8NodePortValue(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());
            auto rValue = args.GetReturnValue();
            if (!node || args.Length() <= 0) {
                rValue.SetUndefined();
                return;
            }

            auto arg1 = args[0];
            if (arg1->IsInt32()) {
                auto id = arg1->Int32Value(isolate->GetCurrentContext()).ToChecked();
                rValue.Set(nodePortValue(isolate, {node, static_cast<uint>(id)}));
            } else if (IS_V8_STRING(arg1)) {
                std::string portName = v8pp::from_v8<std::string>(isolate, arg1);
                rValue.Set(nodePortValue(isolate, node, portName.c_str()));
            } else {
                rValue.SetUndefined();
            }
        }

        void v8NodeTemplateAddress(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());

            const SightNode* t1 = node;
            if (node->templateNode) {
                args.GetReturnValue().Set(v8pp::to_v8(isolate, node->templateNode->fullTemplateAddress.c_str()));
            }
        }

        void v8NodeGetPorts(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());
            auto rValue = args.GetReturnValue();
            if (!node) {
                rValue.SetUndefined();
                return;
            }

            int kind = -1;
            auto context = isolate->GetCurrentContext();
            if (args.Length() > 0) {
                // 
                auto arg1 = args[0];
                if (arg1->IsInt32()) {
                    kind = arg1->Int32Value(context).FromJust();
                }  else if (IS_V8_STRING(arg1)) {
                    auto tmp = kindString(v8pp::from_v8<std::string>(isolate, arg1));
                    if (tmp < 0) {
                        return rValue.SetUndefined();
                    }
                    kind = tmp;
                }
                else {
                    return rValue.SetUndefined();
                }
                
            }
            std::vector<Local<Value>> array;
            array.reserve(node->inputPorts.size() + node->fields.size() + node->outputPorts.size());
            auto nodeFunc = [isolate, kind](std::vector<SightNodePort> const& src, std::vector<Local<Value>> &array) {
                for( const auto& item: src){
                    if (item.portName.empty()) {
                        // ignore title bar ports. 
                        continue;
                    }
                    if (kind > 0) {
                        if (kind == static_cast<int>(NodePortType::Both)) {
                            if (item.kind != NodePortType::Input && item.kind != NodePortType::Output) {
                                break;
                            }
                        } else if (kind != static_cast<int>(item.kind)) {
                            break;
                        }
                    }

                    array.push_back(nodePortValue(isolate, item));
                }
            };

            nodeFunc(node->fields, array);
            nodeFunc(node->inputPorts, array);
            nodeFunc(node->outputPorts, array);
            rValue.Set(v8pp::to_v8(isolate, array.begin(), array.end()));
        }

        Local<Value> v8Include(std::string path, Local<Object> module, int frameOffset = 0) {
            auto isolate = Isolate::GetCurrent();

            if (!startsWith(path, "/")) {
                // Relative path
                auto frame = v8::StackTrace::CurrentStackTrace(isolate, 1 + frameOffset, v8::StackTrace::kScriptName)
                                 ->GetFrame(isolate, 0 + frameOffset);
                if (frame.IsEmpty()) {
                    return v8::Undefined(isolate);
                }
                auto filename = frame->GetScriptName();
                if (filename.IsEmpty()) {
                    logDebug("empty script name, cannot include other script.");
                    return v8::Undefined(isolate);
                }

                auto currentPath = v8pp::from_v8<std::string>(isolate, filename);
                auto scriptPath = std::filesystem::path(currentPath.c_str());

                currentPath = scriptPath.parent_path().c_str();
                appendIfNotExists(currentPath, "/") += path;
                path = currentPath;
            }
            
            // run script
            Local<Value> result;
            if (runJsFile(isolate, path.c_str(), nullptr, module, &result) == CODE_OK) {
                return result;
            }
            return v8::Undefined(isolate);
        }

        void checkTinyData(const char* key, Local<Object> data, Isolate* isolate){
            auto g = currentGraph();
            if (!g) {
                logDebug("need a graph opened!");
                return;
            }

            auto & map = g->getExternalData().tinyDataMap;
            if (!map.contains(key)) {
                // fixme: perhaps g_V8Runtime->isolate should be uiStatus->isolate
                map[key] = SightNodeGraphExternalData::V8ObjectType(isolate, data);
            }
        }

        MaybeLocal<Object> tinyData(const char* key, MaybeLocal<Object> mayData, Isolate* isolate) {
            auto g = currentGraph();
            if (!g) {
                logDebug("need a graph opened!");
                return {};
            }

            auto& map = g->getExternalData().tinyDataMap;

            if (mayData.IsEmpty()) {
                // get data
                if (map.contains(key)) {
                    auto data = map[key].Get(isolate);
                    return {data};
                }
                return {};
            } else {
                // set data
                auto data = mayData.ToLocalChecked();
                if (map.contains(key)) {
                    map[key].Reset();
                }
                map[key] = SightNodeGraphExternalData::V8ObjectType(isolate, data);
                return mayData;
            }
        }

        void v8CheckTinyData(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();
            HandleScope handleScope(isolate);
            auto rValue = args.GetReturnValue();
            if (args.Length() != 2) {
                rValue.Set(-1);
                return;
            }
            auto arg1 = args[0];
            auto arg2 = args[1];
            std::string key;
            if (IS_V8_STRING(arg1)) {
                key = v8pp::from_v8<std::string>(isolate, arg1);
            } else if (arg1->IsObject()) {
                auto node = v8pp::class_<SightNode>::unwrap_object(isolate, arg1);
                if (node && node->templateNode) {
                    // valid object
                    auto t = dynamic_cast<SightJsNode const*>(node->templateNode);
                    if (t) {
                        key = t->fullTemplateAddress;
                    }
                }
            }
            if (key.empty()) {
                rValue.Set(-2);
                return;
            }
            if (!arg2->IsObject()) {
                rValue.Set(-3);
                return;
            }

            auto data = arg2.As<Object>();
            checkTinyData(key.c_str(), data, isolate);
            rValue.Set(0);
        }

        void v8TinyData(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();
            HandleScope handleScope(isolate);
            auto rValue = args.GetReturnValue();
            if (args.Length() <= 0) {
                rValue.Set(-1);
                return;
            }

            auto arg1 = args[0];
            std::string key;
            if (IS_V8_STRING(arg1)) {
                key = v8pp::from_v8<std::string>(isolate, arg1);
            }else if (arg1->IsObject()) {
                auto node = v8pp::class_<SightNode>::unwrap_object(isolate, arg1);
                if (node && node->templateNode) {
                    // valid object
                    auto t = dynamic_cast<SightJsNode const*>(node->templateNode);
                    if (t) {
                        key = t->fullTemplateAddress;
                    }
                }
            }
            if (key.empty()) {
                rValue.Set(-2);
                return;
            }

            MaybeLocal<Object> data = {};
            if (args.Length() >= 2) {
                // set
                auto arg2 = args[1];
                if (!arg2->IsObject()) {
                    rValue.Set(-3);
                    return;
                }

                data = arg2.As<Object>();
            }

            auto tmp = tinyData(key.c_str(), data, isolate);
            if (tmp.IsEmpty()) {
                rValue.SetUndefined();
            } else {
                rValue.Set(tmp.ToLocalChecked());
            }
        }

        int v8InsertSource(const char* source){
            if (g_V8Runtime->parsingGraphData.empty()) {
                return CODE_FAIL;
            }

            g_V8Runtime->parsingGraphData.currentUsedSourceStream() << source;
            return CODE_OK;
        }

        // print func
        void v8Print(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();            
            auto context = isolate->GetCurrentContext();
            auto stackTrace = v8::StackTrace::CurrentStackTrace(isolate, 10, v8::StackTrace::kOverview);

            std::string fileName;
            int lineNumber = 0;
            std::string functionName;
            if (stackTrace->GetFrameCount() > 0) {
                auto frame = stackTrace->GetFrame(isolate, 0);
                if (!frame->GetScriptName().IsEmpty()) {
                    fileName = v8pp::from_v8<std::string>(isolate, frame->GetScriptName());
                }
                lineNumber = frame->GetLineNumber();
                if (!frame->GetFunctionName().IsEmpty()) {
                    functionName = v8pp::from_v8<std::string>(isolate, frame->GetFunctionName());
                }
            }
            LogConstructor tmpLog(fileName.c_str(), lineNumber, functionName.c_str(), LogLevel::Info);
            for( int i = 0; i < args.Length(); i++){
                auto arg = args[i];

                if (IS_V8_STRING(arg)) {
                    std::string msg = v8pp::from_v8<std::string>(isolate, arg);
                    tmpLog.print(msg);
                } else if (IS_V8_NUMBER(arg)) {
                    if (arg->IsInt32() ) {
                        int msg = arg->Int32Value(context).ToChecked();
                        tmpLog.print(msg);
                    } else if (arg->IsUint32()) {
                        uint msg = arg->Uint32Value(context).ToChecked();
                        tmpLog.print(msg);
                    } else {
                        auto msg = arg->NumberValue(context).ToChecked();
                        tmpLog.print(msg);
                    }
                } else if (arg->IsObject()) {
                    auto str = v8pp::json_str(isolate, arg);
                    auto msg = str.c_str();
                    tmpLog.print(msg);
                } else if (arg->IsUndefined()) {
                    constexpr const char* msg = "undefined";
                    tmpLog.print(msg);
                } else if (arg->IsNull()) {
                    constexpr const char* msg = "null";
                    tmpLog.print(msg);
                } else if (arg->IsExternal()) {
                    constexpr const char* msg = "external";
                    tmpLog.print(msg);
                } else if (arg->IsBoolean() || arg->IsBooleanObject()) {
                    const char* msg = arg->BooleanValue(isolate) ? "true" : "false";
                    tmpLog.print(msg);
                }
                else {
                    logError("no-handled-type");
                }
            }
        }

        void v8Tell(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();
            HandleScope handleScope(isolate);

            auto object = Object::New(isolate);
            auto context =isolate->GetCurrentContext();
            auto stringFlag = v8pp::to_v8(isolate, "flag");
            object->Set(context, stringFlag, v8pp::to_v8(isolate, false)).ToChecked();
            if (args.Length() < 2) {
                args.GetReturnValue().Set(object);
                return;
            }

            SightNode* targetNode = nullptr;
            auto arg1 = args[0];
            if (arg1->IsUint32()) {
                auto id = arg1->Uint32Value(context).ToChecked();
                targetNode = currentGraph()->findNode(id);
            } else if (arg1->IsObject()) {
                // maybe a wrap object
                targetNode = v8pp::class_<SightNode>::unwrap_object(isolate, arg1);
            } 
            if (!targetNode) {
                args.GetReturnValue().Set(object);
                return;
            }
            auto templateNode = targetNode->templateNode;
            if (!templateNode->onMsg) {
                args.GetReturnValue().Set(object);
                return;
            }

            auto type = args[1];
            Local<Value> msg = v8::Undefined(isolate);
            if (args.Length() >= 3) {
                msg = args[2];
            }

            auto result = templateNode->onMsg(isolate, targetNode, type, msg);
            if (result.IsEmpty()) {
                args.GetReturnValue().Set(object);
                return;
            }

            object->Set(context, stringFlag, v8pp::to_v8(isolate, true)).ToChecked();
            object->Set(context, v8pp::to_v8(isolate, "result"), result.ToLocalChecked()).ToChecked();
            args.GetReturnValue().Set(object);
        }

        void v8AddType(const FunctionCallbackInfo<Value>& args) {
            if (args.Length() <= 0) {
                args.GetReturnValue().Set(-1);
                return;
            }

            auto v8TypeName = args[0];
            if (!IS_V8_STRING(v8TypeName)) {
                args.GetReturnValue().Set(-2);
                return;
            }
            auto isolate = args.GetIsolate();
            auto name = v8pp::from_v8<std::string>(isolate, v8TypeName);
            TypeInfoRender render;
            render.kind = TypeInfoRenderKind::Default;
            if (args.Length() >= 2) {
                auto options = args[1];
                if (!options->IsObject()) {
                    args.GetReturnValue().Set(-3);
                    return;
                }

                auto context = isolate->GetCurrentContext();
                HandleScope handleScope(isolate);
                auto object = options.As<Object>();
                auto keys = object->GetOwnPropertyNames(context).ToLocalChecked();
                for (int i = 0; i < keys->Length(); i++) {
                    auto key = keys->Get(context, i).ToLocalChecked();
                    auto value = object->Get(context, key).ToLocalChecked();
                    auto keyString = v8pp::from_v8<std::string>(isolate, key);

                    if (keyString == "kind") {
                        if (IS_V8_STRING(value)) {
                            auto valueString = v8pp::from_v8<std::string>(isolate, value);
                            lowerCase(valueString);
                            if (valueString == "combo box") {
                                render.kind = TypeInfoRenderKind::ComboBox;
                            }
                        } else {
                            logDebug("kind not a string");
                            return;
                        }

                        render.initData();
                    } else if (keyString == "data") {
                        switch (render.kind) {
                        case TypeInfoRenderKind::Default:
                            break;
                        case TypeInfoRenderKind::ComboBox:{
                            auto& data = render.data.comboBox;
                            if (!value->IsArray()) {
                                return;
                            }
                            auto array = value.As<Array>();
                            for( int i = 0; i < array->Length(); i++){
                                auto v = array->Get(context, i).ToLocalChecked();
                                if (IS_V8_STRING(v)) {
                                    data.list->push_back(v8pp::from_v8<std::string>(isolate, v));
                                }
                            }
                            break;
                        }
                        }
                    } else if (keyString == "defaultValue") {
                        switch (render.kind) {
                        case TypeInfoRenderKind::Default:
                            break;
                        case TypeInfoRenderKind::ComboBox:
                        {
                            auto& data = render.data.comboBox;
                            if (IS_V8_STRING(value)) {
                                std::string v = v8pp::from_v8<std::string>(isolate, value);
                                auto find = std::find(data.list->begin(), data.list->end(), v);
                                if (find != data.list->end()) {
                                    // has one
                                    data.selected = static_cast<int>(std::distance(data.list->begin(), find));
                                }
                            }  else if (IS_V8_NUMBER(value)) {
                                data.selected = v8pp::from_v8<int>(isolate, value);
                            }
                            break;
                        }
                        }
                    } 
                    else {

                        logDebug("unhandled option", keyString);
                    }
                }
            }

            // add type
            addTypeInfo({
                .name = name,
                .render = render,
            }, true);
        }

        void v8AddNode(SightNode const& node) {
            g_NodeCache.push_back(node);
        }

        void v8AddTemplateNode(const FunctionCallbackInfo<Value> &args) {
            auto isolate = args.GetIsolate();
            HandleScope handleScope(isolate);

            // try make a template node.
            auto arg1 = args[0];
            if (!arg1->IsObject()) {
                args.GetReturnValue().Set(-1);
                return;
            }

            auto context = isolate->GetCurrentContext();
            //
            SightJsNode* sightNode = new SightJsNode();
            auto templateNode = sightNode;
            std::string address;

            auto eventHandleFunc = [isolate, &context](Local<Object> option, const char* key, ScriptFunctionWrapper& f) {
                auto temp = option->Get(context, v8pp::to_v8(isolate, key));
                Local<Value> tempVal;
                if (!temp.IsEmpty() && (tempVal = temp.ToLocalChecked())->IsFunction()) {
                    auto copiedFunc = tempVal.As<Function>();
                    // copiedFunc->SetName(v8pp::to_v8(isolate, key));
                    auto source = functionProtoToString(isolate, context, copiedFunc);
                    f = source;
                }
            };

            // func
            auto nodePortWork = [isolate, &context, &eventHandleFunc](Local<Value> key,Local<Value> value, NodePortType nodePortType, bool allowPortType = false)
                    -> SightJsNodePort {
                // lambda body.
                std::string portName = v8pp::from_v8<std::string>(isolate, key);
                SightJsNodePort port = {
                    portName,
                    nodePortType
                };

                if (IS_V8_STRING(value)) {
                    // type
                    port.type = getIntType(v8pp::from_v8<std::string>(isolate, value), true);
                } else if (value->IsObject()) {
                    // object
                    auto option = value.As<Object>();
                    auto temp = option->Get(context, v8pp::to_v8(isolate, "type"));
                    Local<Value> tempVal;

                    if (!temp.IsEmpty() && IS_V8_STRING((tempVal= temp.ToLocalChecked()))) {
                        // has type
                        port.type = getIntType(v8pp::from_v8<std::string>(isolate, tempVal), true);
                    }

                    temp = option->Get(context, v8pp::to_v8(isolate, "showValue"));
                    if (!temp.IsEmpty() && ((tempVal = temp.ToLocalChecked())->IsBoolean())) {
                        port.options.showValue = v8pp::from_v8<bool>(isolate, tempVal);
                    }

                    temp = option->Get(context, v8pp::to_v8(isolate, "show"));
                    if (!temp.IsEmpty() && ((tempVal = temp.ToLocalChecked())->IsBoolean())) {
                        port.options.show = v8pp::from_v8<bool>(isolate, tempVal);
                    }

                    temp = option->Get(context, v8pp::to_v8(isolate, "typeList"));
                    if (!temp.IsEmpty() && ((tempVal = temp.ToLocalChecked())->IsBoolean())) {
                        port.options.typeList = v8pp::from_v8<bool>(isolate, tempVal);
                    }

                    temp = option->Get(context, v8pp::to_v8(isolate, "defaultValue"));
                    if (!temp.IsEmpty() && !(tempVal = temp.ToLocalChecked())->IsNullOrUndefined()) {
                        setPortValue(isolate, &port, tempVal);
                    }

                    if (allowPortType) {
                        temp = option->Get(context, v8pp::to_v8(isolate, "kind"));
                        if (!temp.IsEmpty()) {
                            tempVal = temp.ToLocalChecked();
                            if (tempVal->IsNullOrUndefined()) {

                            } else if (tempVal->IsNumber() || tempVal->IsNumberObject()) {
                                int i = v8pp::from_v8<int>(isolate, tempVal);
                                if (i >= static_cast<int>(NodePortType::Input) && i <= static_cast<int>(NodePortType::Field)) {
                                    //
                                    port.kind = static_cast<NodePortType>(i);
                                } else {
                                    // maybe throw some msg.
                                }
                            } else if (IS_V8_STRING(tempVal)) {
                                std::string kindString = v8pp::from_v8<std::string>(isolate, tempVal);
                                lowerCase(kindString);

                                if (kindString == "input") {
                                    port.kind = NodePortType::Input;
                                } else if (kindString == "output") {
                                    port.kind = NodePortType::Output;
                                } else if (kindString == "both") {
                                    port.kind = NodePortType::Both;
                                } else if (kindString == "field") {
                                    port.kind = NodePortType::Field;
                                }
                            }
                        }
                    }

                    // events
                    eventHandleFunc(option, "onValueChange", port.onValueChange);
                    eventHandleFunc(option, "onAutoComplete", port.onAutoComplete);
                    eventHandleFunc(option, "onConnect", port.onConnect);
                    eventHandleFunc(option, "onDisconnect", port.onDisconnect);
                    eventHandleFunc(option, "onClick", port.onClick);

                }

                port.value.setType(port.type);
                return port;
            };
            
            // parse object
            Local<Object> object = arg1.As<Object>();
            auto props = object->GetOwnPropertyNames(context).ToLocalChecked();
            for (int i = 0, l = props->Length(); i < l; ++i) {
                Local<Value> localKey = props->Get(context, i).ToLocalChecked();
                Local<Value> localVal = object->Get(context, localKey).ToLocalChecked();

                std::string key = v8pp::from_v8<std::string>(isolate, localKey);

                if (key == "__meta_name") {
                    if (localVal->IsString() || localVal->IsStringObject()) {
                        templateNode->nodeName = v8pp::from_v8<std::string>(isolate, localVal);
                    } else {
                        // maybe need throw something..
                        args.GetReturnValue().Set(-2);
                        return;
                    }
                } else if (key == "__meta_address") {
                    if (localVal->IsString() || localVal->IsStringObject()) {
                        address = v8pp::from_v8<std::string>(isolate, localVal);
                    } else {
                        // maybe need throw something..
                        args.GetReturnValue().Set(-2);
                        return;
                    }
                } else if (key == "__meta_inputs") {
                    if (!localVal->IsObject()) {
                        args.GetReturnValue().Set(-2);
                        return;
                    }

                    auto inputRoot = localVal.As<Object>();
                    auto inputKeys = inputRoot->GetOwnPropertyNames(context).ToLocalChecked();
                    for (int j = 0; j < inputKeys->Length(); ++j) {
                        auto inputKey = inputKeys->Get(context, j).ToLocalChecked();
                        auto inputValue = inputRoot->Get(context, inputKey).ToLocalChecked();

                        auto port = nodePortWork(inputKey, inputValue, NodePortType::Input);
                        templateNode->addPort(port);
                    }
                } else if (key == "__meta_outputs") {
                    if (!localVal->IsObject()) {
                        args.GetReturnValue().Set(-2);
                        return;
                    }

                    auto outputRoot = localVal.As<Object>();
                    auto outputKeys = outputRoot->GetOwnPropertyNames(context).ToLocalChecked();
                    for (int j = 0; j < outputKeys->Length(); ++j) {
                        auto outputKey = outputKeys->Get(context, j).ToLocalChecked();
                        auto outputValue = outputRoot->Get(context, outputKey).ToLocalChecked();

                        auto port = nodePortWork(outputKey, outputValue, NodePortType::Output);
                        templateNode->addPort(port);
                    }
                } else if (key == "__meta_func") {
                    // functions.
                    if (localVal->IsFunction()) {
                        //
                        templateNode->generateCodeWork = Persistent<Function, CopyablePersistentTraits<Function>>(isolate, localVal.As<Function>());
                        templateNode->onReverseActive = templateNode->generateCodeWork;
                    } else if (localVal->IsObject()) {
                        auto functionRoot = localVal.As<Object>();
                        auto temp = functionRoot->Get(context, v8pp::to_v8(isolate, "generateCodeWork"));

                        if (!temp.IsEmpty()) {
                            templateNode->generateCodeWork = Persistent<Function, CopyablePersistentTraits<Function>>(isolate, temp.ToLocalChecked().As<Function>());
                        }

                        temp = functionRoot->Get(context, v8pp::to_v8(isolate, "onReverseActive"));
                        if (temp.IsEmpty()) {
                            logDebug("empty function");
                            templateNode->onReverseActive = templateNode->generateCodeWork;
                        } else {
                            auto checkedTemp = temp.ToLocalChecked();
                            if (checkedTemp->IsFunction()) {
                                templateNode->onReverseActive = Persistent<Function, CopyablePersistentTraits<Function>>(isolate, checkedTemp.As<Function>());
                            } else {
                                templateNode->onReverseActive = templateNode->generateCodeWork;
                            }
                        }

                    } else {
                        args.GetReturnValue().Set(-2);
                        return;
                    }
                } else if (key == "__meta_options") {
                    if (!localVal->IsObject()) {
                        args.GetReturnValue().Set(-2);
                        return;
                    }

                    auto optionsRoot = localVal.As<Object>();
                    auto optionsKeys = optionsRoot->GetOwnPropertyNames(context).ToLocalChecked();

                    for (int j = 0; j < optionsKeys->Length(); ++j) {
                        auto optionKey = optionsKeys->Get(context, j).ToLocalChecked();
                        std::string keyString = v8pp::from_v8<std::string>(isolate, optionKey);
                        auto optionValue = optionsRoot->Get(context, optionKey).ToLocalChecked();

                        // 
                        logDebug("unknown option key: $0", keyString);
                    }
                } else if (key == "__meta_events") {
                    // events 
                    if (!localVal->IsObject()) {
                        args.GetReturnValue().Set(-2);
                        return;
                    }

                    auto eventsRoot = localVal.As<Object>();
                    auto eventsKeys = eventsRoot->GetOwnPropertyNames(context).ToLocalChecked();
                    for( int j = 0; j < eventsKeys->Length(); j++){
                        auto eventKey = eventsKeys->Get(context, j).ToLocalChecked();
                        std::string keyString = v8pp::from_v8<std::string>(isolate, eventKey);
                        auto eventValue = eventsRoot->Get(context, eventKey).ToLocalChecked();
                        std::string sourceCode = functionProtoToString(isolate, context, eventValue.As<Function>());
                        if (sourceCode.empty()) {
                            logDebug("function source not found: $0", keyString);
                            continue;
                        }
                        if (keyString == "onInstantiate") {
                            templateNode->onInstantiate = sourceCode;
                        } else if (keyString == "onDestroyed") {
                            templateNode->onDestroyed = sourceCode;
                        } else if (keyString == "onReload") {
                            templateNode->onReload = sourceCode;
                        } else if (keyString == "onMsg") {
                            templateNode->onMsg = sourceCode;
                        }
                    }
                } else if (key == "__meta_component") {
                    auto& component = templateNode->component;
                    component.active = true;

                    auto tmpRoot = localVal.As<Object>();
                    auto tmpKeys = tmpRoot->GetOwnPropertyNames(context).ToLocalChecked();
                    for( int j = 0; j < tmpKeys->Length(); j++){
                        auto tmpKey = tmpKeys->Get(context, j).ToLocalChecked();
                        std::string keyString = v8pp::from_v8<std::string>(isolate, tmpKey);
                        auto tmpValue = tmpRoot->Get(context, tmpKey).ToLocalChecked();

                        if (keyString == "activeOnReverse") {
                            component.activeOnReverse = tmpValue->BooleanValue(isolate);
                        } else if (keyString == "beforeGenerate") {
                            component.beforeGenerate = ScriptFunctionWrapper::Function(isolate, tmpValue.As<Function>());
                        } else if (keyString == "afterGenerate") {
                            component.afterGenerate = ScriptFunctionWrapper::Function(isolate, tmpValue.As<Function>());
                        }
                    }
                }

                else {
                    // fields
                    auto port = nodePortWork(localKey, localVal, NodePortType::Field, true);
                    templateNode->addPort(port);
                }
            }

            // check name
            if (!endsWith(address, templateNode->nodeName)) {
                if (!endsWith(address, "/")) {
                    address += "/";
                }
                address += templateNode->nodeName;
            }

            g_TemplateNodeCache.emplace_back(
                address,
                sightNode
            );

            args.GetReturnValue().Set(0);
        }

        std::string v8ReverseActiveToCode(uint nodeId){
            auto& data = g_V8Runtime->parsingGraphData;
            if (!data.graph) {
                return {};
            }

            auto node = data.graph->findNode(nodeId);
            if (!node) {
                return {};
            }

            // call reverse active function.
            auto jsNode = findTemplateNode(node);
            if (!jsNode) {
                return {};
            }

            if (jsNode->onReverseActive.IsEmpty()) {
                return {};
            }

            return runGenerateFunction(jsNode->onReverseActive, g_V8Runtime->isolate, node );
        }

        /**
         * @brief 
         * 
         * @return true 
         * @return false 
         */
        bool v8ReverseActive(uint nodeId) {

            auto source = v8ReverseActiveToCode(nodeId);
            if (source.empty()) {
                return false;
            }

            auto & data = g_V8Runtime->parsingGraphData;
            data.currentUsedSourceStream() << source;
            return true;
        }

        bool v8GenerateCode(uint nodeId) {
            auto& data = g_V8Runtime->parsingGraphData;
            if (!data.graph) {
                return false;
            }

            auto node = data.graph->findNode(nodeId);
            if (!node) {
                return false;
            }

            data.addNewLink(node);
            return true;
        }

        void v8EnsureNodeGenerated(uint nodeId) {
            auto& g = g_V8Runtime->parsingGraphData.getGenerateInfo(nodeId);
            if (!g.hasGenerated()) {
                v8GenerateCode(nodeId);
            }
        }

        void v8GetOtherSideValue(FunctionCallbackInfo<Value> const& args){
            auto rValue = args.GetReturnValue();
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());
            if (args.Length() < 3 || !node) {
                return rValue.SetUndefined();
            }

            auto arg1 = args[0];
            auto arg2 = args[1];
            auto arg3 = args[2];

            SightNodePort* selfPort = nullptr;
            std::string targetAddressOrName;
            std::string targetPortName;

            if (IS_V8_STRING(arg1)) {
                selfPort = node->findPort(v8pp::from_v8<const char*>(isolate, arg1)).get();
            } else if (IS_V8_NUMBER(arg1)) {
                selfPort = node->graph->findPort(v8pp::from_v8<uint>(isolate, arg1));
            }
            
            if (IS_V8_STRING(arg2)) {
                targetAddressOrName = v8pp::from_v8<std::string>(isolate, arg2);
            }

            if (IS_V8_STRING(arg3)) {
                targetPortName = v8pp::from_v8<std::string>(isolate, arg3);
            }

            if (!selfPort || !selfPort->isConnect() || targetPortName.empty()) {
                return rValue.SetUndefined();
            }

            auto c = selfPort->connections[0];
            SightNodePortConnection connection(node->graph, c, node);
            auto targetNode = connection.target->node;
            if (!targetAddressOrName.empty()) {
                if (!targetNode->templateNode) {
                    return rValue.SetUndefined();
                }

                auto templateNode = targetNode->templateNode;
                if (templateNode->nodeName != targetAddressOrName && templateNode->fullTemplateAddress != targetAddressOrName) {
                    return rValue.SetUndefined();
                }
            }
            
            auto portHandle = targetNode->findPort(targetPortName.c_str());
            if (portHandle) {
                auto tmpPort = portHandle.get();
                rValue.Set(getPortValue(isolate, tmpPort->getType(), tmpPort->value));
            }  else {
                rValue.SetUndefined();
            }
        }

        bool v8AddBuildTarget(const char* name, Local<Function> buildFunc, bool override){
            auto p = currentProject();
            auto & map = p->getBuildTargetMap();
            if (!override && map.contains(name)) {
                return false;
            }

            auto isolate = buildFunc->GetIsolate();
            map[name] = { name, ScriptFunctionWrapper::Function(isolate, buildFunc) };

            return true;
        }

        void v8EntityFieldTypedValue(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto field = v8pp::class_<SightEntityField>::unwrap_object(isolate, args.This());
            if (!field || field->defaultValue.empty()) {
                return args.GetReturnValue().SetUndefined();
            }
            
            SightNodeValue v;
            v.setType(getIntType(field->type));
            v.setValue(field->defaultValue);
            args.GetReturnValue().Set(getPortValue(isolate, v.getType(), v));
        }

        void v8AddConnectionCodeTemplate(const char* name, const char* description, Local<Function> f) {
            auto isolate = f->GetIsolate();
            
            g_V8Runtime->connectionCodeTemplateMap.try_emplace(name, name, description, PersistentFunction(isolate,f));
        }

        void v8GetNodeHelper(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());
            if (node == nullptr) {
                return args.GetReturnValue().SetUndefined();
            }

            args.GetReturnValue().Set(getNodeHelper(node, isolate));
        }

    }

    /**
     * init js engine (v8)
     *
     * @param execPath
     * @return
     */
    int initJsEngine(const char *execPath) {
        if (g_V8Runtime) {
            return -1;
        }

        // Initialize V8.
        v8::V8::InitializeICUDefaultLocation(execPath);
        v8::V8::InitializeExternalStartupData(execPath);
        std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();
        // Create a new Isolate and make it the current one.
        v8::Isolate::CreateParams createParams;
        createParams.array_buffer_allocator =
                v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        auto isolate = v8::Isolate::New(createParams);
        auto isolate_scope = std::make_unique<v8::Isolate::Scope>(isolate);

        g_V8Runtime = new V8Runtime{
                std::move(platform),
                isolate,
                createParams.array_buffer_allocator,
                std::move(isolate_scope)
        };

        logDebug("init js over.");
        return 0;
    }

    /**
     * destroy js engine
     * @return
     */
    int destroyJsEngine() {
        logDebug("start destroy js engine..");
        freeParser();

        if (!g_V8Runtime) {
            return -1;
        }

        clearJsNodeCache();
        g_V8Runtime->codeTemplateMap.clear();
        g_V8Runtime->connectionCodeTemplateMap.clear();
        
        g_V8Runtime->isolateScope.reset();
        g_V8Runtime->entityFunctions.reset();
        if (g_V8Runtime->isolate) {
            g_V8Runtime->isolate->Dispose();
            g_V8Runtime->isolate = nullptr;
        }

        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();

        if (g_V8Runtime->arrayBufferAllocator) {
            delete g_V8Runtime->arrayBufferAllocator;
            g_V8Runtime->arrayBufferAllocator = nullptr;
        }

        delete g_V8Runtime;
        g_V8Runtime = nullptr;
        return CODE_OK;
    }

    void bindTinyData(Isolate* isolate, const v8::Local<v8::Context>& context) {
        auto tinyData = v8pp::wrap_function(isolate, "tinyData", &v8TinyData);
        auto checkTinyData = v8pp::wrap_function(isolate, "tinyData", &v8CheckTinyData);
        context->Global()->Set(context, v8pp::to_v8(isolate, "tinyData"), tinyData).ToChecked();
        context->Global()->Set(context, v8pp::to_v8(isolate, "checkTinyData"), checkTinyData).ToChecked();
    }

    void bindBaseFunctions(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8pp::module & module) {
        auto global = context->Global();

        auto printFunc = v8pp::wrap_function(isolate, "print", &v8Print);
        global->Set(context, v8pp::to_v8(isolate, "print"), printFunc).ToChecked();
        auto tellFunc = v8pp::wrap_function(isolate, "tell", &v8Tell);
        global->Set(context, v8pp::to_v8(isolate, "tell"), tellFunc).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "v8Include"), v8pp::wrap_function(isolate, "v8Include", &v8Include)).ToChecked();
        auto includeFunc = [](const char* path, Local<Object> module) {
            return v8Include(path, module);
        };
        global->Set(context, v8pp::to_v8(isolate, "include"), v8pp::wrap_function(isolate, "include", includeFunc)).ToChecked();

        // auto initTypes = Object::New(isolate);
        v8pp::module initTypes(isolate);

        initTypes.set_const("JS", MODULE_INIT_JS);
        initTypes.set_const("UI", MODULE_INIT_UI);
        initTypes.set_const("js", MODULE_INIT_JS);
        initTypes.set_const("ui", MODULE_INIT_UI);
        module.set_const("InitType", initTypes);

    }

    void bindNodeTypes(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8pp::module& module) {
        HandleScope handleScope(isolate);

        v8pp::module jsNodePortTypeEnum(isolate);
        jsNodePortTypeEnum
            .set_const("Input", static_cast<int>(NodePortType::Input))
            .set_const("Output", static_cast<int>(NodePortType::Output))
            .set_const("Both", static_cast<int>(NodePortType::Both))
            .set_const("Field", static_cast<int>(NodePortType::Field));
        module.set("NodePortType", jsNodePortTypeEnum);

        v8pp::class_<SightBaseNodePortOptions> nodePortOptionsClass(isolate);
        nodePortOptionsClass
            .ctor<>()
            .set("show", &SightBaseNodePortOptions::show)
            .set("showValue", &SightBaseNodePortOptions::showValue)
            .set("errorMsg", &SightBaseNodePortOptions::errorMsg);
        nodePortOptionsClass.auto_wrap_objects(true);
        module.set("SightNodePortOptions", nodePortOptionsClass);

        v8pp::class_<SightNodePort> nodePortClass(isolate);
        nodePortClass.ctor<>()
            .set("portName", v8pp::property(&SightNodePort::getPortName))
            .set("name", v8pp::property(&SightNodePort::getPortName))
            .set("id", v8pp::property(&SightNodePort::getId))
            .set("node", v8pp::property(&SightNodePort::getNode))
            .set("nodeId", &SightNodePort::getNodeId)
            .set("setKind", &SightNodePort::setKind)
            .set("options", &SightNodePort::options);
        nodePortClass.auto_wrap_objects(true);
        module.set("SightNodePort", nodePortClass);

        v8pp::class_<SightNode> nodeClass(isolate);
        nodeClass
            .ctor<>()
            .set("nodeName", &SightNode::nodeName)
            .set("name", &SightNode::nodeName)
            .set("nodeId", v8pp::property(&SightNode::getNodeId))
            .set("id", v8pp::property(&SightNode::getNodeId))
            .set("addPort", &SightNode::addPort)
            .set("templateAddress", &v8NodeTemplateAddress)
            .set("portValue", &v8NodePortValue)
            .set("getPorts", &v8NodeGetPorts)
            .set("getOtherSideValue", &v8GetOtherSideValue)
            .set("helper", &v8GetNodeHelper)
            ;
        nodeClass.auto_wrap_objects(true);
        module.set("SightNode", nodeClass);

        v8pp::class_<SightNodePortWrapper> nodePortWrapperClass(isolate);
        nodePortWrapperClass
            .set("id", v8pp::property(&SightNodePortWrapper::getId))
            .set("portId", v8pp::property(&SightNodePortWrapper::getId))
            .set("name", v8pp::property(&SightNodePortWrapper::getName))
            .set("kind", v8pp::property(&SightNodePortWrapper::getKind))
            .set("show", v8pp::property(&SightNodePortWrapper::isShow, &SightNodePortWrapper::setShow))
            .set("errorMsg", v8pp::property(&SightNodePortWrapper::getErrorMsg, &SightNodePortWrapper::setErrorMsg))
            .set("readonly", v8pp::property(&SightNodePortWrapper::isReadonly, &SightNodePortWrapper::setReadonly))
            .set("type", v8pp::property(&SightNodePortWrapper::getType, &SightNodePortWrapper::setType))
            .set("deleteLinks", &SightNodePortWrapper::deleteLinks)
            .set("resetType", &SightNodePortWrapper::resetType)
            ;
        module.set("SightNodePortWrapper", nodePortOptionsClass);

        v8pp::class_<SightNodeGraphWrapper> nodeGraphWrapperClass(isolate);
        nodeGraphWrapperClass
            .set("nodes", v8pp::property(&SightNodeGraphWrapper::getCachedNodes))
            .set("updateNodeData", &SightNodeGraphWrapper::updateNodeData)
            .set("findNodeWithId", &SightNodeGraphWrapper::findNodeWithId)
            .set("findNodeWithFilter", &SightNodeGraphWrapper::findNodeWithFilter)
            .set("findNodePort", &SightNodeGraphWrapper::findNodePort)
            .set("findNodeByPortId", &SightNodeGraphWrapper::findNodeByPortId);
        module.set("SightNodeGraphWrapper", nodeGraphWrapperClass);

        v8pp::module connectionModule(isolate);
        connectionModule.set("addCodeTemplate", v8AddConnectionCodeTemplate);
        module.set("connection", connectionModule);

        // connection class
        v8pp::class_<SightNodeConnection> connectionClass(isolate);
        connectionClass.ctor<>()
            .set("priority", &SightNodeConnection::priority)
            .set("connectionId", &SightNodeConnection::connectionId)
            .set("id", &SightNodeConnection::connectionId)
            .set("left", &SightNodeConnection::left)
            .set("right", &SightNodeConnection::right);
        module.set("SightNodeConnection", connectionClass);

        v8pp::class_<SightNodeGenerateHelper> nodeGenerateHelperClass(isolate);
        nodeGenerateHelperClass.ctor()
            .set("varName", &SightNodeGenerateHelper::varName)
            .set("templateName", v8pp::property(&SightNodeGenerateHelper::getTemplateNodeName))
            ;
        module.set("SightNodeGenerateHelper", nodeGenerateHelperClass);
    }

    void bindUIThreadFunctions(const v8::Local<v8::Context>& context, v8pp::module& module) {
        auto isolate = currentUIStatus()->isolate;
        v8pp::module entityModule(isolate);
        auto addEntityOperation = [](const char* name, const char* desc, Local<Function> f) {
            auto uiStatus = currentUIStatus();
            auto& map = uiStatus->entityOperations;
            return map.addOperation(name, desc, PersistentFunction(uiStatus->isolate, f));
        };
        entityModule.set("addOperation", addEntityOperation);
        module.set("entity", entityModule);

        v8pp::class_<SightEntityFieldOptions> entityFieldOptionsClass(isolate);
        entityFieldOptionsClass
            .ctor()
            .set("portType", v8pp::property(&SightEntityFieldOptions::portTypeValue))
            .set("portOptions", &SightEntityFieldOptions::portOptions);
        entityFieldOptionsClass.auto_wrap_objects();

        v8pp::class_<SightEntityField> entityFieldClass(isolate);
        entityFieldClass.ctor<>()
            .set("name", &SightEntityField::name)
            .set("type", &SightEntityField::type)
            .set("defaultValue", &SightEntityField::defaultValue)
            .set("options", &SightEntityField::options)
            .set("v8TypedValue", v8EntityFieldTypedValue);
        entityFieldClass.auto_wrap_objects(true);
        module.set("entityFieldClass", entityFieldClass);

        v8pp::class_<SightEntity> entityClass(isolate);
        entityClass.ctor<>()
            .set("simpleName", v8pp::property(&SightEntity::getSimpleName))
            .set("packageName", v8pp::property(&SightEntity::getPackageName))
            .set("name", &SightEntity::name)
            .set("templateAddress", &SightEntity::templateAddress)
            .set("fields", &SightEntity::fields);
        module.set("entityClass", entityClass);

    }

    void bindLanguage(v8::Isolate* isolate, const v8::Local<v8::Context>& context, v8pp::module& module){
        v8pp::class_<DefLanguage> defLanguage(isolate);
        defLanguage.ctor<int, int>()
            .set("type", &DefLanguage::type)
            .set("version", &DefLanguage::version);
        module.set("DefLanguage", defLanguage);

        v8pp::module langTypes(isolate);
        langTypes.set_const("JavaScript", static_cast<int>(DefLanguageType::JavaScript));
        langTypes.set_const("Java", static_cast<int>(DefLanguageType::Java));
        langTypes.set_const("CSharp", static_cast<int>(DefLanguageType::CSharp));
        langTypes.set_const("Cpp", static_cast<int>(DefLanguageType::Cpp));
        langTypes.set_const("C", static_cast<int>(DefLanguageType::C));
        module.set("DefLanguageTypes", langTypes);

        // function
        auto global = context->Global();
        auto addCodeTemplate = [](DefLanguage* lang, const char* name, const char* desc, Local<Function> callback){
            logDebug(name);
            if (!lang) {
                return false;
            }
            auto& map = g_V8Runtime->codeTemplateMap;
            auto iter = map.find(name);
            if (iter != map.end()) {
                logWarning("replace code template: $0. New description: $1", name, desc);
                iter->second = {lang, name, desc, callback};
            } else {
                map[name] = { lang, name, desc, callback };
            }

            return true;
        };
        global->Set(context, v8pp::to_v8(isolate, "addCodeTemplate"), 
            v8pp::wrap_function(isolate, "addCodeTemplate", addCodeTemplate)).ToChecked();
        
    }

    v8::Isolate* getJsIsolate() {
        return g_V8Runtime->isolate;
    }

    /**
     * init `generate` js bindings
     * cpp struct, functions to js engine.
     * @return
     */
    int initJsBindings(Isolate* isolate, const v8::Local<v8::Context> &context) {
        if (!g_V8Runtime) {
            return -1;
        }
        context->SetSecurityToken(v8pp::to_v8(isolate,  "this-is-sight"));

        v8pp::module module(isolate);

        // functions ...
        module.set("addNode", &v8AddNode);
        module.set("addBuildTarget", &v8AddBuildTarget);

        auto global = context->Global();
        auto addTemplateNodeFunc = v8pp::wrap_function(isolate, "addTemplateNode", v8AddTemplateNode);
        global->Set(context, v8pp::to_v8(isolate, "addTemplateNode"), addTemplateNodeFunc).ToChecked();
        auto addTypeFunc = v8pp::wrap_function(isolate, "addType", &v8AddType);
        global->Set(context, v8pp::to_v8(isolate, "addType"), addTypeFunc).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "v8ReverseActive"), v8pp::wrap_function(isolate, "v8ReverseActive", &v8ReverseActive)).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "v8ReverseActiveToCode"), v8pp::wrap_function(isolate, "v8ReverseActiveToCode", &v8ReverseActiveToCode)).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "v8GenerateCode"), v8pp::wrap_function(isolate, "v8GenerateCode", &v8GenerateCode)).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "v8GetGenerateInfo"), v8pp::wrap_function(isolate, "v8GetGenerateInfo", &v8GetGenerateInfo)).ToChecked();

        bindBaseFunctions(isolate, context, module);
        bindNodeTypes(isolate, context, module);
        //
        v8pp::class_<GenerateOptions> generateOptionsClass(isolate);
        generateOptionsClass
            .ctor<>()
            .set("noCode", &GenerateOptions::noCode)
            .set("appendLineEnd", &GenerateOptions::appendLineEnd);

        v8pp::class_<SightNodeGenerateInfo> generateNodeClass(isolate);
        generateNodeClass
            .set("hasGenerated", v8pp::property(&SightNodeGenerateInfo::hasGenerated));
        module.set("SightNodeGenerateInfo", generateNodeClass);

        v8pp::class_<GenerateArg$$> generateArg$$(isolate);
        generateArg$$
            .set("helper", &GenerateArg$$::helper)
            .set("errorReport", &GenerateArg$$::errorReport)
            .set("insertSource", &v8InsertSource)
            .set("ensureNodeGenerated", &v8EnsureNodeGenerated);
        module.set("GenerateArg$$", generateArg$$);

        v8pp::class_<SightEntityFunctions> entityFunctionsClass(isolate);
        entityFunctionsClass
            .set("generateCodeWork", v8pp::property(&SightEntityFunctions::getGenerateCodeWork, &SightEntityFunctions::setGenerateCodeWork))
            .set("onReverseActive", v8pp::property(&SightEntityFunctions::getOnReverseActive, &SightEntityFunctions::setOnReverseActive));
        module.set("SightEntityFunctions", entityFunctionsClass);

        v8pp::class_<ProjectWrapper> projectWrapperClass(isolate);
        projectWrapperClass
            .set("parseAllGraphs", &ProjectWrapper::parseAllGraphs);
        module.set("Project", projectWrapperClass);

        bindLanguage(isolate, context, module);

        auto sightObject = module.new_instance();
        sightObject->Set(context, v8pp::to_v8(isolate, "entity"), 
            v8pp::class_<SightEntityFunctions>::reference_external(isolate, &g_V8Runtime->entityFunctions)).ToChecked();
        global->Set(context, v8pp::to_v8(isolate, "sight"), sightObject).ToChecked();
        logDebug("init js bindings over!");
        return 0;
    }

    // Extracts a C string from a V8 Utf8Value.
    const char* toCString(const v8::String::Utf8Value& value) {
        return *value ? *value : "<string conversion failed>";
    }

    void reportException(v8::Isolate* isolate, v8::TryCatch* try_catch, std::string& errorMsg) {
        v8::HandleScope handle_scope(isolate);
        v8::String::Utf8Value exception(isolate, try_catch->Exception());
        const char* exception_string = toCString(exception);
        v8::Local<v8::Message> message = try_catch->Message();
        if (message.IsEmpty()) {
            // V8 didn't provide any extra information about this error; just
            // print the exception.
            // fprintf(stderr, "%s\n", exception_string);
            errorMsg = exception_string;
        } else {
            // Print (filename):(line number): (message).
            std::stringstream msg;
            v8::String::Utf8Value filename(isolate,
                                           message->GetScriptOrigin().ResourceName());
            v8::Local<v8::Context> context(isolate->GetCurrentContext());
            const char* filename_string = toCString(filename);
            int linenum = message->GetLineNumber(context).FromJust();
            // fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
            msg << filename_string << ":" << linenum << ":" << exception_string << std::endl;
            // Print line of source code.
            v8::String::Utf8Value sourceline(
                    isolate, message->GetSourceLine(context).ToLocalChecked());
            const char* sourceline_string = toCString(sourceline);
            fprintf(stderr, "%s\n", sourceline_string);
            msg << sourceline_string << std::endl;
            // Print wavy underline (GetUnderline is deprecated).
            int start = message->GetStartColumn(context).FromJust();
            for (int i = 0; i < start; i++) {
                // fprintf(stderr, " ");
                msg << " ";
            }
            int end = message->GetEndColumn(context).FromJust();
            for (int i = start; i < end; i++) {
                // fprintf(stderr, "^");
                msg << "^";
            }
            // fprintf(stderr, "\n");
            msg << std::endl;
            v8::Local<v8::Value> stack_trace_string;
            if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
                stack_trace_string->IsString() &&
                v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
                v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
                const char* stack_trace_string = toCString(stack_trace);
                // fprintf(stderr, "%s\n", stack_trace_string);
                msg << stack_trace_string << std::endl;
            }
            errorMsg = msg.str();
        }
    }

    void flushJsNodeCache(std::promise<int>* promise /*= nullptr */){
        // send nodes to ui thread.
        if (!g_NodeCache.empty()) {
            auto size = g_NodeCache.size();
            auto *nodePointer = (SightNode **) malloc(size * sizeof(void *));
            int index = 0;
            for (const auto &item : g_NodeCache) {
                // copy item to array
                nodePointer[index++] = item.clone();
            }
            g_NodeCache.clear();

            // pluginManager()->getPluginStatus().addNodesFinished = false;
            addUICommand(UICommandType::AddNode, nodePointer, size);
        }

        if (!g_TemplateNodeCache.empty()) {
            auto size = g_TemplateNodeCache.size();
            auto *pointer = (SightNodeTemplateAddress **) malloc(size * sizeof(void *));
            int index = 0;

            for (const auto &item : g_TemplateNodeCache) {
                pointer[index++] = new SightNodeTemplateAddress(item.name, item.templateNode);
            }
            g_TemplateNodeCache.clear();

            // pluginManager()->getPluginStatus().addTemplateNodesFinished = false;
            addUICommand(UICommandType::AddTemplateNode, pointer, size);
        }

        if (promise) {
            promise->set_value(1);
        }

        logDebug("flush node cache over!");
    }

    void clearJsNodeCache(){
        g_NodeCache.clear();
        g_TemplateNodeCache.clear();
    }

    /**
     * run a js file.
     * @param filepath
     * @return
     */
    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise, v8::Local<v8::Object> module, v8::Local<v8::Value>* resultOuter) {
        if (!isolate) {
            return CODE_FAIL;
        }
        logDebug(filepath);
        
        
        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();

        TryCatch tryCatch(isolate);

        auto maySource = readFile(isolate, filepath);
        if (maySource.IsEmpty()) {
            logDebug("file path error: $0", filepath);
            return CODE_FILE_NOT_EXISTS;
        }

        //
        v8::Local<v8::String> sourceCode = maySource.ToLocalChecked();
        Local<Integer> lineOffset = Integer::New(isolate, 0);
        ScriptOrigin scriptOrigin(v8pp::to_v8(isolate, filepath), lineOffset, lineOffset);
        v8::ScriptCompiler::Source source(sourceCode, scriptOrigin);
        Local<Object> context_extensions[0];
        MaybeLocal<Function> mayFunction;

        if (module.IsEmpty() || module->IsNullOrUndefined()) {
            mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 0, nullptr, 0, context_extensions);
        } else {
            v8::Local<v8::String> paramModule = v8::String::NewFromUtf8(isolate, "module").ToLocalChecked();
            v8::Local<v8::String> paramExports = v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked();
            v8::Local<v8::String> arguments[] { paramModule, paramExports};

            mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, std::size(arguments), arguments,
                                                                       0, context_extensions);
        }
        
        if (mayFunction.IsEmpty()) {
            logDebug("js file compile error: $0", filepath);
            if (promise) {
                promise->set_value(0);
            }

            if (tryCatch.HasCaught()) {
                //
                std::string errorMsg;
                reportException(isolate, &tryCatch, errorMsg);
                logError(errorMsg);
            }
            return CODE_FILE_ERROR;
        }
        Local<Function> function = mayFunction.ToLocalChecked();
        Local<Object> recv = Object::New(isolate);
        MaybeLocal<Value> result;

        if (module.IsEmpty() || module->IsNullOrUndefined()) {
            result = function->Call(context, recv, 0, nullptr);
        } else {
            auto mayExports = module->Get(context, v8pp::to_v8(isolate, "exports"));
            Local<Value> exports;
            if (mayExports.IsEmpty() || (exports = mayExports.ToLocalChecked()).IsEmpty() || exports->IsNullOrUndefined()) {
                exports = Object::New(isolate);
                module->Set(context, v8pp::to_v8(isolate, "exports"), exports).ToChecked();
            }
            // check globals
            auto temp =  module->Get(context, v8pp::to_v8(isolate, "globals"));
            if (temp.IsEmpty() || temp.ToLocalChecked()->IsNullOrUndefined()) {
                module->Set(context, v8pp::to_v8(isolate, "globals"), Object::New(isolate)).ToChecked();
            }

            // call function
            Local<Value> args[] = { module, exports };
            result = function->Call(context, recv, std::size(args), args);
        }
        
        if (tryCatch.HasCaught()) {
            logDebug("js file has error");
            if (promise) {
                promise->set_value(0);
            }

            //
            std::string errorMsg;
            reportException(isolate, &tryCatch, errorMsg);
            logError(errorMsg);
            return CODE_FAIL;
        }

        if (promise) {
            promise->set_value(1);
        }
        if (resultOuter) {
            if (result.IsEmpty()) {
                return CODE_SCRIPT_NO_RESULT;
            }
            *resultOuter = result.ToLocalChecked();
        }
        logDebug("js file success ran.");
        return CODE_OK;
    }

    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise, v8::Local<v8::Value>* resultOuter) {
        return runJsFile(isolate, filepath, promise, {}, resultOuter);
    }

    std::string analysisGenerateFunction(Isolate* isolate, Local<Function> function, Local<Context> context, GenerateOptions &options, GenerateFunctionStatus* status = nullptr){
        auto toString = function->Get(context, v8pp::to_v8(isolate, "toString")).ToLocalChecked();
        auto toStringFunc = toString.As<Function>();
        auto str = toStringFunc->Call(context, function, 0 , nullptr).ToLocalChecked();
        std::string code;
        if (IS_V8_STRING(str)) {
            code = v8pp::from_v8<std::string>(isolate, str);
        } else {
            logDebug("toString's result not a string");
            return {};
        }

        auto p1 = code.find_first_of('{');
        auto p2 = code.find_last_of('}');
        auto body = removeComment(trimCopy(std::string(code, p1 + 1, p2 - p1 - 1)));

        if (status) {
            auto title = std::string(code, 0, p1);
            bool parentheses = false;
            bool one = false;
            bool lastIsDollar = false;
            int count = 0;
            for (auto c: title) {
                if (!parentheses) {
                    if (c == '(') {
                        parentheses = true;
                    }
                } else {
                    if (isspace(c)) {

                    } else {
                        if (c == ',') {
                            count++;
                        } else {
                            one = true;
                        }
                    }
                }

                if (c == '$') {
                    if (lastIsDollar) {
                        // here it is $$
                        status->shouldEval = true;
                        status->need$$ = true;
                    }

                } else {
                    if (lastIsDollar) {
                        // here it is $
                        status->need$ = true;
                    }

                }

                lastIsDollar = c == '$';
            }

            if (one) {
                count++;
            }
            status->paramCount = count;

            if (status->paramCount >= 2) {
                status->shouldEval = true;
            }

            // When only has 1 param and the body only has 1 line, and the line start with return, then omitted the `return` keyword.
            if (!status->shouldEval && countLineInString(body) == 1) {
                if (startsWith(body, "return")) {
                    auto left = trimCopy(std::string(body, 6));
                    auto commaPos = left.rfind(';');
                    if (commaPos != std::string::npos) {
                        // remove last ;
                        left = std::string(left, 0, commaPos);
                    }
                    options.appendLineEnd = false;
                    return left;
                }
            }
        }

        return body;
    }

    MaybeLocal<Value> runGenerateCode(std::string const& functionCode, Isolate* isolate, Local<Context>& context, SightNode* node, Local<Object> graphObject,
                                      GenerateOptions* options = nullptr, GenerateFunctionStatus* status = nullptr, int reverseActivePort = -1 ) {
        auto sourceCode = v8pp::to_v8(isolate, functionCode);
        ScriptCompiler::Source source(sourceCode);
        Local<Object> contextExt[0];
        v8::Local<v8::String> param$ = v8::String::NewFromUtf8(isolate, "$").ToLocalChecked();
        v8::Local<v8::String> paramOptions = v8::String::NewFromUtf8(isolate, "$$").ToLocalChecked();
        v8::Local<v8::String> arguments[] = {param$, paramOptions};
        auto mayTargetFunction = ScriptCompiler::CompileFunctionInContext(context, &source, std::size(arguments), arguments, 0,contextExt);
        if (mayTargetFunction.IsEmpty()) {
            logDebug("code compiles error: $0", functionCode.c_str());
            return {};
        }
        auto targetFunction = mayTargetFunction.ToLocalChecked();

        // build args.
        auto arg$ = Object::New(isolate);
        auto tmpArg$$ = new GenerateArg$$;
        tmpArg$$->helper = getNodeHelper(node, isolate);
        auto arg$$ = v8pp::class_<GenerateArg$$>::import_external(isolate, tmpArg$$);

        if (!status || status->need$) {
            auto nodeFunc = [node, isolate, &context, &arg$, graphObject](std::vector<SightNodePort> & list, bool isOutput = false) {

                for (const auto &item : list) {
                    std::string name = item.portName;
                    auto emptyFunc = [](){
                        return std::string();
                    };

                    auto t = [&item, node, isolate, graphObject]() -> std::string {
                        // reverseActive this port.
                        //
                        if (!item.isConnect()) {
                            logError("ReverseActive Error, no connection: $1, $0", item.portName, item.getId());
                            return "";      // maybe need throw sth ?
                        }

                        auto & connections = item.connections;
                        if (connections.size() == 1) {
                            // 1
                            auto c = connections.front();
                            SightNodePortConnection connection(node->graph, c, node);
                            if (connection.bad()) {
                                logError("bad connection");
                                return "";
                            }

                            auto targetNode = connection.target->node;
                            auto targetJsNode = findTemplateNode(targetNode);
                            if (targetJsNode) {
                                return runGenerateFunction(targetJsNode->onReverseActive,isolate, targetNode, connection.target->getId());
                            }
                        } else {
                            logError("multiple connections, do not support yet.");
                        }
                        return "";
                    };

                    auto useEmptyFunc = isOutput;    // || item.getType() == IntTypeProcess;   ? 这里当初为什么这么写？ 
                    auto functionObject = useEmptyFunc ? v8pp::wrap_function(isolate, name, emptyFunc) : v8pp::wrap_function(isolate, name, t);
                    auto realType = item.getType();
                    if (realType != IntTypeProcess && realType != IntTypeObject) {
                        functionObject->Set(context, v8pp::to_v8(isolate, "value"), getPortValue(isolate, item.getType(), item.value)).ToChecked();
                    }

                    functionObject->Set(context, v8pp::to_v8(isolate, "isConnect"), v8pp::to_v8(isolate, item.isConnect())).ToChecked();
                    v8pp::set_const(isolate, functionObject, "name", item.getPortName());
                    v8pp::set_const(isolate, functionObject, "id", item.id);

                    // Maybe need an optimization.
                    // AccessorNameGetterCallback a;
                    if (!name.empty()) {
                        arg$->Set(context, v8pp::to_v8(isolate, name), functionObject).ToChecked();
                    }
                    arg$->Set(context, v8pp::to_v8(isolate, item.id), functionObject).ToChecked();
                }

            };

            nodeFunc(node->inputPorts);
            nodeFunc(node->fields);

            nodeFunc(node->outputPorts, true);
            
        }

        auto component = g_V8Runtime->parsingGraphData.component;
        if (!status || status->need$$) {
            if (options) {
                auto jsOptions = v8pp::class_<GenerateOptions>::reference_external(isolate, options);
                arg$$->Set(context, v8pp::to_v8(isolate, "options"), jsOptions).ToChecked();
            }
            arg$$->Set(context, v8pp::to_v8(isolate, "graph"), graphObject).ToChecked();
            if (reverseActivePort > 0) {
                v8pp::set_const(isolate, arg$$, "reverseActivePort", reverseActivePort);
            }

            if (component) {
                v8pp::set_const(isolate, arg$$, "component", v8pp::class_<SightNode>::reference_external(isolate, component));
            }
        }

        Local<Object> recv = v8pp::class_<SightNode>::reference_external(isolate, node);
        Local<Value> args[2];
        args[0] = arg$;
        args[1] = arg$$;
        auto result = targetFunction->Call(context, recv,std::size(args), args);

        if (options) {
            v8pp::class_<GenerateOptions>::unreference_external(isolate, options);
        }
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        if (component) {
            v8pp::class_<SightNode>::unreference_external(isolate, component);
        }
        return result;
    }


    std::string runGenerateFunction(PersistentFunction const& persistent, Isolate* isolate, SightNode* node,
                                    int reverseActivePort) {
        if (persistent.IsEmpty()) {
            // no function ?
            logDebug("no function");
            return "";
        }

        auto function = persistent.Get(isolate);
        auto context = isolate->GetCurrentContext();
        Local<Object> graphObject = g_V8Runtime->parsingGraphData.graphObject;

        GenerateFunctionStatus status;
        GenerateOptions options;
        std::string functionCode = analysisGenerateFunction(isolate, function, context, options, &status);
        MaybeLocal<Value> resultMaybe;
        if (status.shouldEval) {
            // The code need be eval first.
#if GENERATE_CODE_DETAILS == 1
            logDebug("$0, $1",status.shouldEval, functionCode);
#endif
            if (!functionCode.empty()) {
                resultMaybe = runGenerateCode(functionCode, isolate, context, node, graphObject, &options, &status, reverseActivePort);
                functionCode = "";

                if (!resultMaybe.IsEmpty()) {
                    // not empty.
                    auto firstRunResult = resultMaybe.ToLocalChecked();
                    if (firstRunResult->IsFunction()) {
                        functionCode = analysisGenerateFunction(isolate, firstRunResult.As<Function>(), context, options);
                    } else if (firstRunResult->IsNullOrUndefined()) {
                        // do nothing when null or undefined.

                    } else {
                        // Not a function, let it be the result.
                        return jsObjectToString(firstRunResult, isolate);
                    }
                }
            }
        }

        if (!functionCode.empty() && !options.noCode) {
#if GENERATE_CODE_DETAILS == 1
            logDebug("second run: $0", functionCode);
#endif
            std::stringstream ss;
            bool dollar = false;
            ss << "return `";
            uint leftParenthesesCount = 0;
            for (const auto &item : functionCode) {
                if (!dollar) {
                    if (item == '$') {
                        dollar = true;
                        ss << "${";
                    }
                } else {
                    if (isalpha(item) || isnumber(item) || item == '.' || item == '_'
                        || item == '$') {

                    } else if (item == '(' || item == ')') {
                        if (item == '(') {
                            leftParenthesesCount++;
                        } else {
                            // )
                            if (leftParenthesesCount <= 0) {
                                // no `(` in the stmt.
                                ss << '}';
                                dollar = false;
                                leftParenthesesCount = 0;
                            } else {
                                leftParenthesesCount--;
                            }
                        }
                    } 
                    else {
                        dollar = false;
                        ss << '}';
                    }
                }

                ss << item;

            }
            if (dollar) {
                dollar = false;
                ss << '}';
            }
            if (options.appendLineEnd) {
                // ss << ';' << std::endl;
                ss << std::endl;
            }
            ss << '`';

            status = {
                    .need$ = true,
                    .need$$ = false,
            };
            auto result = runGenerateCode(ss.str(), isolate, context, node, graphObject, nullptr, &status, reverseActivePort);
            if (result.IsEmpty()) {
                logDebug("result is empty: $0", ss.str());
            } else {
                auto resultString = v8pp::from_v8<std::string>(isolate, result.ToLocalChecked());
                return resultString;
            }
        }

        return "";
    }
    
    std::string parseConnection(Isolate* isolate, SightNodeConnection* connection) {
        auto& data = g_V8Runtime->parsingGraphData;
        if (data.connectionCodeTemplate.empty() || !connection->generateCode) {
            return {};
        }
        
        auto leftNode = connection->findLeftPort()->node;
        auto rightNode = connection->findRightPort()->node;

        auto isLeftHasGenerated = data.getGenerateInfo(leftNode->getNodeId()).hasGenerated();
        auto isRightHasGenerated = data.getGenerateInfo(rightNode->getNodeId()).hasGenerated();

#if GENERATE_CODE_DETAILS == 1
        trace("left: $0, right: $1", leftNode->getNodeId(), rightNode->getNodeId());
        trace("left: $0, right: $1", isLeftHasGenerated, isRightHasGenerated);
#endif
        if (!isLeftHasGenerated || !isRightHasGenerated) {
            return {};
        }

        // check connection parse count
        auto& connectionParseInfo = data.getGenerateInfo(connection->connectionId);
        if (connectionParseInfo.generateCodeCount > 0) {
            return {};
        }
        connectionParseInfo.generateCodeCount++;
        
        // both has generated, generate connection code.
        auto& codeTemplate = g_V8Runtime->connectionCodeTemplateMap[data.connectionCodeTemplate];

        auto func = codeTemplate.function.function.Get(isolate);
        auto connectionObject = v8pp::class_<SightNodeConnection>::reference_external(isolate, connection);
        auto result =  v8pp::call_v8(isolate, func, connectionObject, data.graphObject);
        v8pp::class_<SightNodeConnection>::unreference_external(isolate, connection);

        if (result.IsEmpty() || result->IsNullOrUndefined()) {
            return {};
        }
        if (IS_V8_STRING(result)) {
            std::string code = v8pp::from_v8<std::string>(isolate, result);
            return code;
        }

        return {};
    }


    void parseAllConnectionsOfNode(Isolate* isolate, std::stringstream& finalSource) {
        auto& data = g_V8Runtime->parsingGraphData;
        if (data.connectionCodeTemplate.empty() || data.currentNode == nullptr) {
            return;
        }

        auto func = [&finalSource, isolate](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                if (!item.isConnect()) {
                    continue;
                }

                for (auto& conn : item.connections) {
                    auto code = parseConnection(isolate, conn);
                    if (!code.empty()) {
                        finalSource << code;
                    }
                }
            }
        };

        #if GENERATE_CODE_DETAILS == 1
        trace(data.currentNode->getNodeId());
        #endif
        func(data.currentNode->inputPorts);
        func(data.currentNode->outputPorts);
    }

    /**
     * @brief 
     * 
     * @param func 
     * @param isolate 
     * @param node 
     * @param type 1=before, 2=after
     * @return std::string 
     */
    std::string runComponentGenerateFunction(v8::Isolate* isolate, SightNode* node, int type) {
        if (node->components.empty()) {
            return {};
        }

        auto& data = g_V8Runtime->parsingGraphData;
        std::string source = {};
        for( const auto& item: node->components){
            auto c = item->templateNode->component;

            data.component = item;
            if (type == 1) {
                source += runGenerateFunction(c.beforeGenerate.function, isolate, node);
            } else if (type == 2) {
                source += runGenerateFunction(c.afterGenerate.function, isolate, node);
            }
        }

        data.component = nullptr;
        return source;
    }
    
    void parseNode(Isolate* isolate, Local<Object> graphObject){
        auto& data = g_V8Runtime->parsingGraphData;
        auto& list = data.list[data.lastUsedIndex].link;
        if (list.empty()) {
            return;
        }
        // parse head.
        auto node = list.front();
        auto jsNode = findTemplateNode(node);
        list.erase(list.begin());
        data.currentNode = node;
        trace(node->getNodeId());
        if (++(data.getGenerateInfo(node->getNodeId()).generateCodeCount) >= 25) {
            //
            data.errorInfo = {
                .msg = "Generate times is over limit!",
                .nodeId = node->getNodeId(),
                .hasError = true,
            };
            return;
        }

        // generateCodeWork
        // call before, generate, after..
        std::string source = runComponentGenerateFunction(isolate, node, 1);
        source += runGenerateFunction(jsNode->generateCodeWork, isolate, node);
        source += runComponentGenerateFunction(isolate, node, 2);

        auto& parsingLink = data.list[data.lastUsedIndex]; 
        if (!source.empty()) {
            parsingLink.sourceStream << source;
        }

        if (data.hasError()) {
            return;
        }

        // active the next.
        auto port = node->findPortByProcess();
        if (port) {
            auto g = node->graph;
            if (port->connections.size() == 1) {
                parsingLink.link.push_back(SightNodePortConnection(g, port->connections.front(), node).target->node);
            } else {
                // question: append 1st item to parsingLink.link ? 
                // use reverse order.
                for (auto iter = port->connections.rbegin(); iter != port->connections.rend(); iter++) {
                    auto& item = *iter;
                    data.addNewLink(SightNodePortConnection(g, item, node).target->node);
                }

                // move parsingLink to last
                auto it = data.list.begin() + data.lastUsedIndex;
                std::rotate(it, it + 1, data.list.end());
                data.lastUsedIndex = data.list.size() - 1;
            }
        }
    }

    int parseGraphToJs(SightNodeGraph& graph, std::string& source, std::string& errorMsg) {
        auto& data = g_V8Runtime->parsingGraphData;
        data.reset();

        // get enter node.
        int status = -1;
        auto node = graph.findEnterNode(&status);
        if (status != CODE_OK) {
            logError("cannot find the enter node, graph: $0, $1", graph.getFilePath(), status);
            return CODE_FAIL;
        }

        // parse the enter node.
        auto isolate = g_V8Runtime->isolate;
        TryCatch tryCatch(isolate);
        v8::HandleScope handle_scope(isolate);
        data.graphObject = v8pp::class_<SightNodeGraphWrapper>::import_external(isolate, new SightNodeGraphWrapper(&graph));
        data.graph = &graph;
        data.connectionCodeTemplate = graph.getSettings().connectionCodeTemplate;

        data.addNewLink(node);
        auto& outerList = data.list;
        std::stringstream finalSourceStream;

        while (!data.empty()) {
            data.lastUsedIndex = outerList.size() - 1;
            parseNode(isolate, data.graphObject);

            auto& list = outerList.back();

            // try parse connection.  (parse current node's all connection)
            parseAllConnectionsOfNode(isolate, list.sourceStream);

            if (list.link.empty()) {
#if GENERATE_CODE_DETAILS == 1
                logDebug("append source, delete last list..");
                logDebug(list.sourceStream.str());
#endif
                finalSourceStream << list.sourceStream.str() << std::endl;     // append 1 \n
                outerList.erase(outerList.end() - 1);
            }

            // list maybe invalid.
            if (data.hasError()) {
                break;
            }
        }

        if (tryCatch.HasCaught()) {
            reportException(isolate, &tryCatch, errorMsg);
        }
        if (data.hasError()) {
            // report error
            auto& errorInfo = data.errorInfo;
            std::stringstream tmpErrorMsg;
            tmpErrorMsg << errorInfo.msg;
            tmpErrorMsg << "\n";
            SightNode* nodeInfo = nullptr;
            if (errorInfo.portId > 0) {
                auto portInfo = graph.findPort(errorInfo.portId);
                tmpErrorMsg << "Port: " << errorInfo.portId << ", ";

                if (portInfo) {
                    nodeInfo = portInfo->node;
                    if (errorInfo.nodeId <= 0) {
                        errorInfo.nodeId = portInfo->node->getNodeId();
                    }

                    tmpErrorMsg << portInfo->portName;
                } else {
                    tmpErrorMsg << "[No Port Info]";
                }

                tmpErrorMsg << std::endl;
            }
            if (errorInfo.nodeId > 0) {
                if (nodeInfo) {
                    if (nodeInfo->getNodeId() != errorInfo.nodeId) {
                        // node and port is not pair
                        nodeInfo = graph.findNode(errorInfo.nodeId);
                    }
                } else {
                    nodeInfo = graph.findNode(errorInfo.nodeId);
                }
                tmpErrorMsg << "Node: " << errorInfo.nodeId << ", ";
                if (nodeInfo) {
                    tmpErrorMsg << nodeInfo->nodeName;
                } else {
                    tmpErrorMsg << "[No Node Info]";
                }
                tmpErrorMsg << std::endl;
            }

            // maybe need append more info ?
            errorMsg = tmpErrorMsg.str();
            // logDebug(tmpErrorMsg.str());
            
        } else {
            std::string finalSource = finalSourceStream.str();
            trim(finalSource);
            source = finalSource;
            return CODE_OK;
        }

        return CODE_FAIL;
    }

    void runJsCommands() {
        if (!g_V8Runtime) {
            return;
        }

        auto &queue = g_V8Runtime->commandQueue;
        while (true) {
            auto command = queue.front();

            switch (command.type) {
            case JsCommandType::Destroy:
                command.args.dispose();
                queue.pop_front();
                goto break_commands_loop;
            case JsCommandType::File:
                runJsFile(g_V8Runtime->isolate, command.args.argString, nullptr, nullptr);
                flushJsNodeCache(command.args.promise);
                break;
            case JsCommandType::JsCommandHolder:
                // do nothing.
                break;
            case JsCommandType::ParseGraph:{
                // std::string source;
                parseGraph(command.args.argString, true, true);
                break;
            }
            case JsCommandType::InitPluginManager:
            {
                pluginManager()->init(g_V8Runtime->isolate);
                pluginManager()->loadPlugins();
                break;
            }
            case JsCommandType::RunFile:
                runJsFile(g_V8Runtime->isolate, command.args.argString, command.args.promise, nullptr);
                break;
            case JsCommandType::FlushNodeCache:
                flushJsNodeCache(command.args.promise);
                break;
            case JsCommandType::InitParser:
                initParser();
                break;
            case JsCommandType::EndInit:
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                addUICommand(UICommandType::JsEndInit);
                break;
            case JsCommandType::Test:
                break;
            case JsCommandType::PluginReload:
            {
                auto & map = pluginManager()->getPluginMap();
                auto iter = map.find(command.args.argString);
                std::string msg;
                if (iter != map.end()) {
                    iter->second->reload();
                    flushJsNodeCache();
                    msg = command.args.argString;
                    msg += " Load Success " ICON_MD_DONE;
                } else {
                    msg = ICON_MD_ERROR " Cannot found plugin: ";
                    msg += command.args.argString;
                }

                // send ui command
                addUICommand(UICommandType::PluginReloadOver, CommandArgs::copyFrom(msg));
                break;
            }
            case JsCommandType::ProjectBuild:
                currentProject()->build(command.args.argString);
                break;
            case JsCommandType::ProjectClean:
                currentProject()->clean();
                break;
            case JsCommandType::ProjectRebuild:
                currentProject()->rebuild();
                break;
            case JsCommandType::ProjectCodeSetBuild:
                currentProject()->codeSetBuild();
                break;
            }

            command.args.dispose();
            queue.pop_front();
        }

        break_commands_loop:
        return;
    }

    void jsThreadRun(const char *exeName) {
        initJsEngine(exeName);

        logDebug("init code set");
        initCodeSet();

        {
            auto isolate = g_V8Runtime->isolate;
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Context> context = v8::Context::New(isolate);
            //         Enter the context
            v8::Context::Scope context_scope(context);
            initJsBindings(isolate, context);

            runJsCommands();
        }

        logDebug("destroy code set and js engine.");
        destroyCodeSet();
        destroyJsEngine();
    }

    int addJsCommand(JsCommandType type, int argInt) {
        JsCommand command = {
                type,
                {
                        .argInt =  argInt
                },
        };

        return addJsCommand(command);
    }

    int addJsCommand(JsCommandType type, CommandArgs const& args) {
        JsCommand command = { type, args };
        return addJsCommand(command);
    }

    int addJsCommand(JsCommandType type, const char *argString, int argStringLength, bool argStringNeedFree, std::promise<int>* promise) {
        JsCommand command = {
                type,
                {
                        argString,
                        argStringLength,
                        argStringNeedFree,
                },
        };

        command.args.promise = promise;
        return addJsCommand(command);
    }

    int addJsCommand(JsCommand &command) {
        if (!g_V8Runtime) {
            return -1;
        }

        g_V8Runtime->commandQueue.push_back(command);
        return 0;
    }

    v8::Local<v8::Function> recompileFunction(v8::Isolate* isolate, std::string sourceCode) {
        auto context = isolate->GetCurrentContext();

        trim(sourceCode);
        if (!startsWith(sourceCode, "function")) {
            sourceCode = "function " + sourceCode;
        }
        sourceCode = "return " + sourceCode;

        v8::ScriptCompiler::Source source(v8pp::to_v8(isolate, sourceCode.c_str()));
        auto mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 0, nullptr, 0, nullptr);
        Local<Value> tmp;
        if (!mayFunction.IsEmpty() && (tmp = mayFunction.ToLocalChecked())->IsFunction()) {
            tmp = tmp.As<Function>()->Call(context, Object::New(isolate), 0, nullptr).ToLocalChecked();
            if (tmp->IsFunction()) {
                return tmp.As<Function>();
            }
        }

        return {};
    }

    void registerToGlobal(v8::Isolate* isolate, std::map<std::string, std::string>* map) {
        HandleScope handleScope(isolate);
        auto context = isolate->GetCurrentContext();

        auto object = Object::New(isolate);
        for (const auto& [name, code] : *map) {
            auto f = recompileFunction(isolate, code);
            if (!f.IsEmpty()) {
                object->Set(context, v8pp::to_v8(isolate, name), f).ToChecked();
            }
        }

        registerToGlobal(isolate, object);
    }

    void registerToGlobal(v8::Isolate* isolate, v8::Local<v8::Value> object) {
        if (object.IsEmpty() || object->IsNullOrUndefined()) {
            return;
        }

        registerToGlobal(isolate, object.As<Object>());
    }

    void registerToGlobal(v8::Isolate* isolate, v8::Local<v8::Object> object, std::map<std::string, std::string>* outFunctionCode) {
        // register
        auto context = isolate->GetCurrentContext();
        auto global = context->Global();

        auto keys = object->GetOwnPropertyNames(context).ToLocalChecked();
        for (int i = 0; i < keys->Length(); i++) {
            auto key = keys->Get(context, i).ToLocalChecked();
            auto value = object->Get(context, key).ToLocalChecked();
            std::string keyString = v8pp::from_v8<std::string>(isolate, key);

            if (value->IsFunction() || value->IsObject()) {
                // only register function, object
                if (value->IsFunction() && outFunctionCode) {
                    // out code
                    auto function = value.As<Function>();
                    outFunctionCode->insert({ keyString, functionProtoToString(isolate, context, function) });
                }

                if (isValid(global->Get(context, key))) {
                    logDebug("repeat key, replace previous one: $0", keyString);
                    // continue;
                }

                if (!global->Set(context, key, value).ToChecked()) {
                    logDebug("set failed: $0", keyString);
                }
            }
        }
    }

    void registerGlobals(v8::Local<v8::Value> value) {
        if (value.IsEmpty() || !value->IsObject()) {
            return;
        }

        auto map = new std::map<std::string, std::string>();
        registerToGlobal(g_V8Runtime->isolate, value.As<Object>(), map);

        // send map to ui thread.
        // addUICommand(UICommandType::RegScriptGlobalFunctions, map);
        delete map;
    }

    SightJsNode& registerEntityFunctions(SightJsNode& node) {
        node.generateCodeWork = g_V8Runtime->entityFunctions.generateCodeWork;
        node.onReverseActive = g_V8Runtime->entityFunctions.onReverseActive;
        return node;
    }

    SightNodePortWrapper::SightNodePortWrapper(SightNodePortHandle portHandle)
        : portHandle(std::move(portHandle)) {
            updatePointer();
    }

    sight::SightNodePortWrapper::~SightNodePortWrapper()
    {
        logDebug("~SightNodePortWrapper");
    }

    uint sight::SightNodePortWrapper::getId() const {
        return pointer->getId();
    }

    const char* sight::SightNodePortWrapper::getErrorMsg() const {
        return pointer->options.errorMsg.c_str();
    }

    void SightNodePortWrapper::setErrorMsg(const char* msg) {
        pointer->options.errorMsg = msg;
    }

    void sight::SightNodePortWrapper::setType(uint v) {
        pointer->type = v;
    }

    void sight::SightNodePortWrapper::resetType() {
        pointer->type = pointer->templateNodePort->type;
    }

    uint sight::SightNodePortWrapper::getType() const {
        return pointer->type;
    }

    void sight::SightNodePortWrapper::setReadonly(bool v) {
        pointer->options.readonly = v;
    }

    bool sight::SightNodePortWrapper::isReadonly() const {
        return pointer->options.readonly;
    }

    void sight::SightNodePortWrapper::setShow(bool v) {
        pointer->options.show = v;
    }

    bool sight::SightNodePortWrapper::isShow() const {
        return pointer->options.show;
    }

    void sight::SightNodePortWrapper::deleteLinks() {
        pointer->clearLinks();
    }

    const char* sight::SightNodePortWrapper::getName() const {
        return pointer->getPortName();
    }

    void sight::SightNodePortWrapper::updatePointer() {
        pointer = nullptr;
        if (portHandle) {
            pointer = portHandle.get();
        }
    }

    int SightNodePortWrapper::getKind() const {
        return static_cast<int>(pointer->kind);
    }

    SightNodeGraphWrapper::SightNodeGraphWrapper(SightNodeGraph* graph) : graph(graph) {
        buildNodeCache();
    }

    SightNode sight::SightNodeGraphWrapper::findNodeByPortId(uint id) {
        auto p = graph->findPort(id);
        if (p) {
            return *(p->node);
        }

        return {};
    }

    SightNodePort sight::SightNodeGraphWrapper::findNodePort(uint id) {
        auto p = graph->findPort(id);
        if (p) {
            return *p;
        }

        return {};
    }

    bool sight::SightNodeGraphWrapper::updateNodeData(uint nodeId, v8::Local<v8::Function> f) {
        if (f.IsEmpty()) {
            return false;
        }

        auto isolate = f->GetIsolate();
        isolate->ThrowException(v8::Exception::TypeError(v8pp::to_v8(isolate, "Not impl.")));
        return false;
    }

    SightNode sight::SightNodeGraphWrapper::findNodeWithId(uint id) {
        auto p = graph->findNode(id);
        if (p) {
            return *p;
        }
        return {};
    }

    SightNode sight::SightNodeGraphWrapper::findNodeWithFilter(std::string const& templateAddress, v8::Local<v8::Function> filter) {
        SightNode n;
        for( const auto& item: graph->getNodes()){
            if (!templateAddress.empty() && templateAddress != item.templateAddress()) {
                continue;
            }

            if (filter.IsEmpty()) {
                n = item;
                break;
            }
            
            auto isolate = filter->GetIsolate();
            auto v = v8pp::call_v8(isolate, filter, Object::New(isolate), item);
            if (v->BooleanValue(isolate)) {
                n = item;
                break;
            }
        }

        return n;
    }

    void sight::SightNodeGraphWrapper::buildNodeCache() {
        cachedNodes.clear();
        if (!graph) {
            return;
        }

        for( auto& item: graph->getNodes()){
            cachedNodes.push_back(item);
        }
    }

    std::vector<SightNode>& sight::SightNodeGraphWrapper::getCachedNodes() {
        return cachedNodes;
    }

    sight::SightNodeGraphWrapper::~SightNodeGraphWrapper()
    {
        logDebug("~SightNodeGraphWrapper");
    }

    bool SightNodeGenerateInfo::hasGenerated() const {
        return this->generateCodeCount > 0;
    }

    int BuildTarget::build() const {
        auto isolate = Isolate::GetCurrent();
        auto func = this->buildFunction.Get(isolate);

        auto result = v8pp::call_v8(isolate, func, Object::New(isolate), v8pp::class_<ProjectWrapper>::import_external(isolate, new ProjectWrapper(currentProject())));
        if (!result.IsEmpty()) {
            logDebug("has result");
        }
        return CODE_OK;
    }

    /**
     *
     * @param filename
     * @return
     */
    std::vector<std::string> & getCodeTemplateNames() {
        static std::vector<std::string> names;
        
        auto& map = g_V8Runtime->codeTemplateMap;
        if (names.size() != map.size()) {
            names.clear();
            for( const auto& item: map){
                names.push_back(item.first);
            }
        }

        return names;
    }

    std::vector<std::string>& getConnectionCodeTemplateNames() {
        static std::vector<std::string> names;

        auto& map = g_V8Runtime->connectionCodeTemplateMap;
        if (names.size() != map.size()) {
            names.clear();
            for (const auto& item : map) {
                names.push_back(item.first);
            }
        }

        return names;
    }

    int parseGraph(const char* filename, bool generateTargetLang, bool writeToOutFile) {
        logDebug(filename);
        SightNodeGraph graph;
        int i = graph.load(filename);
        if (i == 1) {
            return CODE_FAIL;
        }
        if (i < 0) {
            return CODE_FAIL;
        }

        std::string source;
        std::string errorMsg;
        if (parseGraphToJs(graph, source, errorMsg) != CODE_OK) {
            logError(errorMsg);
            return CODE_FAIL;
        }

        trace("generated js code: ");
        trace(source);
        if (!generateTargetLang) {
            return CODE_OK;
        }

        auto& settings = graph.getSettings();
        source = parseSource(source, settings.language, &i);
        if (i != CODE_OK) {
            return i;
        }

        // apply code template
        auto const& codeTemplateName = graph.getSettings().codeTemplate;
        if (!codeTemplateName.empty()) {
            auto codeTemplateIter = g_V8Runtime->codeTemplateMap.find(codeTemplateName);
            if (codeTemplateIter == g_V8Runtime->codeTemplateMap.end()) {
                logWarning("Unable to find code-template: $0, jump it.", codeTemplateName);
            } else {
                auto codeTemplate = codeTemplateIter->second;
                std::string_view graphName = graph.getName();
                auto tmp = codeTemplate.getHeader(graphName);
                if (!tmp.empty()) {
                    source = tmp + source;
                }

                source += codeTemplate.getFooter(graphName);
            }
        }

        logInfo(source);
        if (writeToOutFile) {
            if (settings.outputFilePath.empty()) {
                logWarning("graph $0 do not have a output path.", graph.getFilePath());
            } else {
                std::ofstream out(settings.outputFilePath.data(), std::ios::trunc);
                out << source;
            }
        } else {
            logDebug("source do not to write to file");
        }

        return CODE_OK;
    }


    std::string unpackToString(v8::Isolate* isolate, v8::MaybeLocal<v8::Value> value){
        if (value.IsEmpty()) {
            return {};
        }

        auto o = value.ToLocalChecked();
        if (IS_V8_STRING(o)) {
            return v8pp::from_v8<std::string>(isolate, o);
        }
        return {};
    }

    CodeTemplateFunc::CodeTemplateFunc(DefLanguage* lang, std::string_view name, std::string_view desc, v8::Local<v8::Function> func)
        : CommonOperation(std::string{ name }, std::string{ desc }, PersistentFunction{ func->GetIsolate(), func })
        , language(*lang) {
    }

    std::string sight::CodeTemplateFunc::operator()(int index, std::string_view graphName) const {
        auto isolate = g_V8Runtime->isolate;
        auto result = this->function(isolate, index, graphName.data());
        return unpackToString(isolate, result);
    }

    std::string sight::CodeTemplateFunc::getFooter(std::string_view graphName) const {
        if (!enableFooter) {
            return {};
        }

        return (*this)(FooterIndex, graphName);
    }

    std::string sight::CodeTemplateFunc::getHeader(std::string_view graphName) const {
        if (!enableHeader) {
            return {};
        }

        return (*this)(HeaderIndex, graphName);
    }

}
