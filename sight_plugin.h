//
// Created by Aincvy(aincvy@gmail.com) on 2021/9/17.
//

#pragma once

#include <string>
#include "vector"
#include <atomic>
#include <map>
#include "absl/container/flat_hash_map.h"

#include "v8.h"

namespace sight {

    class PluginManager;

    enum class PluginStatus {
        WaitLoad = 0,
        Loading,
        Loaded,
        Disabled,
    };

    class Plugin {
    public:
        using V8ObjectType = v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>>;

        Plugin(PluginManager* pluginManager, std::string name, std::string path, std::string version, std::string author);

        /**
         * construct plugin from a folder or a file.
         * @param path
         */
        Plugin(PluginManager* pluginManager, std::string path);

        void debugBrieflyInfo() const;

        PluginManager* getPluginManager();
        [[nodiscard]] const PluginManager* getPluginManager() const;

        /**
         *
         * @return CODE_OK, CODE_FAIL
         */
        int load();

        [[nodiscard]] std::string const & getName() const;

        const char* getPath() const;
        const char* getVersion() const;
        const char* getAuthor() const;

        int reload();

        PluginStatus getPluginStatus() const;
        const char* getPluginStatusString() const;

    private:
        std::string name;
        std::string path;
        std::string version;
        std::string author;
        bool disabled = false;
        // every plugin share one module.
        V8ObjectType module = {};
        PluginStatus status = PluginStatus::WaitLoad;

        PluginManager* pluginManager = nullptr;

        int loadFromFolder();
        int loadFromFile();

        /**
         *
         * @param object
         */
        void readBaseInfoFromJsObject(v8::Local<v8::Object> object);

    };


    /**
     * se plugin manager from js thread.
     */
    class PluginManager {
    public:
        PluginManager();
        ~PluginManager();

        int init(v8::Isolate* isolate);

        int loadPlugins();

        v8::Isolate* getIsolate();

        void addSearchPath(const char* path);

        absl::flat_hash_map<std::string, Plugin*> & getPluginMap();
        absl::flat_hash_map<std::string, Plugin*> const& getPluginMap() const;

        std::map<std::string, const Plugin*> const& getSnapshotMap();

        std::vector<std::string> const& getSortedPluginNames() const;

        uint getLoadedPluginCount() const;

    private:
        absl::flat_hash_map<std::string, Plugin*> pluginMap;
        // 
        std::map<std::string, const Plugin*> snapshotMap;
        // used for ui show
        std::vector<std::string> sortedPluginNames;

        /**
         * share with jsThread
         */
        v8::Isolate* isolate = nullptr;

        std::vector<std::string> searchPaths;

        uint loadedPluginCount = 0;
    };

    /**
     * Get the plugin manager.
     * @return
     */
    PluginManager* pluginManager();


    extern v8::MaybeLocal<v8::String> readFile(v8::Isolate *isolate, const char *name);

}