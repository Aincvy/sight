//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
#include <algorithm>
#include <cstdio>
#include <iterator>
#include <map>
#include <stdio.h>
#include <sstream>
#include "dbg.h"
#include "functional"
#include <string>
#include <thread>
#include <chrono>

#include "sight.h"
#include "sight_defines.h"
#include "sight_js.h"
#include "sight_node_editor.h"
#include "shared_queue.h"
#include "sight_log.h"
#include "sight_ui.h"
#include "sight_util.h"
#include "sight_js_parser.h"
#include "sight_project.h"
#include "sight_plugin.h"

#include "v8.h"
#include "libplatform/libplatform.h"

#include "v8pp/convert.hpp"
#include "v8pp/function.hpp"
#include "v8pp/module.hpp"
#include "v8pp/class.hpp"
#include "v8pp/property.hpp"
#include "v8pp/object.hpp"
#include "v8pp/json.hpp"

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

    std::string runGenerateFunction(PersistentFunction const& persistent, Isolate* isolate, SightNode* node);

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


    Local<Value> getPortValue(Isolate* isolate, int type, SightNodeValue const& value) {
        switch (type) {
        case IntTypeProcess:
            break;
        case IntTypeFloat:
            return v8pp::to_v8(isolate, value.f);
        case IntTypeDouble:
            return v8pp::to_v8(isolate, value.d);
        case IntTypeString:
            return v8pp::to_v8(isolate, (const char*)value.string);
        case IntTypeInt:
        case IntTypeLong:
            return v8pp::to_v8(isolate, value.i);
        case IntTypeBool:
            return v8pp::to_v8(isolate, value.b);
        case IntTypeLargeString:
            return v8pp::to_v8(isolate, (const char*)value.largeString.pointer);
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
                        v8pp::set_const(isolate, object, "index", value.i);
                        v8pp::set_const(isolate, object, "name", render.data.comboBox.list->at(value.i));
                        return object;
                    }
                    }
                }
            }
            dbg("unhandled type error", type);
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
            port->value.f = v8pp::from_v8<float>(isolate, value);
            break;
        case IntTypeDouble:
            port->value.d = v8pp::from_v8<double>(isolate, value);
            break;
        case IntTypeString:
        {
            // dbg(port->value.string);
            std::string tmp = v8pp::from_v8<std::string>(isolate, value);
            auto size = std::size(port->value.string);
            if (tmp.size() >= size) {
                dbg("Warning!!  string is too large. It will be truncated.");
            }
            snprintf(port->value.string, size, "%s", tmp.c_str());
            break;
        }
        case IntTypeInt:
        case IntTypeLong:
            port->value.i = v8pp::from_v8<int>(isolate, value);
            break;
        case IntTypeBool:
            port->value.b = v8pp::from_v8<bool>(isolate, value);
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
                                port->value.i = static_cast<int>(std::distance(data.list->begin(), find));
                            } else {
                                flag = false;
                            }
                        } else if (IS_V8_NUMBER(value)) {
                            auto selected = v8pp::from_v8<int>(isolate, value);
                            if (selected < 0 || selected >= data.list->size()) {
                                flag = false;
                            } else {
                                port->value.i = selected;
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
                dbg("unhandled type error", port->getType());
            }
            break;
        }

        return flag;
    }

    Local<Value> nodePortValue(Isolate* isolate, SightNodePortHandle portHandle) {
        if (!portHandle) {
            return Undefined(isolate);
        }

        auto object = Object::New(isolate);
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
        auto deleteLinks = [portHandle, isolate]() {
            if (!portHandle) {
                return v8pp::to_v8(isolate, 0);
            }

            auto nodePort = portHandle.get();
            auto count = nodePort->clearLinks();

            return v8pp::to_v8(isolate, count);
        };
        auto show = [portHandle](bool v = true) {
            if (portHandle) {
                portHandle->options.show = v;
            }
        };

        auto errorMsg = [portHandle](const char* msg){
            if (portHandle) {
                portHandle->options.errorMsg = msg;
            }
        };

        auto readonly = [portHandle](bool v){
            if (portHandle) {
                portHandle->options.readonly = v;
            }
        };

        auto type = [portHandle](uint v) {
            if (portHandle) {
                if (v > 0) {
                    portHandle->type = v;
                } else {
                    return portHandle->type;
                }
            }
            return 0U;
        };
        auto resetType = [portHandle]() {
            if (portHandle) {
                auto p = portHandle.get();
                p->type = p->templateNodePort->type;
            }
        };

        auto port = portHandle.get();
        object->Set(context, v8pp::to_v8(isolate, "get"), v8pp::wrap_function(isolate, "get", get)).ToChecked();
        object->Set(context, v8pp::to_v8(isolate, "set"), v8pp::wrap_function(isolate, "set", set)).ToChecked();
        v8pp::set_const(isolate, object, "id", port->id);
        v8pp::set_const(isolate, object, "name", port->portName);
        v8pp::set_const(isolate, object, "deleteLinks", v8pp::wrap_function(isolate, "deleteLinks", deleteLinks));
        v8pp::set_const(isolate, object, "show", v8pp::wrap_function(isolate, "show", show));
        v8pp::set_const(isolate, object, "errorMsg", v8pp::wrap_function(isolate, "errorMsg", errorMsg));
        v8pp::set_const(isolate, object, "readonly", v8pp::wrap_function(isolate, "readonly", readonly));
        v8pp::set_const(isolate, object, "type", v8pp::wrap_function(isolate, "type", type));
        v8pp::set_const(isolate, object, "resetType", v8pp::wrap_function(isolate, "resetType", resetType));

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
        auto g = getCurrentGraph();

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
            bool isPart = false;
        };

        struct V8Runtime {
            std::unique_ptr<v8::Platform> platform;
            v8::Isolate *isolate = nullptr;
            v8::ArrayBuffer::Allocator *arrayBufferAllocator = nullptr;
            std::unique_ptr<v8::Isolate::Scope> isolateScope;
            SharedQueue<JsCommand> commandQueue;
        };

        struct GenerateFunctionStatus {
            int paramCount = 0;
            bool shouldEval = false;
            std::string returnBody;
            bool noReturn = false;
            bool need$ = false;
            bool need$$ = false;
        };

        //
        //    some util functions.
        //

        /**
         * @brief same as jsFunctionToString
         * 
         * @return std::string 
         */
        std::string functionProtoToString(Isolate* isolate, Local<Context> context, Local<Function> function){
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
                dbg("toString's result not a string");
                return {};
            }
        }


        //
        //   register functions to v8
        //

        void v8NodePortValue(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());
            auto rValue = args.GetReturnValue();
            if (!node || args.Length() <= 0) {
                rValue.SetUndefined();
                return;
            }

            auto arg1 = args[0];
            if (!IS_V8_STRING(arg1)) {
                rValue.SetUndefined();
                return;
            }

            std::string portName = v8pp::from_v8<std::string>(isolate, arg1);
            rValue.Set(nodePortValue(isolate, node, portName.c_str()));
        }

        void v8NodeTemplateAddress(FunctionCallbackInfo<Value> const& args) {
            auto isolate = args.GetIsolate();
            auto node = v8pp::class_<SightNode>::unwrap_object(isolate, args.This());

            const SightNode* t1 = node;
            if (node->templateNode) {
                args.GetReturnValue().Set(v8pp::to_v8(isolate, node->templateNode->fullTemplateAddress.c_str()));
            }
        }

        void checkTinyData(const char* key, Local<Object> data, Isolate* isolate){
            auto g = getCurrentGraph();
            if (!g) {
                dbg("need a graph opened!");
                return;
            }

            auto & map = g->getExternalData().tinyDataMap;
            if (!map.contains(key)) {
                // fixme: perhaps g_V8Runtime->isolate should be uiStatus->isolate
                map[key] = SightNodeGraphExternalData::V8ObjectType(isolate, data);
            }
        }

        MaybeLocal<Object> tinyData(const char* key, MaybeLocal<Object> mayData, Isolate* isolate) {
            auto g = getCurrentGraph();
            if (!g) {
                dbg("need a graph opened!");
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

        // print func
        void v8Print(const FunctionCallbackInfo<Value>& args) {
            auto isolate = args.GetIsolate();            
            auto context = isolate->GetCurrentContext();
            auto stackTrace = v8::StackTrace::CurrentStackTrace(isolate, 10, v8::StackTrace::kOverview);
            std::stringstream ss;
            if (stackTrace->GetFrameCount() > 0) {
                auto frame = stackTrace->GetFrame(isolate, 0);
                ss << "[";
                if (!frame->GetScriptName().IsEmpty()) {
                    ss << v8pp::from_v8<std::string>(isolate, frame->GetScriptName());
                }
                ss << ":";
                ss << frame->GetLineNumber();
                ss << " (";
                if (!frame->GetFunctionName().IsEmpty()) {
                    ss << v8pp::from_v8<std::string>(isolate, frame->GetFunctionName());
                }
                ss << ")] ";
            }
            auto traceMsg = ss.str();

            for( int i = 0; i < args.Length(); i++){
                auto arg = args[i];

                if (IS_V8_STRING(arg)) {
                    std::string msg = v8pp::from_v8<std::string>(isolate, arg);
                    printf("%s%s\n", traceMsg.c_str(), msg.c_str());
                } else if (IS_V8_NUMBER(arg)) {
                    if (arg->IsInt32() ) {
                        int msg = arg->Int32Value(context).ToChecked();
                        // dbg(msg);
                        printf("%s%d\n", traceMsg.c_str(), msg);
                    } else if (arg->IsUint32()) {
                        uint msg = arg->Uint32Value(context).ToChecked();
                        // dbg(msg);
                        printf("%s%d\n", traceMsg.c_str(), msg);
                    } else {
                        auto msg = arg->NumberValue(context).ToChecked();
                        // dbg(msg);
                        printf("%s%f\n", traceMsg.c_str(), msg);
                    }
                } else if (arg->IsObject()) {
                    // dbg("print object is waited for impl.");
                    auto str = v8pp::json_str(isolate, arg);
                    auto msg = str.c_str();
                    // dbg(msg);
                    printf("%s%s\n", traceMsg.c_str(), msg);
                } else if (arg->IsUndefined()) {
                    constexpr const char* msg = "undefined";
                    // dbg(msg);
                    printf("%s%s\n", traceMsg.c_str(), msg);
                } else if (arg->IsNull()) {
                    constexpr const char* msg = "null";
                    // dbg(msg);
                    printf("%s%s\n", traceMsg.c_str(), msg);
                } else if (arg->IsExternal()) {
                    constexpr const char* msg = "external";
                    // dbg(msg);
                    printf("%s%s\n", traceMsg.c_str(), msg);
                } else if (arg->IsBoolean() || arg->IsBooleanObject()) {
                    const char* msg = arg->BooleanValue(isolate) ? "true" : "false";
                    // dbg(msg);
                    printf("%s%s\n", traceMsg.c_str(), msg);
                }
                else {
                    dbg("no-handled-type");
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
                targetNode = getCurrentGraph()->findNode(id);
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
                            dbg("kind not a string");
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

                        dbg("unhandled option", keyString);
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
                    auto source = functionProtoToString(isolate, context, tempVal.As<Function>());
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

                    // if (!port.onValueChange.sourceCode.empty()) {
                    //     dbg(port.onValueChange.sourceCode);
                    // }
                }

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
                            dbg("empty function");
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
                        if (keyString == "enter") {
                            if (optionValue->IsBoolean() && optionValue->BooleanValue(isolate)) {
                                templateNode->options.processFlag = SightNodeProcessType::Enter;
                            }
                        } else if (keyString == "exit") {
                            if (optionValue->IsBoolean() && optionValue->BooleanValue(isolate)) {
                                templateNode->options.processFlag = SightNodeProcessType::Exit;
                            }
                        } 

                        else {
                            dbg("unknown option key", keyString);
                        }
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
                            dbg("function source not found", keyString);
                            continue;
                        }
                        dbg(sourceCode);
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
                std::move(isolate_scope),
        };

        logDebug("init js over.");
        return 0;
    }

    /**
     * destroy js engine
     * @return
     */
    int destroyJsEngine() {

        freeParser();

        if (!g_V8Runtime) {
            return -1;
        }

        clearJsNodeCache();
        dbg("destroy js engine..");
        g_V8Runtime->isolateScope.reset();
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
        return 0;
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

        // auto initTypes = Object::New(isolate);
        v8pp::module initTypes(isolate);

        initTypes.set_const("JS", MODULE_INIT_JS);
        initTypes.set_const( "UI", MODULE_INIT_UI);
        initTypes.set_const( "js", MODULE_INIT_JS);
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

        v8pp::class_<SightNodePortOptions> nodePortOptionsClass(isolate);
        nodePortOptionsClass
            .ctor<>()
            .set("show", &SightNodePortOptions::show)
            .set("showValue", &SightNodePortOptions::showValue)
            .set("errorMsg", &SightNodePortOptions::errorMsg);
        nodePortOptionsClass.auto_wrap_objects(true);
        module.set("SightNodePortOptions", nodePortOptionsClass);

        v8pp::class_<SightNodePort> nodePortClass(isolate);
        nodePortClass.ctor<>()
            .set("portName", v8pp::property(&SightNodePort::getPortName))
            .set("name", v8pp::property(&SightNodePort::getPortName))
            .set("id", v8pp::property(&SightNodePort::getId))
            .set("setKind", &SightNodePort::setKind)
            .set("options", &SightNodePort::options);
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
            .set("nodePortValue", &v8NodePortValue);
        module.set("SightNode", nodeClass);

    }

    v8::Isolate* getJsIsolate() {
        return g_V8Runtime->isolate;
    }

    /**
     * init js binds
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
        
        auto addTemplateNodeFunc = v8pp::wrap_function(isolate, "addTemplateNode", v8AddTemplateNode);
        context->Global()->Set(context, v8pp::to_v8(isolate, "addTemplateNode"), addTemplateNodeFunc).ToChecked();
        auto addTypeFunc = v8pp::wrap_function(isolate, "addType", &v8AddType);
        context->Global()->Set(context, v8pp::to_v8(isolate, "addType"), addTypeFunc).ToChecked();

        v8pp::class_<GenerateOptions> generateOptionsClass(isolate);
        generateOptionsClass
                .ctor<>()
                .set("isPart", &GenerateOptions::isPart)
                ;

        bindBaseFunctions(isolate, context, module);
        bindNodeTypes(isolate, context, module);

        context->Global()->Set(context, v8pp::to_v8(isolate, "sight"), module.new_instance()).ToChecked();
        logDebug("init js bindings over!");
        return 0;
    }

    // Extracts a C string from a V8 Utf8Value.
    const char* ToCString(const v8::String::Utf8Value& value) {
        return *value ? *value : "<string conversion failed>";
    }

    void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
        v8::HandleScope handle_scope(isolate);
        v8::String::Utf8Value exception(isolate, try_catch->Exception());
        const char* exception_string = ToCString(exception);
        v8::Local<v8::Message> message = try_catch->Message();
        if (message.IsEmpty()) {
            // V8 didn't provide any extra information about this error; just
            // print the exception.
            fprintf(stderr, "%s\n", exception_string);
        } else {
            // Print (filename):(line number): (message).
            v8::String::Utf8Value filename(isolate,
                                           message->GetScriptOrigin().ResourceName());
            v8::Local<v8::Context> context(isolate->GetCurrentContext());
            const char* filename_string = ToCString(filename);
            int linenum = message->GetLineNumber(context).FromJust();
            fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
            // Print line of source code.
            v8::String::Utf8Value sourceline(
                    isolate, message->GetSourceLine(context).ToLocalChecked());
            const char* sourceline_string = ToCString(sourceline);
            fprintf(stderr, "%s\n", sourceline_string);
            // Print wavy underline (GetUnderline is deprecated).
            int start = message->GetStartColumn(context).FromJust();
            for (int i = 0; i < start; i++) {
                fprintf(stderr, " ");
            }
            int end = message->GetEndColumn(context).FromJust();
            for (int i = start; i < end; i++) {
                fprintf(stderr, "^");
            }
            fprintf(stderr, "\n");
            v8::Local<v8::Value> stack_trace_string;
            if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
                stack_trace_string->IsString() &&
                v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
                v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
                const char* stack_trace_string = ToCString(stack_trace);
                fprintf(stderr, "%s\n", stack_trace_string);
            }
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

            pluginManager()->getPluginStatus().addNodesFinished = false;
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

            pluginManager()->getPluginStatus().addTemplateNodesFinished = false;
            addUICommand(UICommandType::AddTemplateNode, pointer, size);
        }

        if (promise) {
            promise->set_value(1);
        }

        dbg("flush node cache over!");
    }

    void clearJsNodeCache(){
        g_NodeCache.clear();
        g_TemplateNodeCache.clear();
    }

    std::string serializeJsNode(SightJsNode const& node) {
        // generate a function call.
        std::stringstream ss;
        ss << "addTemplateNode({\n";

        // name, address, nodes
        ss << "    __meta_name: \"" << node.nodeName << "\",\n";
        ss << "    __meta_address: \"" << node.fullTemplateAddress << "\",\n";

        ss << "    __meta_inputs: { \n";
        std::string spaces = "        ";
        std::string intervalSpaces = "    ";
        for (const auto & item : node.inputPorts) {
            ss << spaces << item->portName << ": '";
            ss << getTypeName(item->type);
            ss << "',\n";
        }

        spaces.erase(0, intervalSpaces.size());
        ss << spaces << "},\n";

        ss << "    __meta_outputs: { \n";
        spaces += intervalSpaces;
        for (const auto& item : node.outputPorts) {
            ss << spaces << item->portName << ": {\n";
            spaces += intervalSpaces;

            ss << spaces << "type: '" << getTypeName(item->type) << "',\n";
            ss << spaces << "showValue: true,\n";

            spaces.erase(0, intervalSpaces.size());
            ss  << spaces << "},\n";
        }
        spaces.erase(0, intervalSpaces.size());
        ss << spaces << "},\n";

        // functions
        auto writeFunction = [&ss, spaces,intervalSpaces](std::string const& name, v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> p) {
            if (!p.IsEmpty()) {
                auto code = jsFunctionToString(p.Get(g_V8Runtime->isolate));
                if (!code.empty()) {
                    ss << spaces << intervalSpaces << name << ": ";
                    ss << code;
                    ss << spaces << intervalSpaces << ",";
                }
            }
        };

        ss << spaces << "__meta_func: { \n";
        writeFunction("generateCodeWork", node.generateCodeWork);
        if (node.onReverseActive != node.generateCodeWork) {
            //
            writeFunction("onReverseActive", node.onReverseActive);
        }
        ss << spaces << "}, \n";

        for (const auto& item : node.fields) {
            ss << spaces << item->portName << ": '";
            ss << getTypeName(item->type);
            ss << "',\n";
        }

        ss << "});";
        return ss.str();
    }

    /**
     * run a js file.
     * @param filepath
     * @return
     */
    int runJsFile(v8::Isolate* isolate, const char* filepath, std::promise<int>* promise, v8::Local<v8::Object> module) {
        if (!isolate) {
            return CODE_FAIL;
        }
        dbg(filepath);

        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();

        TryCatch tryCatch(isolate);

        v8::Local<v8::String> sourceCode = readFile(isolate, filepath).ToLocalChecked();

        //

        Local<Integer> lineOffset = Integer::New(isolate, 0);
        ScriptOrigin scriptOrigin(v8pp::to_v8(isolate, filepath), lineOffset, lineOffset);
        v8::ScriptCompiler::Source source(sourceCode, scriptOrigin);
        Local<Object> context_extensions[0];
        MaybeLocal<Function> mayFunction;

        if (module.IsEmpty()) {
            mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 0, nullptr, 0, context_extensions);
        } else {
            v8::Local<v8::String> paramModule = v8::String::NewFromUtf8(isolate, "module").ToLocalChecked();
            v8::Local<v8::String> paramExports = v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked();
            v8::Local<v8::String> arguments[] { paramModule, paramExports};

            mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, std::size(arguments), arguments,
                                                                       0, context_extensions);
        }
        
        if (mayFunction.IsEmpty()) {
            dbg("js file compile error", filepath);
            if (promise) {
                promise->set_value(0);
            }

            if (tryCatch.HasCaught()) {
                //
                ReportException(isolate, &tryCatch);
            }
            return CODE_FILE_ERROR;
        }
        Local<Function> function = mayFunction.ToLocalChecked();
        Local<Object> recv = Object::New(isolate);
        MaybeLocal<Value> result;

        if (module.IsEmpty()) {
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
            dbg("js file has error", filepath);
            if (promise) {
                promise->set_value(0);
            }

            //
            ReportException(isolate, &tryCatch);
            return CODE_FAIL;
        }

        if (promise) {
            promise->set_value(1);
        }
        dbg("js file success ran.");
        return CODE_OK;
    }

    std::string analysisGenerateFunction(Isolate* isolate, Local<Function> function, Local<Context> context, GenerateFunctionStatus* status = nullptr){
        auto toString = function->Get(context, v8pp::to_v8(isolate, "toString")).ToLocalChecked();
        auto toStringFunc = toString.As<Function>();
        auto str = toStringFunc->Call(context, function, 0 , nullptr).ToLocalChecked();
        std::string code;
        if (IS_V8_STRING(str)) {
            code = v8pp::from_v8<std::string>(isolate, str);
        } else {
            dbg("toString's result not a string");
            return {};
        }

        auto p1 = code.find_first_of('{');
        auto p2 = code.find_last_of('}');
        auto body = removeComment(trimCopy(std::string(code, p1 + 1, p2 - p1 - 1)));

        if (status) {
            auto title = std::string(code, 0, p1);
            // dbg(title);
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


            auto pos = body.rfind(std::string("return"));
            if (pos != std::string::npos && pos + 6 < body.size()) {
                // has a return keyword
                auto left = trimCopy(std::string(body, pos + 6));
                if (startsWith(left, "function")) {
                    status->shouldEval = true;
                } else {
                    if (status->shouldEval) {
                        // It returns a statement or expr
                        auto commaPos = left.rfind(';');
                        if (commaPos != std::string::npos) {
                            // find last ;
                            status->returnBody = std::string (left, 0, commaPos);
                        } else {
                            status->returnBody = left;
                        }
                        // remove return stmt from body.
                        body.erase(pos);
                        trim(body);
                    }
                }
            } else {
                status->noReturn = true;
            }
        }

        return body;
    }

    MaybeLocal<Value> runGenerateCode(std::string const &functionCode ,Isolate* isolate, Local<Context> & context, SightNode* node,
                                      GenerateOptions* options = nullptr, GenerateFunctionStatus* status = nullptr){
        dbg(functionCode.c_str());
        if (status) {
            dbg(status->need$, status->need$$);
        }
        auto sourceCode = v8pp::to_v8(isolate, functionCode);
        ScriptCompiler::Source source(sourceCode);
        Local<Object> contextExt[0];
        v8::Local<v8::String> param$ = v8::String::NewFromUtf8(isolate, "$").ToLocalChecked();
        v8::Local<v8::String> paramOptions = v8::String::NewFromUtf8(isolate, "$$").ToLocalChecked();
        v8::Local<v8::String> arguments[] = {param$, paramOptions};
        auto targetFunction = ScriptCompiler::CompileFunctionInContext(context, &source, std::size(arguments), arguments, 0,contextExt).ToLocalChecked();

        // build args.
        auto arg$ = Object::New(isolate);
        auto arg$$ = Object::New(isolate);

        if (!status || status->need$) {
            auto nodeFunc = [node, isolate, &context, &arg$](std::vector<SightNodePort> & list, bool isOutput = false) {

                for (const auto &item : list) {
                    if (item.portName.empty()) {
                        continue;
                    }

                    std::string name = item.portName;
                    auto emptyFunc = [](){
                        return std::string();
                    };

                    auto t = [&item,node,isolate]() -> std::string  {
                        // reverseActive this port.
                        //
                        if (!item.isConnect()) {
                            return "";      // maybe need throw sth ?
                        }

                        auto & connections = item.connections;
                        if (connections.size() == 1) {
                            // 1
                            auto c = connections.front();
                            SightNodePortConnection connection(node->graph, c, node);
                            if (connection.bad()) {
                                dbg("bad connection");
                                return "";
                            }

                            auto targetNode = connection.target->node;
                            auto targetJsNode = findTemplateNode(targetNode);
                            if (targetJsNode) {
                                return runGenerateFunction(targetJsNode->onReverseActive,isolate, targetNode);
                            }
                        } else {

                        }
                        return "";
                    };

                    auto functionObject = isOutput ? v8pp::wrap_function(isolate, name, emptyFunc) : v8pp::wrap_function(isolate, name, t);
                    functionObject->Set(context, v8pp::to_v8(isolate, "value"), getPortValue(isolate, item.getType(), item.value)).ToChecked();
                    functionObject->Set(context, v8pp::to_v8(isolate, "isConnect"), v8pp::to_v8(isolate, item.isConnect())).ToChecked();

                    arg$->Set(context, v8pp::to_v8(isolate, name), functionObject).ToChecked();
                }

            };

            nodeFunc(node->inputPorts);
            nodeFunc(node->fields);

            nodeFunc(node->outputPorts, true);
        }

        if (!status || status->need$$) {
            if (options) {
                auto jsOptions = v8pp::class_<GenerateOptions>::reference_external(isolate, options);
                arg$$->Set(context, v8pp::to_v8(isolate, "options"), jsOptions).ToChecked();
            }
        }

        Local<Object> recv = Object::New(isolate);
        Local<Value> args[2];
        args[0] = arg$;
        args[1] = arg$$;
        auto result = targetFunction->Call(context, recv,std::size(args), args);

        if (options) {
            v8pp::class_<GenerateOptions>::unreference_external(isolate, options);
        }

        return result;
    }


    std::string runGenerateFunction(PersistentFunction const& persistent, Isolate* isolate, SightNode* node) {
        if (persistent.IsEmpty()) {
            // no function ?
            dbg("no function");
            return "";
        }

        auto function = persistent.Get(isolate);
        auto context = isolate->GetCurrentContext();

        GenerateFunctionStatus status;
        std::string functionCode = analysisGenerateFunction(isolate, function, context, &status);
        MaybeLocal<Value> resultMaybe;
        GenerateOptions options;
        if (status.shouldEval) {
            // The code need be eval first.
            if (!functionCode.empty()) {
                resultMaybe = runGenerateCode(functionCode, isolate, context, node, &options, &status);
            }

            if (status.noReturn) {
                functionCode = "";
            } else if (status.returnBody.empty()) {
                // expect `result` is a function
                if (!resultMaybe.IsEmpty()) {
                    auto firstRunResult = resultMaybe.ToLocalChecked();
                    if (firstRunResult->IsFunction()) {
                        functionCode = analysisGenerateFunction(isolate, firstRunResult.As<Function>(), context);
                    }
                }
            } else {
                // then, parse return body.
                functionCode = status.returnBody;
            }
        }

        if (!functionCode.empty()) {
            std::stringstream ss;
            bool dollar = false;
            ss << "return `";
            for (const auto &item : functionCode) {
                if (!dollar) {
                    if (item == '$') {
                        dollar = true;
                        ss << "${";
                    }
                } else {
                    if (isalpha(item) || isnumber(item) || item == '.' || item == '_'
                        || item == '(' || item == ')' || item == '$') {

                    } else {
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
            ss << '`';

            status = {
                    .need$ = true,
                    .need$$ = false,
            };
            auto result = runGenerateCode(ss.str(), isolate, context, node, nullptr, &status);
            if (result.IsEmpty()) {
                dbg("result is empty", ss.str());
            } else {
                auto resultString = v8pp::from_v8<std::string>(isolate, result.ToLocalChecked());
                return resultString;
            }
        }

        return "";
    }

    void parseNode(std::vector<SightNode*> & list, Isolate* isolate, std::stringstream & finalSource){
        auto node = list.front();
        auto jsNode = findTemplateNode(node);
        list.erase(list.begin());


        // generateCodeWork
        std::string tmp = runGenerateFunction(jsNode->generateCodeWork, isolate, node);
        if (!tmp.empty()) {
            finalSource << tmp;
        }

        // active the next.
        auto connection = node->findConnectionByProcess();
        if (connection.bad()) {
            dbg("bad connection", node->nodeName);
            return;
        }

        list.push_back(connection.target->node);
    }

    /**
     *
     * @param filename
     * @return
     */
    int parseGraph(const char* filename){
        dbg(filename);
        SightNodeGraph graph;
        int i = graph.load(filename, false);
        if (i == 1) {
            return 1;
        }
        if (i < 0) {
            return 2;
        }

        // get enter node.
        int status = -1;
        auto node = graph.findNode(SightNodeProcessType::Enter, &status);
        if (status != 0) {
            dbg("error graph",filename, status);
            return 3;
        }

        // here it is.
        auto isolate = g_V8Runtime->isolate;
        v8::HandleScope handle_scope(isolate);

        std::vector<SightNode*> list;
        list.push_back(node);
        std::stringstream finalSourceStream;

        while (!list.empty()) {
            parseNode(list, isolate, finalSourceStream);
        }

        std::string finalSource = finalSourceStream.str();
        trim(finalSource);
        dbg(finalSource);
//        dbg(finalSource.length(), finalSource.size());
//        dbg(finalSource.data());
        dbg(finalSource.c_str());
//
//        printf("%s\n", finalSource.c_str());
//        for (const auto &item : finalSource) {
//            printf("%d",item);
//        }
//        printf("\n");

        return 0;
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
                goto break_commands_loop;
            case JsCommandType::File:
                runJsFile(g_V8Runtime->isolate, command.args.argString, nullptr);
                flushJsNodeCache(command.args.promise);
                break;
            case JsCommandType::JsCommandHolder:
                // do nothing.
                break;
            case JsCommandType::ParseGraph:
                parseGraph(command.args.argString);
                break;
            case JsCommandType::InitPluginManager:
            {
                pluginManager()->init(g_V8Runtime->isolate);
                pluginManager()->loadPlugins();
                break;
            }
            case JsCommandType::RunFile:
                runJsFile(g_V8Runtime->isolate, command.args.argString, command.args.promise);
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
            default:
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

        {
            auto isolate = g_V8Runtime->isolate;
            v8::HandleScope handle_scope(isolate);
            v8::Local<v8::Context> context = v8::Context::New(isolate);
            //         Enter the context
            v8::Context::Scope context_scope(context);
            initJsBindings(isolate, context);

            runJsCommands();
        }

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

            if (value->IsFunction()) {
                // only register function
                if (outFunctionCode) {
                    // out code
                    auto function = value.As<Function>();
                    outFunctionCode->insert({ keyString, functionProtoToString(isolate, context, function) });
                }

                if (isValid(global->Get(context, key))) {
                    dbg("repeat key, ignore this one", keyString);
                    continue;
                }

                if (!global->Set(context, key, value).ToChecked()) {
                    dbg("set failed", keyString);
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
}
