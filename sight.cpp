//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include <fstream>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "sight.h"
#include "sight_js.h"
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

        std::ifstream fin(path);
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

        return CODE_OK;
    }

    int saveSightSettings() {
        YAML::Emitter out;
        out << YAML::BeginMap;
        
        out << YAML::Key << whoAmI << YAML::Value << "sight-settings";
        out << YAML::Key << "networkListenPort" << YAML::Value << sightSettings.networkListenPort;
        out << YAML::Key << "lastOpenProject" << YAML::Value << sightSettings.lastOpenProject;

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
}     // namespace sight