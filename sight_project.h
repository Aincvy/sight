//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#pragma once

#include <cstring>
#include <string>
#include <vector>
#include "sight_plugin.h"
#include "string"
#include "atomic"
#include "sys/types.h"

#include "absl/container/btree_map.h"

namespace sight {

    enum TypeIntValues {
        IntTypeProcess = 1,

        IntTypeInt = 100,
        IntTypeFloat,
        IntTypeDouble,
        IntTypeChar,
        IntTypeString,
        IntTypeBool,
        IntTypeLong,
        IntTypeColor,
        IntTypeVector3,
        IntTypeVector4,

        IntTypeNext = 3000,
    };

    /**
     * todo colors
     */
    struct SightNodeStyles {

    };

    struct ProjectConfig {
        std::atomic<uint> nodeOrPortId = 3000;
    };

    class SightNodeGraph;

    /**
     * @brief build target settings
     * May include programming language, application type.
     */
    struct BuildTarget {

    };

    enum class ProjectFileType {
        Regular,
        Directory,
        Graph,
        Plugin,
    };

    /**
     * @brief A file or a direcotry.
     * 
     */
    struct ProjectFile{
        ProjectFileType fileType{ProjectFileType::Regular};
        // base on project.baseDir
        std::string path;
        std::string filename;
        // if this is a directory.
        std::vector<ProjectFile> files;

        
    };

    /**
     * project class.
     *
     */
    class Project {
    public:
        bool isLoadCallbackCalled = false;

        Project() = default;
        Project(const char* baseDir, bool createIfNotExist);

        int build();
        int clean();
        int rebuild();

        uint nextNodeOrPortId();

        int load();
        int loadConfigFile();

        /**
         * load all need plugins
         * @return
         */
        int loadPlugins();

        /**
         *
         * @param name as a folder/file name
         * @return
         */
        int loadPlugin(const char* name);

        int save();
        int saveConfigFile();


        /**
         *
         * @param str
         * @return -1 if not found.
         */
        uint getIntType(std::string const& str, bool addIfNotFound = false);

        /**
         *
         * @param type
         * @return full name
         */
        std::string const & getTypeName(int type);

        /**
         *
         * @param name need type's full name
         * @return
         */
        uint addType(std::string const& name);

        std::string getBaseDir() const;

        /**
         * @brief Create a Graph object
         * 
         * @param path 
         * @param fixPath if true it will be add pathGraphFolder as prefix. `openGraph` is same.
         * @return SightNodeGraph* 
         */
        SightNodeGraph* createGraph(const char* path, bool fixPath = true);
        SightNodeGraph* openGraph(const char* path, bool fixPath = true);

        /**
         * @brief if has last open graph, then open it.
         * 
         */
        void checkOpenLastGraph();

        void buildFilesCache();

        int openFile(ProjectFile const& file);

        ProjectFile const& getFileCache() const;

        std::string pathGraphFolder() const;
        std::string pathPluginsFolder() const;
        std::string pathEntityFolder() const;

    private:
        std::string baseDir;
        bool createIfNotExist;
        ProjectConfig projectConfig;
        ProjectFile fileCache;

        std::atomic<uint> typeIdIncr;
        // todo this need to serialization and deserialization
        absl::btree_map<std::string, uint> typeMap;
        absl::btree_map<uint, std::string> reverseTypeMap;

        std::string lastOpenGraph{};

        // file locations
        std::string pathConfigFile() const;

        std::string pathSrcFolder() const;
        std::string pathTargetFolder() const;


        void initTypeMap();

        void initFolders();

        uint addType(std::string const& name, uint v);
    };

    /**
     * @brief Called if project is load success.
     * If ui status is not ready, then this function will be do nothing.
     *  And when ui status is ready, this function will be called.
     * 
     * @param project 
     */
    void onProjectAndUILoadSuccess(Project* project);

    /**
     * @brief 
     * 
     * @param baseDir 
     * @param createIfNotExist 
     * @return int 
     */
    int initProject(const char* baseDir, bool createIfNotExist = false);

    Project* currentProject();

    /**
     * todo multiple thread test.
     * @param str
     * @return -1 if not found.
     */
    uint getIntType(std::string const& str, bool addIfNotFound = false);

    /**
     *
     * @param type
     * @return full name
     */
    std::string const & getTypeName(int type);

    /**
     *
     * @param name need type's full name
     * @return
     */
    uint addType(std::string const& name);

}