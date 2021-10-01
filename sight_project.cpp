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
#include <algorithm>
#include <string>
#include <sys/types.h>

namespace sight {

    using directory_iterator = std::filesystem::directory_iterator;

    static Project* g_Project = nullptr;

    namespace  {
        
        bool isGraphFile(std::string const& path){
            std::ifstream fin(path);
            if (!fin.is_open()) {
                return false;
            }

            try {
                auto root = YAML::Load(fin);
                auto node = root["who-am-i"];
                if (node.IsDefined()) {
                    return node.as<std::string>() == "sight-graph";
                }
            } catch (const YAML::BadConversion & e){
                return false;
            }

            return false;
        }

        void fillFiles(ProjectFile & parent){
            parent.files.clear();

            for (const auto& item : directory_iterator{parent.path}) {
                std::string fullpath = std::filesystem::canonical(item.path());
                if (item.is_directory()) {
                    parent.files.push_back(
                        {
                            ProjectFileType::Directory,
                            fullpath,
                            item.path().filename().generic_string(),
                            {}
                        }
                    );
                    fillFiles(parent.files.back());
                    continue;
                }
                if (item.is_regular_file()) {
                    auto filename = item.path().filename();
                    if (filename == ".DS_Store") {
                        continue;
                    }

                    if (item.path().has_extension()) {
                        auto ext = item.path().extension();
                        if (ext == ".yaml") {
                            // check the is a graph or not.
                            if (isGraphFile(item.path().generic_string())) {
                                parent.files.push_back({
                                    ProjectFileType::Graph,
                                    fullpath,
                                    removeExt(filename.generic_string()),
                                    {}
                                });

                                continue;
                            }
                        } else if (ext == ".json") {
                            // graph position file
                            std::string nameWithoutExt = removeExt(filename);
                            auto findResult = std::find_if(parent.files.begin(), parent.files.end(), [&nameWithoutExt](ProjectFile const& f){ 
                                return f.fileType == ProjectFileType::Graph && f.filename == nameWithoutExt;
                            });
                            if (findResult != parent.files.end()) {
                                // has graph, ignore this file.
                                continue;
                            }

                            if (isGraphFile(removeExt(item.path().generic_string()) + ".yaml")) {
                                continue;
                            }
                        }
                    }

                    parent.files.push_back({
                        ProjectFileType::Regular,
                        fullpath,
                        filename.generic_string(),
                        {}
                    });
                }
            }

            // sort directory and file
            std::sort(parent.files.begin(), parent.files.end(), [](ProjectFile const& f1, ProjectFile const& f2) {
                if (f1.fileType == ProjectFileType::Directory && f2.fileType == ProjectFileType::Regular) {
                    return true;
                } else if ((f1.fileType == ProjectFileType::Directory && f2.fileType == ProjectFileType::Directory) 
                            || (f1.fileType != ProjectFileType::Directory && f2.fileType != ProjectFileType::Directory)) {
                    return f1.filename < f2.filename;    
                }
                
                return false;
            });

            
        }

    }

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

        fileCache.path = baseDir;
        fileCache.filename = "";
        fileCache.fileType = ProjectFileType::Directory;
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
        out << YAML::Key << "lastOpenGraph" << YAML::Value << lastOpenGraph;

        out << YAML::Key << "typeMap" << YAML::BeginMap;
        for (const auto & item : this->typeMap) {
            if (item.second < IntTypeNext) {
                continue;
            }

            out << YAML::Key << item.first << YAML::Value << item.second;
        }
        out << YAML::EndMap;

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

            auto temp = root["lastOpenGraph"];
            if (temp.IsDefined()) {
                this->lastOpenGraph = temp.as<std::string>();
            }

            temp = root["typeMap"];
            if (temp.IsDefined()) {
                for (const auto & item : temp) {
                    addType(item.first.as<std::string>(), item.second.as<uint>());
                }
            }

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

    SightNodeGraph* Project::createGraph(const char* path, bool fixPath) {
        auto g = openGraph(path, fixPath);
        buildFilesCache();
        return g;
    }

    SightNodeGraph* Project::openGraph(const char* path, bool fixPath) {
        std::string targetPath = fixPath ? pathGraphFolder() + path : path;
        std::filesystem::path temp(targetPath);
        if (temp.has_extension()) {
            std::string ext = temp.extension().generic_string();
            if (ext == ".json" || ext == ".yaml") {
                targetPath = std::string(targetPath, 0, targetPath.rfind('.'));
            }
        }
        dbg(targetPath);
        changeGraph(targetPath.c_str());
        lastOpenGraph = path;
        return getCurrentGraph();
    }

    void Project::checkOpenLastGraph() {
        if (lastOpenGraph.empty()) {
            return;
        }

        openGraph(lastOpenGraph.c_str(), false);
    }

    ProjectFile const& Project::getFileCache() const {
        return fileCache;
    }

    void Project::buildFilesCache() {
        fillFiles(this->fileCache);
    }

    int Project::openFile(ProjectFile const& file) {

        return CODE_OK;
    }

    std::string const &Project::getTypeName(int type) {
        auto a = reverseTypeMap.find(type);
        if (a != reverseTypeMap.end()) {
            return a->second;
        }
        return emptyString;
    }

    uint Project::addType(const std::string &name) {
        return addType(name, ++(typeIdIncr));
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

    uint Project::addType(std::string const& name, uint v) {
        typeMap[name] = v;
        reverseTypeMap[v] = name;
        return v;
    }

    std::string Project::pathGraphFolder() const {
        return pathSrcFolder() + "graph/";
    }

    absl::btree_map<std::string, BuildTarget> & Project::getBuildTargetMap() {
        return buildTargetMap;
    }

    Project *currentProject() {
        return g_Project;
    }

    void onProjectAndUILoadSuccess(Project* project){
        if (!project || project->isLoadCallbackCalled) {
            return;
        }

        auto status = currentUIStatus();
        if (!status) {
            return;
        }

        project->isLoadCallbackCalled = true;
        status->selection.projectPath = project->getBaseDir();
        
        project->buildFilesCache();
        project->checkOpenLastGraph();
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
            onProjectAndUILoadSuccess(g_Project);
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