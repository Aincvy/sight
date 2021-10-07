//
// Created by Aincvy(aincvy@gmail.com) on 2021/9/17.
//

#pragma once

#include <string>
#include "vector"
#include "absl/container/flat_hash_map.h"

#include "v8.h"

namespace sight {

    class PluginManager;

    class Plugin {
    public:
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

    private:
        std::string name;
        std::string path;
        std::string version;
        std::string author;
        bool disabled = false;

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

    private:
        absl::flat_hash_map<std::string, Plugin*> pluginMap;

        /**
         * share with jsThread
         */
        v8::Isolate* isolate = nullptr;

        std::vector<std::string> searchPaths;

    };

    /**
     * Use plugin manager from js thread.
     * @return
     */
    PluginManager* pluginManager();

    extern v8::MaybeLocal<v8::String> readFile(v8::Isolate *isolate, const char *name);

}