//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//
#pragma once

#include <future>
#include <string>
#include <string_view>

#include "sight_defines.h"

namespace sight {

    extern const std::string resourceFolder;
    extern const std::string whoAmI;

    constexpr const char* sightIsProject = "sight-project";
    constexpr const char* sightIsCopyText = "sight-copy-text";

    constexpr const char* stringType = "type";
    constexpr const char* stringData = "data";

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

        /**
         * @brief Copy from a string.  use `strdup()`
         * 
         * @param str 
         * @return CommandArgs 
         */
        static CommandArgs copyFrom(std::string_view str);
        
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
        CODE_PLUGIN_NO_RELOAD,                    // not reload-able

        CODE_SCRIPT_NO_RESULT = 250,

        CODE_GRAPH_START = 300,
        CODE_TEMPLATE_ADDRESS_INVALID,
        CODE_NO_TEMPLATE_ADDRESS,
        CODE_GRAPH_BROKEN,
        CODE_NODE_HAS_CONNECTIONS,
    };

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
        IntTypeObject,
        // a large string.
        IntTypeLargeString,
        // render as a button
        IntTypeButton,

        IntTypeNext = 3000,
    };

    struct UIWindowStatus {
        bool nodeGraph = false;
        bool createEntity = false;
        bool testWindow = false;
        bool aboutWindow = false;
        bool projectSettingsWindow = false;
        bool entityListWindow = false;
        bool entityInfoWindow = false;
        bool generateResultWindow = false;

        bool layoutReset = false;

        // popups
        bool popupGraphName = false;
        bool popupAskModal = false;
        bool popupSaveModal = false;
        bool popupAlertModal = false;
    };

    /**
     * @brief 
     * 
     */
    struct SightSettings {
        std::string path = "sight.yaml";

        std::string lastOpenProject = "";
        ushort networkListenPort = 39455;

        UIWindowStatus windowStatus;

        bool autoSave = false;

        std::string lastUseEntityOperation = "";
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