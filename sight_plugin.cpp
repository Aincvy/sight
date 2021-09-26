//
// Created by Aincvy(aincvy@gmail.com) on 2021/9/17.
//

#include "sight_plugin.h"
#include "sight.h"
#include "filesystem"
#include "sight_util.h"
#include "sight_js.h"

#include "v8pp/convert.hpp"

#define FILE_NAME_PACKAGE "package.js"

namespace sight {

    using directory_iterator = std::filesystem::directory_iterator;

    namespace {
        PluginManager* g_pluginManager = new PluginManager();

    }

    PluginManager::~PluginManager() {
        //todo free plugin memory.

    }

    int PluginManager::init(v8::Isolate* isolate) {
        if (this->isolate) {
            return CODE_OK;
        }

        // init v8
        this->isolate = isolate;
        return CODE_OK;
    }

    int PluginManager::loadPlugins() {
        dbg("load plugins");

        for (auto it = directory_iterator {"./plugins"}; it != directory_iterator {}; ++it) {
            auto plugin = new Plugin(this, it->path().string());
            if (plugin->load() != CODE_OK) {
                dbg("plugin load fail", it->path().c_str());
                goto loadFailed;
            }

            plugin->debugBrieflyInfo();
            flushJsNodeCache();
            this->pluginMap[plugin->getName()] = plugin;

            continue;
            loadFailed:
            clearJsNodeCache();
            delete plugin;
        }

        return 0;
    }

    v8::Isolate *PluginManager::getIsolate() {
        return this->isolate;
    }

    PluginManager::PluginManager() {
        // this->searchPaths.emplace_back("./plugins");
        this->searchPaths.emplace_back("./lib-plugins");
    }

    void PluginManager::addSearchPath(const char *path) {
        this->searchPaths.emplace_back(path);
    }

    Plugin::Plugin(PluginManager* pluginManager,std::string name, std::string path, std::string version, std::string author)
        : name(std::move(name)), path(std::move(path)), version(std::move(version)), author(std::move(author)), pluginManager(pluginManager)
    {

    }

    void Plugin::debugBrieflyInfo() const{
        dbg(name, path, version, author);
    }

    Plugin::Plugin(PluginManager* pluginManager, std::string path) : path(std::move(path)), pluginManager(pluginManager) {

    }

    int Plugin::loadFromFile() {
        // todo load single file as this plugin

        return CODE_OK;
    }

    int Plugin::loadFromFolder() {

        // load `package.js` first.
        //
        std::string pkgJsPath = path + FILE_NAME_PACKAGE;
        auto isolate = this->pluginManager->getIsolate();
        auto sourceCode = readFile(isolate, pkgJsPath.c_str());
        if (sourceCode.IsEmpty()) {
            return CODE_PLUGIN_NO_PKG_FILE;
        }

        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
        v8::ScriptCompiler::Source source(sourceCode.ToLocalChecked());
        auto mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 0, nullptr, 0, nullptr);
        if (mayFunction.IsEmpty()) {
            return CODE_PLUGIN_COMPILE_FAIL;
        }

        auto function = mayFunction.ToLocalChecked();
        v8::Local<v8::Object> recv = v8::Object::New(isolate);
        auto mayResult = function->Call(context, recv, 0, nullptr);

        auto mayInfoObject = recv->Get(context, v8pp::to_v8(isolate, "info"));
        if (!mayInfoObject.IsEmpty()) {
            auto infoObject = mayInfoObject.ToLocalChecked();
            if (infoObject->IsObject()) {
                readBaseInfoFromJsObject(infoObject.As<v8::Object>());
            }
        } else {
            if (mayResult.IsEmpty()) {
                return CODE_PLUGIN_NO_DESCRIPTION;
            }

            auto result = mayResult.ToLocalChecked();
            if (!result->IsObject()) {
                return CODE_PLUGIN_NO_DESCRIPTION;
            }
            auto object = result.As<v8::Object>();
            readBaseInfoFromJsObject(object);
        }

        // load other files.
        directory_iterator begin{ this->path}, end{};
        for (auto it = begin; it != end ; ++it) {
            if (!it->is_regular_file()) {
                continue;
            }

            if (!it->path().has_extension() || it->path().extension() != ".js"){
                continue;
            }
            if (it->path().filename() == FILE_NAME_PACKAGE)
            {
                continue;
            }
            

            auto fullPath = std::filesystem::canonical(it->path());
            auto code = runJsFile(fullPath.c_str());
            if (code != 0) {
                dbg("js run failed", fullPath.c_str());
                return CODE_PLUGIN_FILE_ERROR;
            }
        }

        return CODE_OK;
    }

    int Plugin::load() {
        if (!std::filesystem::exists(path)) {
            CODE_FAIL;
        }

        if (!endsWith(path, "/")) {
            path += "/";
        }

        if (std::filesystem::is_directory(path)) {
            return loadFromFolder();
        } else if (std::filesystem::is_regular_file(path)) {
            return loadFromFile();
        }

        return CODE_OK;
    }

    PluginManager *Plugin::getPluginManager() {
        return this->pluginManager;
    }

    const PluginManager *Plugin::getPluginManager() const {
        return this->pluginManager;
    }

    void Plugin::readBaseInfoFromJsObject(v8::Local<v8::Object> object) {
        // js code example
//        return {
//                name: 'aa',
//                version: '1.0',
//                author: 'xxx',
//        };

        auto isolate = this->pluginManager->getIsolate();
        auto context = isolate->GetCurrentContext();
        auto keys = object->GetOwnPropertyNames(context).ToLocalChecked();

        for (int i = 0; i < keys->Length(); ++i) {
            auto keyObject = keys->Get(context, i).ToLocalChecked();
            auto valueObject = object->Get(context, keyObject).ToLocalChecked();

            std::string key = v8pp::from_v8<std::string>(isolate, keyObject);

            if (key == "name") {
                if (IS_V8_STRING(valueObject)) {
                    this->name = v8pp::from_v8<std::string>(isolate, valueObject);
                }
            } else if (key == "version") {
                if (IS_V8_STRING(valueObject)) {
                    this->version = v8pp::from_v8<std::string>(isolate, valueObject);
                }
            } else if (key == "author") {
                if (IS_V8_STRING(valueObject)) {
                    this->author = v8pp::from_v8<std::string>(isolate, valueObject);
                }
            }

        }

    }

    std::string const & Plugin::getName() const {
        return this->name;
    }


    PluginManager *pluginManager() {
        return g_pluginManager;
    }

}