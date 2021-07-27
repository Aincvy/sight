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

namespace sight {
    namespace {
        struct V8Runtime;
    }
}
static sight::V8Runtime* g_V8Runtime = nullptr;

// cache for js script, it will send to ui thread once
static std::vector<sight::SightNode> g_NodeCache;

namespace sight {


    namespace  {

        struct V8Runtime {
            std::unique_ptr<v8::Platform> platform;
            v8::Isolate* isolate = nullptr;
            v8::ArrayBuffer::Allocator* arrayBufferAllocator = nullptr;
            std::unique_ptr<v8::Isolate::Scope> isolateScope;
            SharedQueue<JsCommand> commandQueue;
        };

        // use it for register to v8.
        struct NodePortTypeStruct{
            int getInput() const {
                return NodePortType::Input;
            }

            int getOutput() const{
                return NodePortType::Output;
            }

            int getBoth() const{
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
        v8::MaybeLocal<v8::String> readFile(v8::Isolate* isolate, const char* name){
            FILE* file = fopen(name, "rb");
            if (file == NULL) {
                return {};
            }

            fseek(file, 0, SEEK_END);
            size_t size = ftell(file);
            rewind(file);

            char* chars = new char[size + 1];
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
        void v8rPrint(const char*msg) {
            printf("%s\n", msg);

        }

        void v8AddNode(SightJsNode & node){
            g_NodeCache.push_back(node);
        }

    }

    void testJs(char *arg1) {
        // Initialize V8.
        v8::V8::InitializeICUDefaultLocation(arg1);
        v8::V8::InitializeExternalStartupData(arg1);
        std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
        v8::V8::InitializePlatform(platform.get());
        v8::V8::Initialize();
        // Create a new Isolate and make it the current one.
        v8::Isolate::CreateParams create_params;
        create_params.array_buffer_allocator =
                v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        v8::Isolate *isolate = v8::Isolate::New(create_params);
        {
            v8::Isolate::Scope isolate_scope(isolate);
            // Create a stack-allocated handle scope.
            v8::HandleScope handle_scope(isolate);
            // Create a new context.
            v8::Local<v8::Context> context = v8::Context::New(isolate);
            // Enter the context for compiling and running the hello world script.
            v8::Context::Scope context_scope(context);
            {
                // Create a string containing the JavaScript source code.
                v8::Local<v8::String> source =
                        v8::String::NewFromUtf8Literal(isolate, "'Hello' + ', World!'");
                // Compile the source code.
                v8::Local<v8::Script> script =
                        v8::Script::Compile(context, source).ToLocalChecked();
                // Run the script to get the result.
                v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
                // Convert the result to an UTF8 string and print it.
                v8::String::Utf8Value utf8(isolate, result);
                printf("%s\n", *utf8);
            }
            {
                // Use the JavaScript API to generate a WebAssembly module.
                //
                // |bytes| contains the binary format for the following module:
                //
                //     (func (export "add") (param i32 i32) (result i32)
                //       get_local 0
                //       get_local 1
                //       i32.add)
                //
                const char csource[] = R"(
        let bytes = new Uint8Array([
          0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07, 0x01,
          0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
          0x07, 0x01, 0x03, 0x61, 0x64, 0x64, 0x00, 0x00, 0x0a, 0x09, 0x01,
          0x07, 0x00, 0x20, 0x00, 0x20, 0x01, 0x6a, 0x0b
        ]);
        let module = new WebAssembly.Module(bytes);
        let instance = new WebAssembly.Instance(module);
        instance.exports.add(3, 4);
      )";

                // Create a string containing the JavaScript source code.
                v8::Local<v8::String> source =
                        v8::String::NewFromUtf8Literal(isolate, csource);
                // Compile the source code.
                v8::Local<v8::Script> script =
                        v8::Script::Compile(context, source).ToLocalChecked();
                // Run the script to get the result.
                v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
                // Convert the result to a uint32 and print it.
                uint32_t number = result->Uint32Value(context).ToChecked();
                printf("3 + 4 = %u\n", number);
            }
        }
        // Dispose the isolate and tear down V8.
        isolate->Dispose();
        v8::V8::Dispose();
        v8::V8::ShutdownPlatform();
        delete create_params.array_buffer_allocator;
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

        printf("destroy js engine..");
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
    int initJsBindings(const v8::Local<v8::Context> & context) {
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
                .set("Field", v8pp::property(&NodePortTypeStruct::getField))
                ;
        module.set("NodePortType", jsNodePortTypeEnum);

        v8pp::class_<SightNodePort> jsNodePortClass(isolate);
        jsNodePortClass.ctor<>()
                .set("portName", &SightNodePort::portName)
                .set("id", &SightNodePort::id)
                .set("intKind", &SightNodePort::intKind)
                .set("updateStatus", &SightNodePort::updateStatus)
                ;
        module.set("SightNodePort", jsNodePortClass);

        v8pp::class_<SightJsNode> jsNodeClass(isolate);
        jsNodeClass
            .ctor<>()
            .set("nodeName", &SightJsNode::nodeName)
            .set("nodeId", &SightJsNode::nodeId)
            .set("addPort", &SightJsNode::addPort)
        ;
        module.set("SightJsNode", jsNodeClass);

        // functions ...
        module.set("nextNodeOrPortId", &nextNodeOrPortId);
        module.set("addNode", &v8AddNode);

        auto currentContext = context;
        currentContext->Global()->Set(currentContext, v8pp::to_v8(isolate, "sight"), module.new_instance());

        auto printFunc = v8pp::wrap_function(isolate, "print", &v8rPrint);
        currentContext->Global()->Set(currentContext, v8pp::to_v8(isolate, "print"), printFunc);

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

        v8::Local<v8::String> source = readFile(isolate, filepath).ToLocalChecked();
        v8::MaybeLocal<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
        v8::MaybeLocal<v8::Value> result = script.ToLocalChecked()->Run(context);

        //
        // v8::ScriptCompiler::CompileFunctionInContext()

        // send nodes to ui thread.
        if (!g_NodeCache.empty()) {
            auto size = g_NodeCache.size();
            auto* nodePointer = (SightNode**)malloc(size * sizeof(void*));
//            auto* nodePointer = new SightNode*[size];
            int index = 0;
            for (const auto &item : g_NodeCache) {
                // copy item to array
                nodePointer[index++] = item.clone();
            }
            g_NodeCache.clear();

            addUICommand(AddNode, nodePointer, size);
        }

        return 0;
    }


    void runJsCommands(){
        if (!g_V8Runtime) {
            return;
        }

        auto& queue = g_V8Runtime->commandQueue;
        while(true) {
            auto command = queue.front();

            switch (command.type) {
                case Destroy:
                    goto break_commands_loop;
                case File:
                    runJsFile(command.args.argString);
                    break;
                case JsCommandHolder:
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


