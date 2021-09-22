//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#pragma once

#include <cstring>
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

    /**
     * project class.
     *
     */
    class Project {
    public:

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


    private:
        std::string baseDir;
        bool createIfNotExist;
        ProjectConfig projectConfig;

        std::atomic<uint> typeIdIncr;
        // todo this need to serialization and deserialization
        absl::btree_map<std::string, uint> typeMap;
        absl::btree_map<uint, std::string> reverseTypeMap;

        // file locations
        std::string pathConfigFile() const;

        std::string pathSrcFolder() const;
        std::string pathTargetFolder() const;
        std::string pathPluginsFolder() const;
        std::string pathEntityFolder() const;
        std::string pathGraphFolder() const;

        void initTypeMap();

        void initFolders();

    };

    /**
     * @brief Called if project is load success.
     * If ui status is not ready, then this function will be do nothing.
     *  And when ui status is ready, this function will be called.
     * 
     * @param project 
     */
    void onProjectLoadSuccess(Project* project);

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