//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
#include <stdio.h>
#include <thread>

#include "sight_js.h"
#include "sight_node_editor.h"
#include "shared_queue.h"
#include "sight_log.h"
#include "sight_ui.h"

#include "v8.h"
#include "libplatform/libplatform.h"

#include "v8pp/module.hpp"
#include "v8pp/class.hpp"

using namespace v8;

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


    namespace {

        struct V8Runtime {
            std::unique_ptr<v8::Platform> platform;
            v8::Isolate *isolate = nullptr;
            v8::ArrayBuffer::Allocator *arrayBufferAllocator = nullptr;
            std::unique_ptr<v8::Isolate::Scope> isolateScope;
            SharedQueue<JsCommand> commandQueue;
        };

        // use it for register to v8.
        struct NodePortTypeStruct {
            int getInput() const {
                return NodePortType::Input;
            }

            int getOutput() const {
                return NodePortType::Output;
            }

            int getBoth() const {
                return NodePortType::Both;
            }

            int getField() const {
                return NodePortType::Field;
            }
        };

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


        //
        //   register functions to v8
        //

        // print func
        void v8rPrint(const char *msg) {
            printf("%s\n", msg);

        }

        void v8AddNode(SightJsNode &node) {
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
            auto templateNode = new SightNode();
            std::string address;

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
                        templateNode->addPort({
                                                      v8pp::from_v8<std::string>(isolate, inputKey),
                                                      -1,
                                                      NodePortType::Input
                                              });
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
                        templateNode->addPort({
                                                      v8pp::from_v8<std::string>(isolate, outputKey),
                                                      -1,
                                                      NodePortType::Output
                                              });
                    }
                } else {
                    // fields

                }
            }

            //
            g_TemplateNodeCache.push_back({
                address,
                templateNode,
            });
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

        logDebug("init js over.\n");
        return 0;
    }

    /**
     * destroy js engine
     * @return
     */
    int destroyJsEngine() {
        if (!g_V8Runtime) {
            return -1;
        }

        dbg("destroy js engine..");
        g_V8Runtime->isolateScope.reset();
        if (g_V8Runtime->isolate) {
            g_V8Runtime->isolate->Dispose();
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

    /**
     * init js binds
     * cpp struct, functions to js engine.
     * @return
     */
    int initJsBindings(const v8::Local<v8::Context> &context) {
        if (!g_V8Runtime) {
            return -1;
        }

        auto isolate = g_V8Runtime->isolate;
        v8pp::module module(isolate);


        // classes ...
        v8pp::class_<NodePortTypeStruct> jsNodePortTypeEnum(isolate);
        jsNodePortTypeEnum.ctor<>()
                .set("Input", v8pp::property(&NodePortTypeStruct::getInput))
                .set("Output", v8pp::property(&NodePortTypeStruct::getOutput))
                .set("Both", v8pp::property(&NodePortTypeStruct::getBoth))
                .set("Field", v8pp::property(&NodePortTypeStruct::getField));
        module.set("NodePortType", jsNodePortTypeEnum);

        v8pp::class_<SightNodePort> jsNodePortClass(isolate);
        jsNodePortClass.ctor<>()
                .set("portName", &SightNodePort::portName)
                .set("id", &SightNodePort::id)
                .set("setKind", &SightNodePort::setKind);
        module.set("SightNodePort", jsNodePortClass);

        v8pp::class_<SightJsNode> jsNodeClass(isolate);
        jsNodeClass
                .ctor<>()
                .set("nodeName", &SightJsNode::nodeName)
                .set("nodeId", &SightJsNode::nodeId)
                .set("addPort", &SightJsNode::addPort);
        module.set("SightJsNode", jsNodeClass);

        // functions ...
        module.set("nextNodeOrPortId", &nextNodeOrPortId);
        module.set("addNode", &v8AddNode);

        auto currentContext = context;
        currentContext->Global()->Set(currentContext, v8pp::to_v8(isolate, "sight"), module.new_instance());

        auto printFunc = v8pp::wrap_function(isolate, "print", &v8rPrint);
        currentContext->Global()->Set(currentContext, v8pp::to_v8(isolate, "print"), printFunc);
        auto addTemplateNodeFunc = v8pp::wrap_function(isolate, "addTemplateNode", v8AddTemplateNode);
        currentContext->Global()->Set(currentContext, v8pp::to_v8(isolate, "addTemplateNode"), addTemplateNodeFunc);

        logDebug("init js bindings over!\n");
        return 0;
    }

    /**
     * run a js file.
     * @param filepath
     * @return
     */
    int runJsFile(const char *filepath) {
        if (!g_V8Runtime) {
            return -1;
        }

        auto isolate = g_V8Runtime->isolate;
        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
//        auto context = v8::Context::New(isolate);
//        v8::Context::Scope contextScope(context);
//        printf(context.IsEmpty() ? "empty\n" : "not empty\n");

        v8::Local<v8::String> sourceCode = readFile(isolate, filepath).ToLocalChecked();

        //
        v8::Local<v8::String> module = v8::String::NewFromUtf8(isolate, "module").ToLocalChecked();
        v8::Local<v8::String> exports = v8::String::NewFromUtf8(isolate, "exports").ToLocalChecked();
        v8::Local<v8::String> params = v8::String::NewFromUtf8(isolate, "params").ToLocalChecked();
        v8::Local<v8::String> arguments[] = {module, exports, params};
        v8::ScriptCompiler::Source source(sourceCode);
        Local<Object> context_extensions[0];
        Local<Function> function = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 3, arguments,
                                                                                0, context_extensions).ToLocalChecked();
        Local<Object> recv = Object::New(isolate);
        recv->Set(context, String::NewFromUtf8(isolate, "test").ToLocalChecked(),
                  String::NewFromUtf8(isolate, "value-from-cpp").ToLocalChecked());

        Local<Value> args[3];
        args[0] = Object::New(isolate);
        args[1] = Object::New(isolate);
        args[2] = Object::New(isolate);
        MaybeLocal<Value> result = function->Call(context, recv, 3, args);

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

            addUICommand(UICommandType::AddTemplateNode, pointer, size);
        }

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
                    runJsFile(command.args.argString);
                    break;
                case JsCommandType::JsCommandHolder:
                    // do nothing.
                    break;
            }

            command.args.dispose();
            queue.pop_front();
        }

        break_commands_loop:
        destroyJsEngine();
    }


    void jsThreadRun(const char *exeName) {
        initJsEngine(exeName);

        auto isolate = g_V8Runtime->isolate;
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        //         Enter the context
        v8::Context::Scope context_scope(context);
        initJsBindings(context);

        runJsCommands();
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

    int addJsCommand(JsCommandType type, const char *argString, int argStringLength, bool argStringNeedFree) {
        JsCommand command = {
                type,
                {
                        argString,
                        argStringLength,
                        argStringNeedFree,
                },
        };

        return addJsCommand(command);
    }

    int addJsCommand(JsCommand &command) {
        if (!g_V8Runtime) {
            return -1;
        }

        g_V8Runtime->commandQueue.push_back(command);
        return 0;
    }


}


