//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//
#pragma once

#include <future>
#include <string>

#include "sight_defines.h"

namespace sight {

    extern const std::string resourceFolder;
    extern const std::string whoAmI;

    constexpr const char* sightIsProject = "sight-project";

    /**
     *
     */
    struct CommandArgs {
        // string arg.
        const char* argString = nullptr;
        int argStringLength = 0;
        bool needFree = false;

        int argInt = 0;
        void* data = nullptr;
        // if data pointer to an array, this is the length.
        size_t dataLength = 0;

        // for simple thread sync.
        std::promise<int>* promise = nullptr;

        /**
         * dispose command
         */
        void dispose();
    };

    enum ErrorCode {
        CODE_OK = 0,
        CODE_FAIL = 1,
        CODE_NOT_IMPLEMENTED,
        CODE_ERROR,

        CODE_FILE_ERROR = 100,
        CODE_FILE_NOT_EXISTS,
        CODE_FILE_FORMAT_ERROR,
        CODE_FILE_INIT,

        CODE_USER_CANCELED = 150,

        CODE_PLUGIN_NO_PKG_FILE = 200,
        CODE_PLUGIN_COMPILE_FAIL,
        CODE_PLUGIN_NO_DESCRIPTION,
        CODE_PLUGIN_FILE_ERROR,
        CODE_PLUGIN_DISABLED,

        CODE_SCRIPT_NO_RESULT = 250,
    };

    /**
     * @brief 
     * 
     */
    struct SightSettings {
        std::string path = "sight.yaml";

        std::string lastOpenProject = "";
        ushort networkListenPort = 39455;
    };

    /**
     * @brief 
     * Create one if not exist.
     * @param path  config file, yaml
     * @return int CODE_OK or others
     */
    int loadSightSettings(const char* path);

    int saveSightSettings();

    SightSettings* getSightSettings();
    
    /**
     * exit program.
     * do clean work except ui and node_editor
     */
    void exitSight(int v = 0);

    /**
     * you need free this function return value.
     * @param from
     * @return
     */
    void* copyObject(void* from, size_t size);


}     // namespace sight