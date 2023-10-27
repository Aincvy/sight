//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include <cstring>
#include <fstream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <filesystem>

#include "sight.h"
#include "sight_js.h"
#include "sight_log.h"
#include "sight_project.h"


#include "yaml-cpp/yaml.h"

static sight::SightSettings sightSettings;

namespace sight {


    const std::string resourceFolder = "resources/";
    const std::string whoAmI = "who-am-i";


    int loadSightSettings(const char* path) {
        if (!path) {
            path = sightSettings.path.c_str();
        }

        sightSettings.path = std::filesystem::absolute(path).string();
        logDebug("loading sight settings at: $0",  sightSettings.path);

        std::ifstream fin(sightSettings.path);
        if (!fin.is_open()) {
            sightSettings.path = path;
            saveSightSettings();
            return CODE_FILE_INIT;
        }

        auto root = YAML::Load(fin);

        // check sign
        auto n = root[whoAmI];
        if (n.IsDefined()) {
            if (n.as<std::string>() != "sight-settings") {
                return CODE_FILE_ERROR;
            }
        } else {
            return CODE_FILE_ERROR;
        }

        // load data
        n = root["networkListenPort"];
        if (n.IsDefined()) {
            sightSettings.networkListenPort = n.as<ushort>();
        }

        n = root["lastOpenProject"];
        if (n.IsDefined()) {
            sightSettings.lastOpenProject = n.as<std::string>();
        }

        n = root["autoSave"];
        if (n.IsDefined()) {
            sightSettings.autoSave = n.as<bool>();
        }

        n = root["windowStatus"];
        if (n.IsDefined()) {
            auto& windowStatus = sightSettings.windowStatus;
            windowStatus.nodeGraph = n["nodeGraph"].as<bool>();
            windowStatus.createEntity = n["createEntity"].as<bool>();
            windowStatus.testWindow = n["testWindow"].as<bool>();
            windowStatus.aboutWindow = n["aboutWindow"].as<bool>();
            windowStatus.projectSettingsWindow = n["projectSettingsWindow"].as<bool>();
            windowStatus.entityListWindow = n["entityListWindow"].as<bool>();
            windowStatus.entityInfoWindow = n["entityInfoWindow"].as<bool>();
            windowStatus.generateResultWindow = n["generateResultWindow"].as<bool>();
            windowStatus.terminalWindow = n["terminal"].as<bool>(false);
        }

        n = root["lastUseEntityOperation"];
        if (n.IsDefined()) {
            sightSettings.lastUseEntityOperation = n.as<std::string>();
        }

        n = root["sightRootFolder"];
        if (n.IsDefined()) {
            sightSettings.sightRootFolder = n.as<std::string>();
        }

        n = root["lastMainWindowWidth"];
        if (n.IsDefined()) {
            sightSettings.lastMainWindowWidth = n.as<int>();
            logDebug("n is defined, lastMainWindowWidth: $0", n.as<int>());
        }

        n = root["lastMainWindowHeight"];
        if (n.IsDefined()) {
            sightSettings.lastMainWindowHeight = n.as<int>();
        }

        logDebug("lastMainWindowWidth: $0, lastMainWindowHeight: $1", 
            sightSettings.lastMainWindowWidth, sightSettings.lastMainWindowHeight);

        // for (const auto& tmp : root) {
        //     logDebug("root child: $0", tmp.first.as<std::string>());
        // }
        
        return CODE_OK;
    }

    int saveSightSettings() {
        YAML::Emitter out;
        out << YAML::BeginMap;
        
        out << YAML::Key << whoAmI << YAML::Value << "sight-settings";
        out << YAML::Key << "networkListenPort" << YAML::Value << sightSettings.networkListenPort;
        out << YAML::Key << "lastOpenProject" << YAML::Value << sightSettings.lastOpenProject;
        out << YAML::Key << "autoSave" << YAML::Value << sightSettings.autoSave;

        // windows status
        out << YAML::Key << "windowStatus" << YAML::Value << YAML::BeginMap;
        auto& windowStatus = sightSettings.windowStatus;
        out << YAML::Key << "nodeGraph" << YAML::Value << windowStatus.nodeGraph;
        out << YAML::Key << "createEntity" << YAML::Value << windowStatus.createEntity;
        out << YAML::Key << "testWindow" << YAML::Value << windowStatus.testWindow;
        out << YAML::Key << "aboutWindow" << YAML::Value << windowStatus.aboutWindow;
        out << YAML::Key << "projectSettingsWindow" << YAML::Value << windowStatus.projectSettingsWindow;
        out << YAML::Key << "entityListWindow" << YAML::Value << windowStatus.entityListWindow;
        out << YAML::Key << "entityInfoWindow" << YAML::Value << windowStatus.entityInfoWindow;
        out << YAML::Key << "generateResultWindow" << YAML::Value << windowStatus.generateResultWindow;
        out << YAML::Key << "terminal" << YAML::Value << windowStatus.terminalWindow;
        out << YAML::EndMap;        // end of windowStatus
        
        out << YAML::Key << "lastUseEntityOperation" << YAML::Value << sightSettings.lastUseEntityOperation;
        out << YAML::Key << "sightRootFolder" << YAML::Value << sightSettings.sightRootFolder;

        out << YAML::Key << "lastMainWindowWidth" << YAML::Value << sightSettings.lastMainWindowWidth;
        out << YAML::Key << "lastMainWindowHeight" << YAML::Value << sightSettings.lastMainWindowHeight;

        out << YAML::EndMap;
        std::ofstream fOut(sightSettings.path, std::ios::out | std::ios::trunc);
        fOut << out.c_str() << std::endl;
        return CODE_OK;
    }

    SightSettings* getSightSettings() {
        return &sightSettings;
    }

    void exitSight(int v) {
        saveSightSettings();

        addJsCommand(JsCommandType::Destroy);
        auto p = currentProject();
        if (p) {
            p->save();
        }
        // exit(v);
    }

    void* copyObject(void* from, size_t size) {
        auto dst = calloc(1, size);
        memcpy(dst, from, size);
        return dst;
    }

    void CommandArgs::dispose() {
        if (needFree) {
            if (argString) {
                free(const_cast<char*>(argString));
                argString = nullptr;
                argStringLength = 0;
            }
            if (data) {
                free(data);
                data = nullptr;
                dataLength = 0;
            }
        }
    }

    CommandArgs CommandArgs::copyFrom(std::string_view str) {
        return {
            .argString = strdup(str.data()),
            .argStringLength = static_cast<int>(str.length()),
            .needFree = true,
        };
    }
}     // namespace sight