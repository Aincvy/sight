//
// Created by Aincvy(aincvy@gmail.com) on 2021/9/17.
//

#include "sight_plugin.h"
#include "sight.h"
#include "filesystem"
#include "sight_defines.h"
#include "sight_project.h"
#include "sight_ui.h"
#include "sight_util.h"
#include "sight_js.h"

#include "v8-context.h"
#include "v8-local-handle.h"
#include "v8pp/convert.hpp"
#include "v8pp/call_v8.hpp"
#include "v8pp/object.hpp"
#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#ifdef NOT_WIN32
#    include <unistd.h>
#endif

#ifdef _WIN32
#    define strdup _strdup
#endif

#define FILE_NAME_PACKAGE "package.js"
#define FILE_NAME_EXPORTS_UI "exports-ui.js"


namespace sight {

    namespace fs = std::filesystem;
    using directory_iterator = fs::directory_iterator;

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

    void PluginManager::afterPluginLoadSuccess(Plugin* plugin) {
        plugin->debugBrieflyInfo();
        flushJsNodeCache();
        this->pluginMap[plugin->getName()] = plugin;
        if (plugin->getPluginStatus() == PluginStatus::Loaded) {
            ++loadedPluginCount;
        }
    };

    int PluginManager::loadPlugins() {
        logDebug("load plugins");
        loadedPluginCount = 0;

        // load sight-base first.
        fs::path sightBasePath{ "./plugins/sight-base" };
        if (fs::exists(sightBasePath)) {
            auto plugin = new Plugin(this, sightBasePath.string());
            plugin->load();
            afterPluginLoadSuccess(plugin);
        }

        for (auto it = directory_iterator{ "./plugins" }; it != directory_iterator{}; ++it) {
            loadPluginAt(it->path().string());
        }

        return 0;
    }

    int PluginManager::loadPluginAt(std::string_view path) {

        fs::path sightBasePath{ "./plugins/sight-base" };

        if (fs::equivalent(sightBasePath, path)) {
            return CODE_FAIL;
        }

        auto plugin = new Plugin(this, std::string(path));
        int i = CODE_OK;
        bool fail = false;
        do {
            i = plugin->load();
            if (i != CODE_OK && i != CODE_PLUGIN_DISABLED_BY_PROJECT) {
                if (i != CODE_PLUGIN_DISABLED) {
                    logDebug("plugin load fail: $0", path);
                }
                fail = true;
                break;
            }

            if (pluginMap.contains(plugin->getName())) {
                logDebug("maybe name repeat: $0", plugin->getName());
                fail = true;
                break;
            }

            afterPluginLoadSuccess(plugin);
        } while (false);

        if (fail) {
            clearJsNodeCache();
            delete plugin;
            plugin = nullptr;
        } 

        return i;
    }

    absl::flat_hash_map<std::string, Plugin*> & PluginManager::getPluginMap() {
        return this->pluginMap;
    }

    std::vector<std::string> const& PluginManager::getSortedPluginNames() const {
        return sortedPluginNames;
    }

    absl::flat_hash_map<std::string, Plugin*> const& PluginManager::getPluginMap() const {
        return this->pluginMap;
    }

    uint PluginManager::getLoadedPluginCount() const {
        return loadedPluginCount;
    }

    int PluginManager::enablePlugin(std::string_view name) {    
        auto plugin = pluginMap.find(name);
        if (plugin == pluginMap.end()) {
            return CODE_PLUGIN_NOT_FOUND;
        }

        v8::HandleScope handle_scope(isolate);
        auto context = isolate->GetCurrentContext();
        return plugin->second->justLoad(context);
    }

    int PluginManager::disablePlugin(std::string_view name, bool fromProject) {
        auto plugin = pluginMap.find(name);
        if (plugin == pluginMap.end()) {
            return CODE_PLUGIN_NOT_FOUND;
        }

        return plugin->second->disablePlugin(fromProject);
    }

    std::map<std::string, const Plugin*> const& PluginManager::getSnapshotMap(){
        if (snapshotMap.size() != pluginMap.size()) {
            snapshotMap.clear();
            auto & sortedNames = sortedPluginNames;
            sortedPluginNames.clear();
            std::transform(pluginMap.begin(), pluginMap.end(), std::back_inserter(sortedNames), [](auto const& a){ return a.first;});

            std::sort(sortedNames.begin(), sortedNames.end(), [this](auto const& a, auto const& b) {
                Plugin* t1 = pluginMap[a];
                Plugin* t2 = pluginMap[b];
                if ((t1->getPluginStatus() != PluginStatus::Disabled && t2->getPluginStatus() != PluginStatus::Disabled)
                    || (t1->getPluginStatus() == PluginStatus::Disabled && t2->getPluginStatus() == PluginStatus::Disabled)) {
                    return a < b;
                } else if (t2->getPluginStatus() == PluginStatus::Disabled) {
                    return true;
                }
                return false;
            });

            for( const auto& [name, plugin]: pluginMap){
                snapshotMap[name] = plugin;
            }

        }
        return this->snapshotMap;
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
        logDebug("$0, $1, $2, $3", name, path, version, author);
    }

    Plugin::Plugin(PluginManager* pluginManager, std::string path) : path(std::move(path)), pluginManager(pluginManager) {

    }

    int Plugin::loadFromFile() {
        // todo load single file as this plugin

        return CODE_NOT_IMPLEMENTED;
    }

    int Plugin::loadFromFolder() {
        status = PluginStatus::Loading;
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
        auto mayFunction = v8::ScriptCompiler::CompileFunction(context, &source, 0, nullptr);
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
        if (this->disabled) {
            logDebug("plugin disabled: $0", this->name);
            status = PluginStatus::Disabled;
            return CODE_PLUGIN_DISABLED;
        }

        auto project = currentProject();
        if (project) {
            // check disabled by project?
            if (project->getDisabledPluginNames().contains(this->name)) {
                logDebug("plugin disabled by project: $0", this->name);
                this->disabledByProject = true;
                this->status = PluginStatus::Disabled;
                return CODE_PLUGIN_DISABLED_BY_PROJECT;
            }
        }

        return justLoad(context);
    }

    int Plugin::load() {
        if (!std::filesystem::exists(path)) {
            return CODE_FAIL;
        }

        if (!endsWith(path, "/")) {
            path += "/";
        }

        if (std::filesystem::is_directory(path)) {
            return loadFromFolder();
        } else if (std::filesystem::is_regular_file(path)) {
            return loadFromFile();
        }

        return CODE_PLUGIN_NO_PKG_FILE;
    }

    PluginManager *Plugin::getPluginManager() {
        return this->pluginManager;
    }

    const PluginManager *Plugin::getPluginManager() const {
        return this->pluginManager;
    }

    void Plugin::readBaseInfoFromJsObject(v8::Local<v8::Object> object) {
        // js code example
        // this.info = {
        //     name: 'dot-plugin',
        //     version: '-',
        //     author: '-',
        //     url: 'https://github.com/olado/doT/tree/v2',
        //     reloadAble: false,
        //     disabled: false,
        //     loadFiles: [],     // extra load files; only load `main.js`, `exports.js` by default.
        //     depends: [],
        // };

        // init something
        loadFiles.clear();
        loadFiles.push_back("main.js");
        loadFiles.push_back("exports.js");

        depends.clear();

        auto isolate = this->pluginManager->getIsolate();
        auto context = isolate->GetCurrentContext();

        auto loadStringArray = [isolate](std::vector<std::string>& list, v8::Local<v8::Value> valueObject) {
            if (IS_V8_STRING(valueObject)) {
                std::string str = v8pp::from_v8<std::string>(isolate, valueObject);
                addIfNotExist<std::string>(list, str);
            } else if (valueObject->IsArray()) {
                // expect array of strings.
                auto tmpList = v8pp::from_v8<std::vector<std::string>>(isolate, valueObject);
                for( const auto& item: tmpList){
                    addIfNotExist(list, item);
                }
            }
        };

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
            } else if (key == "url") {
                if (IS_V8_STRING(valueObject)) {
                    this->url = v8pp::from_v8<std::string>(isolate, valueObject);
                }
            } else if (key == "reloadAble") {
                if (valueObject->IsBoolean()) {
                    this->reloadAble = v8pp::from_v8<bool>(isolate, valueObject);
                }
            } else if (key == "disabled") {
                if (valueObject->IsBoolean()) {
                    this->disabled = v8pp::from_v8<bool>(isolate, valueObject);
                }
            } else if (key == "loadFiles") {
                loadStringArray(loadFiles, valueObject);
            } else if (key == "depends") {
                loadStringArray(depends, valueObject);
            }
        }

    }

    std::string const & Plugin::getName() const {
        return this->name;
    }

    const char* Plugin::getPath() const {
        return this->path.c_str();
    }

    const char* Plugin::getVersion() const {
        return this->version.c_str();
    }

    const char* Plugin::getAuthor() const {
        return this->author.c_str();
    }

    int Plugin::reload() {
        if (!reloadAble) {
            return CODE_PLUGIN_NO_RELOAD;
        }
        auto isolate = pluginManager->getIsolate();
        auto reloading = v8pp::to_v8(isolate, "reloading");
        auto context = isolate->GetCurrentContext();
        auto object = module.Get(isolate);
        object->Set(context, reloading, v8::Boolean::New(isolate, true)).ToChecked();
        int code = load();
        object->Delete(context, reloading).ToChecked();

        return code;
    }

    int Plugin::justLoad(v8::Local<v8::Context> context) {
        if (status == PluginStatus::Loaded) {
            return CODE_PLUGIN_ALREADY_LOAD;
        }

        auto isolate = this->pluginManager->getIsolate();
        v8::HandleScope handle_scope(isolate);

        // load files.
        auto module = this->module.IsEmpty() ? v8::Object::New(isolate) : this->module.Get(isolate);
        fs::path rootPath = this->path;
        for (const auto& item : loadFiles) {
            fs::path path = rootPath / item;
            if (!fs::exists(path)) {
                continue;
            }

            auto fullPath = std::filesystem::canonical(path);
            auto code = runJsFile(isolate, fullPath.string().c_str(), nullptr, module);
            if (code != CODE_OK) {
                logDebug("js run failed: $0", fullPath.string().c_str());
                return CODE_PLUGIN_FILE_ERROR;
            }
        }

        // check exports-ui.js file
        auto exportsUIPath = rootPath / "exports-ui.js";
        if (fs::exists(exportsUIPath)) {
            auto fullPath = std::filesystem::canonical(exportsUIPath);
            addUICommand(UICommandType::RunScriptFile, strdup(fullPath.string().c_str()), 0, true);
        }

        if (this->module.IsEmpty()) {
            this->module = V8ObjectType(isolate, module);
        }
        // register global functions
        registerGlobals(module->Get(context, v8pp::to_v8(isolate, "globals")).ToLocalChecked());
        // auto onInit = module->Get(context, v8pp::to_v8(isolate, "onInit")).ToLocalChecked();
        // if (onInit->IsFunction()) {
        //     v8pp::call_v8(isolate, onInit.As<v8::Function>(), v8::Object::New(isolate), MODULE_INIT_JS);
        // }

        status = PluginStatus::Loaded;
        return CODE_OK;
    }

    int Plugin::unload() {
        return CODE_NOT_IMPLEMENTED;
    }

    int Plugin::disablePlugin(bool fromProject) {
        this->disabled = true;
        if (fromProject) {
            this->disabledByProject = true;
        }
        this->status = PluginStatus::Disabled;
        unload();
        return CODE_OK;
    }

    const char* Plugin::getUrl() const {
        return url.c_str();
    }

    bool Plugin::isUrlEmpty() const {
        return url.empty();
    }

    PluginStatus Plugin::getPluginStatus() const {
        return status;
    }

    const char* Plugin::getPluginStatusString() const {
        static const char* array[] = { "WaitLoad", "Loading", "Loaded", "Disabled" };
        return array[static_cast<int>(this->status)];
    }

    bool Plugin::isDisabled() const {
        return this->disabled;
    }

    bool Plugin::isReloadAble() const {
        return this->reloadAble;
    }

    bool Plugin::isDisabledByProject() const {
        return this->disabledByProject;        
    }

    PluginManager *pluginManager() {
        return g_pluginManager;
    }

}