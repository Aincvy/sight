//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_project.h"
#include "dbg.h"
#include "sight_js_parser.h"
#include "sight_ui.h"
#include "sight_util.h"
#include "sight.h"
#include "sight_node_editor.h"

#include "yaml-cpp/yaml.h"

#include "filesystem"
#include "fstream"
#include <string>

namespace sight {

    static Project* g_Project = nullptr;

    int Project::build() {
        return 0;
    }

    int Project::clean() {
        return 0;
    }

    int Project::rebuild() {
        clean();
        return build();
    }

    Project::Project(const char *baseDir,bool createIfNotExist) : baseDir(baseDir),
        createIfNotExist(createIfNotExist), typeIdIncr(IntTypeNext) {

        auto path = std::filesystem::path(this->baseDir);
        if (!path.is_absolute()) {
            // 
            this->baseDir = std::filesystem::canonical(path).generic_string();
            dbg(this->baseDir);
        }
        if (!endsWith(this->baseDir, "/")) {
            this->baseDir += "/";
        }

        initTypeMap();

        initFolders();
    }

    uint Project::nextNodeOrPortId() {
        return projectConfig.nodeOrPortId++;
    }

    std::string Project::pathConfigFile() const{
        return baseDir + "project.yaml";
    }

    int Project::load() {
        int i;
        CHECK_CODE(loadConfigFile(), i);

        return 0;
    }

    int Project::save() {
        int i;
        CHECK_CODE(saveConfigFile(), i);

        return 0;
    }

    int Project::saveConfigFile() {
        std::string path = pathConfigFile();

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << "nodeOrPortId" << YAML::Value << projectConfig.nodeOrPortId.load();

        out << YAML::EndMap;

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        return CODE_OK;
    }

    int Project::loadConfigFile() {
        std::string path = pathConfigFile();

        std::ifstream fin(path);
        if (!fin.is_open()) {
            if (createIfNotExist) {
                return saveConfigFile();
            }
            return CODE_FILE_ERROR;
        }

        try {
            auto root = YAML::Load(fin);
            projectConfig.nodeOrPortId = root["nodeOrPortId"].as<uint>();

        } catch (const YAML::BadConversion & e){
            dbg("load project config error.", path.c_str(), e.what());
            return CODE_FILE_FORMAT_ERROR;
        }

        return CODE_OK;
    }

    int Project::loadPlugin(const char *name) {


        return 0;
    }

    int Project::loadPlugins() {

        return 0;
    }

    void Project::initTypeMap() {
        std::string process = "Process";
        std::string _string = "String";
        const char* number = "Number";

        typeMap[process] = IntTypeProcess;
        lowerCase(process);
        typeMap[process] = IntTypeProcess;

        typeMap[_string] = IntTypeString;
        lowerCase(_string);
        typeMap[_string] = IntTypeString;

        typeMap[number] = IntTypeFloat;
        typeMap["int"] = IntTypeInt;
        typeMap["float"] = IntTypeFloat;
        typeMap["double"] = IntTypeDouble;
        typeMap["long"] = IntTypeLong;
        typeMap["bool"] = IntTypeBool;
        typeMap["Color"] = IntTypeColor;
        typeMap["Vector3"] = IntTypeVector3;
        typeMap["Vector4"] = IntTypeVector4;

        for (auto & item: typeMap) {
            reverseTypeMap[item.second] = item.first;
        }
    }

    uint Project::getIntType(const std::string &str, bool addIfNotFound) {
        auto a = typeMap.find(str);
        if (a != typeMap.end()) {
            return a->second;
        } else if (addIfNotFound) {
            return addType(str);
        }

        return -1;
    }

    std::string Project::getBaseDir() const {
        return baseDir;
    }

    SightNodeGraph* Project::createGraph(const char* path) {
        std::string targetPath = pathGraphFolder() + path;
        std::filesystem::path temp(targetPath);
        if (temp.has_extension()) {
            targetPath = std::string(targetPath, 0, targetPath.rfind('.') - 1);
        }
        dbg(targetPath);
        changeGraph(targetPath.c_str());
        return getCurrentGraph();
    }

    SightNodeGraph* Project::openGraph(const char* path) {
        return createGraph(path);
    }

    std::string const &Project::getTypeName(int type) {
        auto a = reverseTypeMap.find(type);
        if (a != reverseTypeMap.end()) {
            return a->second;
        }
        return emptyString;
    }

    uint Project::addType(const std::string &name) {
        uint a = ++ (typeIdIncr);
        typeMap[name] = a;
        reverseTypeMap[a] = name;
        return a;
    }

    std::string Project::pathSrcFolder() const {
        return baseDir + "src/";
    }

    std::string Project::pathTargetFolder() const {
        return baseDir + "target/";
    }

    std::string Project::pathPluginsFolder() const {
        return baseDir + "plugins/";
    }

    std::string Project::pathEntityFolder() const {
        return pathSrcFolder() + "entity/";
    }

    void Project::initFolders() {
        std::filesystem::create_directories(pathEntityFolder());
        std::filesystem::create_directories(pathTargetFolder());
        std::filesystem::create_directories(pathPluginsFolder());
        std::filesystem::create_directories(pathGraphFolder());
    }

    std::string Project::pathGraphFolder() const {
        return pathSrcFolder() + "graph/";
    }

    Project *currentProject() {
        return g_Project;
    }

    void onProjectLoadSuccess(Project* project){
        auto status = currentUIStatus();
        if (!status) {
            return;
        }

        status->selection.projectPath = project->getBaseDir();
        
    }

    int initProject(const char* baseDir, bool createIfNotExist) {
        bool hasParent = true;
        if (!std::filesystem::exists(baseDir) || !std::filesystem::is_directory(baseDir)) {
            if (createIfNotExist) {
                std::filesystem::create_directories(baseDir);
                hasParent = false;
            } else {
                return CODE_FILE_ERROR;
            }
        }

        g_Project = new Project(baseDir, createIfNotExist);
        auto code = g_Project->load();
        if (code == CODE_OK) {
            onProjectLoadSuccess(g_Project);
        }
        return code;
    }


    uint getIntType(const std::string &str, bool addIfNotFound) {
        return g_Project->getIntType(str, addIfNotFound);
    }

    std::string const &getTypeName(int type) {
        return g_Project->getTypeName(type);
    }

    uint addType(const std::string &name) {
        return g_Project->addType(name);
    }
    
}