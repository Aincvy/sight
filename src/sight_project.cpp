//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_project.h"
#include "crude_json.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_log.h"
#include "sight_plugin.h"
#include "sight_ui_node_editor.h"
#include "sight_util.h"
#include "sight.h"
#include "sight_node.h"
#include "sight_node_graph.h"
#include "sight_memory.h"
#include "sight_widgets.h"
#include "sight_code_set.h"


#include "yaml-cpp/node/parse.h"
#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <tuple>
#include <utility>
#include <vector>
#include <yaml-cpp/emittermanip.h>

#include "absl/strings/substitute.h"



namespace sight {

    namespace fs = std::filesystem;
    using directory_iterator = fs::directory_iterator;

    static Project* g_Project = nullptr;
    static TypeInfo emptyTypeInfo;

    static SightArray<TypeStyle> typeStyleArray;
    static TypeStyle defaultTypeStyle = { IM_COL32_WHITE, IconType::Circle };
    static const char* iconTypeStrings[] = { "Flow", "Circle", "Square","Grid","RoundSquare","Diamond" };

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

            for (const auto& item : directory_iterator{ parent.path }) {
                if (startsWith(item.path().filename().generic_string(), ".")) {
                    continue;
                }
                std::string fullpath = std::filesystem::canonical(item.path()).string();
                if (item.is_directory()) {
                    parent.files.emplace_back(ProjectFileType::Directory, fullpath, item.path().filename().generic_string());
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
                                parent.files.emplace_back(ProjectFileType::Graph, fullpath, removeExt(filename.generic_string()));

                                continue;
                            }
                        } else if (ext == ".json") {
                            // graph position file
                            std::string nameWithoutExt = removeExt(filename.string());
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

                    parent.files.emplace_back(ProjectFileType::Regular, fullpath, filename.generic_string());
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

        YAML::Emitter& operator << (YAML::Emitter& out, SightEntity const& entity){
            out << YAML::Key << entity.templateAddress << YAML::BeginMap;            
            out << YAML::Key << "name" << YAML::Value << entity.name;
            out << YAML::Key << "typeId" << YAML::Value << entity.typeId;
            out << YAML::Key << "parentEntity" << YAML::Value << entity.parentEntity;

            out << YAML::Key << "fields" << YAML::BeginMap;
            for( const auto& item: entity.fields){
                out << YAML::Key << item.name << YAML::BeginMap;
                out << YAML::Key << "type" << YAML::Value << item.type;
                out << YAML::Key << "defaultValue" << YAML::Value << item.defaultValue;

                out << YAML::Key << "options" << YAML::BeginMap;
                out << YAML::Key << "portType" << YAML::Value << item.options.portTypeValue();
                out << YAML::Key << "show" << YAML::Value << item.options.portOptions.show;
                out << YAML::Key << "showValue" << YAML::Value << item.options.portOptions.showValue;
                out << YAML::Key << "readonly" << YAML::Value << item.options.portOptions.readonly;
                out << YAML::EndMap;

                out << YAML::EndMap;
            }
            out << YAML::EndMap;


            out << YAML::EndMap;
            return out;
        }

        // Serialization
        YAML::Emitter& operator<<(YAML::Emitter& out, const SightNodeGraphOutputJsonConfig& rhs) {
            out << YAML::BeginMap;
            out << YAML::Key << "nodeRootName" << YAML::Value << rhs.nodeRootName;
            out << YAML::Key << "connectionRootName" << YAML::Value << rhs.connectionRootName;
            out << YAML::Key << "includeRightConnections" << YAML::Value << rhs.includeRightConnections;
            out << YAML::Key << "includeNodeIdOnConnectionData" << YAML::Value << rhs.includeNodeIdOnConnectionData;
            out << YAML::Key << "exportData" << YAML::Value << rhs.exportData;
            out << YAML::Key << "fieldNameCaseType" << YAML::Value << static_cast<int>(rhs.fieldNameCaseType);
            out << YAML::EndMap;
            return out;
        }

        // Deserialization
        bool operator>>(YAML::Node const& node, SightNodeGraphOutputJsonConfig& rhs) {
            rhs.nodeRootName = node["nodeRootName"].as<std::string>();
            rhs.connectionRootName = node["connectionRootName"].as<std::string>();
            rhs.includeRightConnections = node["includeRightConnections"].as<bool>();
            rhs.includeNodeIdOnConnectionData = node["includeNodeIdOnConnectionData"].as<bool>();
            rhs.exportData = node["exportData"].as<bool>();
            rhs.fieldNameCaseType = static_cast<CaseTypes>(node["fieldNameCaseType"].as<int>());
            return true;
        }

        SightEntity loadEntity(std::string const& templateAddress, YAML::Node const& node){
            SightEntity entity;
            entity.templateAddress = templateAddress;
            entity.name = node["name"].as<std::string>();
            auto tmp = node["typeId"];
            if (tmp.IsDefined()) {
                entity.typeId = tmp.as<uint>();
            }
            
            // tmp = node["parentEntity"];
            if (node["parentEntity"]) {
                entity.parentEntity = node["parentEntity"].as<std::string>();
            }

            auto fieldsNode = node["fields"];
            for( const auto& item: fieldsNode){
                std::string name = item.first.as<std::string>();
                auto &dataNode = item.second;
                entity.fields.emplace_back(name, dataNode["type"].as<std::string>(), dataNode["defaultValue"].as<std::string>());

                auto & f = entity.fields.back();
                auto optionsNode = dataNode["options"];
                if (optionsNode.IsDefined()) {
                    f.options.portType = static_cast<NodePortType>(optionsNode["portType"].as<int>());
                    f.options.portOptions.show = optionsNode["show"].as<bool>();
                    f.options.portOptions.showValue = optionsNode["showValue"].as<bool>();
                    f.options.portOptions.readonly = optionsNode["readonly"].as<bool>();
                }
            }

            return entity;
        }
    }

    int Project::build(const char* target) {
        auto iter = buildTargetMap.find(target);
        if (iter == buildTargetMap.end()) {
            return CODE_FAIL;
        }

        auto & buildTarget = iter->second;
        buildTarget.build();

        if (lastBuildTarget != target) {
            lastBuildTarget = target;
        }
        return CODE_OK;
    }

    int Project::clean() {
        // remove target/graph folder
        std::filesystem::remove_all(pathTargetFolder() + "graph");
        return CODE_OK;
    }

    int Project::rebuild() {
        clean();
        return build(lastBuildTarget.c_str());
    }

    Project::Project(const char *baseDir,bool createIfNotExist) : baseDir(baseDir),
        createIfNotExist(createIfNotExist), typeIdIncr(IntTypeNext) {

        auto path = std::filesystem::path(this->baseDir);
        if (!path.is_absolute()) {
            // 
            this->baseDir = std::filesystem::canonical(path).generic_string();
            logDebug(this->baseDir);
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

    int Project::codeSetBuild() {
        logDebug("start code set build.");

        logDebug("end code set build.");
        return CODE_OK;
    }

    uint Project::nextNodeOrPortId() {
        return projectConfig.nodeOrPortId++;
    }

    uint Project::maxNodeOrPortId() const {
        return projectConfig.nodeOrPortId;
    }

    std::string Project::pathConfigFile() const{
        return baseDir + "project.yaml";
    }

    std::string Project::pathStyleConfigFile() const {
        return baseDir + "styles.yaml";
    }

    std::string Project::pathEntitiesConfigFile() const {
        return baseDir + "entities.yaml";
    }

    int Project::load() {
        int i;
        CHECK_CODE(loadConfigFile(), i);
        CHECK_CODE(loadStyleInfo(), i);
        CHECK_CODE(loadEntities(), i);
        // CHECK_CODE(loadPlugins(), i);    // NEED INIT V8 FIRST

        return 0;
    }

    int Project::save() {
        int i;
        CHECK_CODE(saveConfigFile(), i);
        CHECK_CODE(saveStyleInfo(), i);
        CHECK_CODE(saveEntities(), i);

        return CODE_OK;
    }

    int Project::saveConfigFile() {
        std::string path = pathConfigFile();
        // logDebug("save project config file: $0", path.c_str());

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << whoAmI << YAML::Value << sightIsProject;
        out << YAML::Key << "nodeOrPortId" << YAML::Value << projectConfig.nodeOrPortId.load();
        out << YAML::Key << "typeIdIncr" << YAML::Value << typeIdIncr.load();
        out << YAML::Key << "lastOpenGraph" << YAML::Value << lastOpenGraph;
        out << YAML::Key << "lastBuildTarget" << YAML::Value << lastBuildTarget;

        out << YAML::Key << "typeMap" << YAML::BeginMap;
        for (const auto & item : this->typeMap) {
            if (item.second < IntTypeNext) {
                continue;
            }

            out << YAML::Key << item.first << YAML::Value << item.second;
        }
        out << YAML::EndMap;

        // code settings
        out << YAML::Key << "codeSetSettings" << codeSetSettings;

        // disabledPluginNames
        std::vector<std::string> tmpArray(disabledPluginNames.begin(), disabledPluginNames.end());
        out << YAML::Key << "disabledPluginNames" << YAML::Value << tmpArray;

        // graphOutputJsonConfig
        out << YAML::Key << "graphOutputJsonConfig" << YAML::Value << graphOutputJsonConfig;


        out << YAML::EndMap;

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        return CODE_OK;
    }

    int Project::saveStyleInfo() {
        auto path = pathStyleConfigFile();

        YAML::Emitter out;

        out << YAML::BeginMap;
        out << YAML::Key << whoAmI << YAML::Value << "sight-styles";

        out << YAML::Key << "typeMap" << YAML::BeginMap;
        for (const auto& item : this->typeInfoMap) {
            if (!item.second.style) {
                continue;
            }

            out << YAML::Key << item.first;
            out << YAML::BeginMap;
            auto style = item.second.style;
            out << YAML::Key << "icon" << YAML::Value << static_cast<int>(style->iconType);
            out << YAML::Key << "color" << YAML::Value << style->color;
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        out << YAML::EndMap;

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        return CODE_OK;
    }

    int Project::saveEntities() {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << whoAmI << YAML::Value << "sight-entities";
        for( const auto& item: this->entitiesMap){
            out << item.second;
        }
        out << YAML::EndMap;

        std::ofstream outToFile(pathEntitiesConfigFile(), std::ios::out | std::ios::trunc);
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
            typeIdIncr = root["typeIdIncr"].as<uint>();

            auto temp = root["lastOpenGraph"];
            if (temp.IsDefined()) {
                this->lastOpenGraph = temp.as<std::string>();
            }
            temp = root["lastBuildTarget"];
            if (temp.IsDefined()) {
                this->lastBuildTarget = temp.as<std::string>();
            }

            temp = root["typeMap"];
            if (temp.IsDefined()) {
                for (const auto & item : temp) {
                    addType(item.first.as<std::string>(), item.second.as<uint>());
                }
            }

            temp = root["codeSetSettings"];
            if(temp.IsDefined()) {
                temp >> this->codeSetSettings;
            }

            temp = root["disabledPluginNames"];
            if (temp.IsDefined()) {
                for (const auto& item : temp) {
                    this->disabledPluginNames.insert(item.as<std::string>());
                }
            }

            temp = root["graphOutputJsonConfig"];
            if (temp.IsDefined()) {
                temp >> this->graphOutputJsonConfig;
            }

        } catch (const YAML::BadConversion & e){
            logDebug("load project config error. $0, $1", path.c_str(), e.what());
            return CODE_FILE_FORMAT_ERROR;
        }

        return CODE_OK;
    }

    int Project::loadStyleInfo() {
        std::string path = pathStyleConfigFile();

        std::ifstream fin(path);
        if (!fin.is_open()) {
            return CODE_FILE_ERROR;
        }

        try {
            auto root = YAML::Load(fin);
            auto temp = root[whoAmI];
            if (temp.IsDefined()) {
                if (temp.as<std::string>() != "sight-styles") {
                    logDebug("style file who-am-i error: $0", path);
                    return CODE_FILE_ERROR;
                }
            }

            temp = root["typeMap"];
            if (temp.IsDefined()) {
                for (const auto& item : temp) {
                    auto id = item.first.as<uint>();
                    TypeStyle typeStyle = {
                        item.second["color"].as<uint>(),
                        static_cast<IconType>(item.second["icon"].as<int>())
                    };

                    auto iter = typeInfoMap.find(id);
                    if (iter == typeInfoMap.end()) {
                        logDebug("type not found: $0", id);
                        continue;
                    }

                    auto& typeInfo = iter->second;
                    if (typeInfo.style) {
                        *typeInfo.style = typeStyle;
                    } else {
                        typeInfo.style = typeStyleArray.add(typeStyle);
                    }
                }
            }

        } catch (const YAML::BadConversion& e) {
            logDebug("load project config error. $0, $1", path.c_str(), e.what());
            return CODE_FILE_FORMAT_ERROR;
        }
        return CODE_OK;
    }

    int Project::loadEntities() {
        auto path = pathEntitiesConfigFile();
        std::ifstream fin(path);
        if (!fin.is_open()) {
            return CODE_FILE_ERROR;
        }

        try {
            auto root = YAML::Load(fin);
            auto temp = root[whoAmI];
            if (temp.IsDefined()) {
                if (temp.as<std::string>() != "sight-entities") {
                    logDebug("entity file who-am-i error: $0", path);
                    return CODE_FILE_ERROR;
                }
            }

            for( const auto& item: root){
                auto key = item.first.as<std::string>();
                if (key == whoAmI) {
                    continue;
                }

                auto entity = loadEntity(key, item.second);
                this->entitiesMap[entity.name] = entity;

                getIntType(entity.name, true);
            }
            
        } catch (const YAML::BadConversion& e) {
            logDebug("load project config error. $0, $1", path.c_str(), e.what());
            return CODE_FILE_FORMAT_ERROR;
        }

        return CODE_OK;
    }

    int Project::loadPlugins() {
        auto path = pathPluginsFolder();
        // loop of path child folder
        auto mgr = pluginManager();
        for (const auto& child : std::filesystem::directory_iterator(path)) {
            int i = mgr->loadPluginAt(child.path().string());

            if (i != CODE_OK) {
                return i;
            }
        }
        
        return CODE_OK;
    }

    void Project::initTypeMap() {

        ImU32 color = IM_COL32(0, 99, 160, 255);
        addTypeInfo({ .name = "void", .intValue = IntTypeVoid }, { IM_COL32(255,255,255,255), IconType::Circle });
        addTypeInfo({ .name = "Process", .intValue = IntTypeProcess }, { color, IconType::Flow });
        addTypeInfo({ .name = "int", .intValue = IntTypeInt}, { color, IconType::Circle });
        addTypeInfo({ .name = "float", .intValue = IntTypeFloat }, { color, IconType::Circle });
        addTypeInfo({ .name = "double", .intValue = IntTypeDouble }, { color, IconType::Circle });
        addTypeInfo({ .name = "long", .intValue = IntTypeLong }, { color, IconType::Circle });
        addTypeInfo({ .name = "bool", .intValue = IntTypeBool }, { color, IconType::Circle });
        addTypeInfo({ .name = "char", .intValue = IntTypeChar }, { color, IconType::Circle });
        addTypeInfo({ .name = "Color", .intValue = IntTypeColor }, { color, IconType::Circle });
        addTypeInfo({ .name = "Vector3", .intValue = IntTypeVector3 }, { color, IconType::Circle });
        addTypeInfo({ .name = "Vector4", .intValue = IntTypeVector4 }, { color, IconType::Circle });
        addTypeInfo({ .name = "String", .intValue = IntTypeString }, { color, IconType::Circle });
        addTypeInfo({ .name = "LargeString", .intValue = IntTypeLargeString }, { color, IconType::Circle });
        addTypeInfo({ .name = "Object", .intValue = IntTypeObject }, { color, IconType::Circle });
        addTypeInfo({ .name = "button", .intValue = IntTypeButton }, { color, IconType::Circle });

        // type alias
        typeMap["Number"] = IntTypeFloat;
        typeMap["process"] = IntTypeProcess;
        typeMap["string"] = IntTypeString;
        typeMap["object"] = IntTypeObject;
        typeMap["number"] = IntTypeFloat;
        typeMap["Flow"] = IntTypeProcess;
        typeMap["boolean"] = IntTypeBool;
    }

    uint Project::getIntType(const std::string &str, bool addIfNotFound) {
        auto a = typeMap.find(str);
        if (a != typeMap.end()) {
            return a->second;
        } else if (addIfNotFound) {
            return addType(str);
        }

        return 0;
    }

    bool Project::delIntType(uint type) {
        auto iter = typeInfoMap.find(type);
        if (iter == typeInfoMap.end()) {
            return false;
        }

        if (typeMap.erase(iter->second.name) <= 0) {
            logDebug("del failed from typeMap: $0", iter->second.name);
        }
        
        return typeInfoMap.erase(iter), true;
    }

    std::string Project::getBaseDir() const {
        return baseDir;
    }

    SightNodeGraph* Project::createGraph(std::string_view path, v8::Isolate* isolate, char* pathWithoutExtOut) {
        auto g = openGraph(path, isolate, pathWithoutExtOut);
        buildFilesCache();
        return g;
    }

    SightNodeGraph* Project::openGraph(std::string_view path, v8::Isolate* isolate, char* pathWithoutExtOut) {
        std::string targetPath = std::filesystem::absolute(path).generic_string();
        std::filesystem::path temp(targetPath);
        if (temp.has_extension()) {
            std::string ext = temp.extension().generic_string();
            if (ext == ".json" || ext == ".yaml") {
                targetPath = std::string(targetPath, 0, targetPath.rfind('.'));
            }
        }
        logDebug(targetPath);
        changeGraph(targetPath.c_str(), isolate);
        if (pathWithoutExtOut) {
            sprintf(pathWithoutExtOut, "%s", targetPath.c_str());
        }
        lastOpenGraph = targetPath;
        return currentGraph();
    }

    void Project::checkOpenLastGraph() {
        if (lastOpenGraph.empty()) {
            return;
        }

        uiChangeGraph(lastOpenGraph.c_str());
    }

    ProjectFile const& Project::getFileCache() const {
        return fileCache;
    }

    ProjectFile& Project::getFileCache() {    
        return fileCache;
    }

    void Project::addToFileCache(std::string_view name, ProjectFileType fileType) {
        std::filesystem::path filepath(name);
        std::filesystem::path projectFolder(baseDir);

        std::filesystem::path relativePath = std::filesystem::relative(filepath, projectFolder);
        logDebug("relativePath: $0", relativePath.generic_string());

        std::vector<std::string> tmpArray;
        for (const auto& child : relativePath) {
            tmpArray.push_back(child.generic_string());
        }

        auto parent = &this->fileCache;
        for (int i = 0; i < tmpArray.size() - 1; i++) {
            parent = parent->findChild(tmpArray[i]);
            if (parent == nullptr) {
                parent = parent->createChildDirectory(tmpArray[i]);
            }
        }

        // parent->files.push_back({});
        // auto& f = parent->files.back();
        // f.filename = filepath.filename().generic_string();
        // f.fileType = fileType;
        // f.path = filepath.generic_string();
        parent->files.emplace_back(fileType, filepath.generic_string(), filepath.filename().generic_string());
    }

    void Project::addNewGraphToFileCache(std::string_view graphName) {
        this->addToFileCache(pathGraph(graphName), ProjectFileType::Graph);
    }

    void Project::buildFilesCache() {
        fillFiles(this->fileCache);
    }

    int Project::openFile(ProjectFile const& file) {

        return CODE_NOT_IMPLEMENTED;
    }

    std::string const &Project::getTypeName(int type) {
        auto a = typeInfoMap.find(type);
        if (a != typeInfoMap.end()) {
            return a->second.name;
        }
        return emptyString;
    }

    bool Project::changeTypeName(std::string const& from, std::string const& to) {
        auto iter = typeMap.find(from);
        if (iter == typeMap.end()) {
            return false;
        }

        auto type = iter->second;
        auto &info = typeInfoMap[type];
        info.name = to;

        typeMap.erase(iter);
        typeMap[to] = type;
        return true;
    }

    uint Project::addType(const std::string &name) {
        return addType(name, ++(typeIdIncr));
    }

    uint Project::addTypeInfo(TypeInfo info, bool merge) {
        defaultTypeStyle.color = randomColor();
        return this->addTypeInfo(info, defaultTypeStyle, merge);
    }

    std::string Project::pathSrcFolder() const {
        return baseDir + "src/";
    }

    std::string Project::pathTargetFolder() const {
        return baseDir + "target/";
    }

    SightCodeSetSettings& Project::getSightCodeSetSettings() {
        return this->codeSetSettings;
    }

    SightCodeSetSettings const& Project::getSightCodeSetSettings() const {
        return this->codeSetSettings;
    }

    absl::flat_hash_set<std::string> const& Project::getDisabledPluginNames() const {

        return disabledPluginNames;
    }

    absl::flat_hash_set<std::string>& Project::getDisabledPluginNames() {
    
        return disabledPluginNames;    
    }

    SightNodeGraphOutputJsonConfig* Project::getGraphOutputJsonConfig() {
        return &(this->graphOutputJsonConfig);
    }

    void Project::resetTypeListCache() {
        typeListCache.clear();
        typeListCache.reserve(typeMap.size());
        
        for (auto const& item : typeMap) {
            typeListCache.push_back(item.first);
        }
    }

    std::vector<std::string> const& Project::getTypeListCache() const {
        return typeListCache;
    }

    std::string Project::pathPluginsFolder() const {
        return baseDir + "plugins/";
    }

    absl::btree_map<uint, TypeInfo> const& Project::getTypeInfoMap() const {
        return typeInfoMap;
    }

    std::string Project::getLastOpenGraph() const {
        return lastOpenGraph;
    }

    absl::btree_map<std::string, SightEntity> const& Project::getEntitiesMap() const {
        return this->entitiesMap;
    }

    SightEntity const* Project::getSightEntity(std::string_view fullName) const {
        return const_cast<Project*>(this)->getSightEntity(fullName);
    }

    SightEntity* Project::getSightEntity(std::string_view fullName) {
        auto iter = entitiesMap.find(fullName);
        if (iter == entitiesMap.end()) {
            return nullptr;
        }

        return &iter->second;
    }

    bool Project::hasEntity(std::string_view fullName) const {
        return entitiesMap.contains(fullName);
    }

    bool Project::addEntity(SightEntity const& entity) {
        if (this->entitiesMap.contains(entity.name)) {
            return false;
        }

        for( const auto& item: entity.fields){
            if (getIntType(item.type) <= 0) {
                logDebug("type not found: $0", item.type);
                return false;
            }
        }

        // update to template
        return (this->entitiesMap[entity.name] = entity).effect() == CODE_OK;
    }

    bool Project::updateEntity(SightEntity const& entity, SightEntity const& oldEntity) {
        for (const auto& item : entity.fields) {
            if (getIntType(item.type) <= 0) {
                logDebug("type not found, $0", item.type);
                return false;
            }
        }

        auto iter = this->entitiesMap.find(entity.name);
        auto& editTemplateAddress = oldEntity.templateAddress;

        // update type name.
        if (entity.name != oldEntity.name) {
            // name have been changed.

            // target name invalid.
            if (iter != this->entitiesMap.end()) {
                return false;
            }

            changeTypeName(oldEntity.name, entity.name);
        } else {
            if (iter == this->entitiesMap.end()) {
                return false;
            }
        }

        // update template address
        if (editTemplateAddress != entity.templateAddress) {
            // template address have been changed.

            // delete old
            // auto eraseResult = entitiesMap.erase(entity.name);
            // delTemplateNode(editTemplateAddress);
            auto eraseResult = delEntity(entity.name);
            assert(eraseResult);

            // add new one
            this->entitiesMap[entity.templateAddress] = entity;
        } else {
            // update fields
            auto& entityInMap = iter->second;
            auto& f = entityInMap.fields;
            f.clear();
            f.assign(entity.fields.begin(), entity.fields.end());

            entityInMap.name = entity.name;
        }

        // update template node
        addTemplateNode(entity, true);

        return true;
    }

    bool Project::delEntity(std::string_view fullName) {
        auto iter = this->entitiesMap.find(fullName);
        if(iter != entitiesMap.end()){
            // delete type
            auto & entity = iter->second;
            if (entity.typeId > 0) {
                delIntType(entity.typeId);
                entity.typeId = 0;
            }
            delTemplateNode(entity.templateAddress);
            entitiesMap.erase(iter);
            return true;
        }
        return false;
    }

    void Project::updateEntitiesToTemplateNode() const {
        for( const auto& item: this->entitiesMap){
            addTemplateNode(item.second);
        }
    }

    void Project::parseAllGraphs() const {
        auto targetPathString = pathTargetFolder();
        targetPathString += "graph/";

        for (const auto& item : directory_iterator(pathGraphFolder())) {
            if (item.is_directory()) {
                
            } else if (item.is_regular_file()) {
                auto path = item.path();
                if (!path.has_extension() || path.extension().generic_string() != ".yaml") {
                    continue;
                }

                parseGraph(path.string());
            }
        }
    }

    bool Project::isAnyGraphHasTemplate(std::string_view templateAddress, std::string* pathOut) {
        auto g = currentGraph();
        std::string currentGraphPath;

        auto checkNodesFunc = [templateAddress, pathOut](SightNodeGraph* g) {
            for (const auto& item : g->getNodes()) {
                if (templateAddress == item.templateAddress()) {
                    if (pathOut) {
                        pathOut->assign(g->getFilePath());
                    }
                    return true;
                }
            }
            return false;
        };

        if (g) {
            if (checkNodesFunc(g)) {
                return true;
            }
            currentGraphPath = g->getFilePath();
        }

        for (const auto& item : directory_iterator(pathGraphFolder())) {
            if (!item.is_regular_file()) {
                continue;
            }

            auto path = item.path();
            if (!path.has_extension() || path.extension().generic_string() != ".yaml") {
                continue;
            }
            if (path.generic_string() == currentGraphPath) {
                continue;
            }

            SightNodeGraph graph;
            if (graph.load(path.string()) != CODE_OK) {
                logDebug("graph load failed: $0", path.c_str());
                continue;
            }

            if (checkNodesFunc(&graph)) {
                return true;
            }
        }
        return false;
    }

    std::string Project::pathEntityFolder() const {
        return pathSrcFolder() + "entity/";
    }

    std::string Project::pathGraph(std::string_view graphName) const {
        return absl::Substitute("$0$1", pathGraphFolder(), graphName);
    }

    void Project::initFolders() {
        std::filesystem::create_directories(pathEntityFolder());
        std::filesystem::create_directories(pathTargetFolder());
        std::filesystem::create_directories(pathPluginsFolder());
        std::filesystem::create_directories(pathGraphFolder());
    }

    uint Project::addType(std::string const& name, uint v) {
        return this->addTypeInfo({
            .name = name,
            .intValue = v,
        });
    }

    uint Project::addTypeInfo(TypeInfo info, TypeStyle const& typeStyle, bool merge) {
        uint id = 0;
        if (info.intValue > 0) {
            id = info.intValue;
        } else {
            id = getIntType(info.name);
        }

        if (id > 0) {
            auto iter = typeInfoMap.find(id);
            if (iter != typeInfoMap.end()) {
                if (merge) {
                    iter->second.mergeFrom(info);

                    if (info.style) {
                        typeStyleArray.remove(info.style);
                        info.style = nullptr;
                    }
                    logDebug("$0, $1, type merged.", iter->first, iter->second.name);
                    return iter->first;
                } else {
                    logDebug("already exist: $0", info.intValue);
                    return 0;
                }
            }
        }

        if (id <= 0) {
            info.intValue = ++typeIdIncr;
        } 
        if (!info.style) {
            info.style = typeStyleArray.add(typeStyle);
            // info.style->init();
        }
        typeInfoMap[info.intValue] = info;
        typeMap[info.name] = info.intValue;

        resetTypeListCache();
        return info.intValue;
    }

    TypeInfo const& Project::getTypeInfo(uint id, bool* isFind) {
        auto iter = typeInfoMap.find(id);
        if (iter == typeInfoMap.end()) {
            if (isFind) {
                *isFind = false;
            }
        } else {
            if (isFind) {
                *isFind = true;
            }
            return iter->second;
        }
        return emptyTypeInfo;
    }

    std::tuple<TypeInfo const&, bool> Project::findTypeInfo(uint id) {
        bool find = false;
        auto & typeInfo = getTypeInfo(id, &find);
        return std::forward_as_tuple(typeInfo, find);
        // return std::make_tuple(typeInfo, find);
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
        project->updateEntitiesToTemplateNode();
        project->checkOpenLastGraph();

    }

    int openProject(const char* baseDir, bool createIfNotExist, bool callLoadSuccess) {
        if (g_Project) {
            return CODE_FAIL;    
        }

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
        if (code == CODE_OK && callLoadSuccess) {
            logDebug(baseDir);
            onProjectAndUILoadSuccess(g_Project);
        }
        return code;
    }

    int disposeProject() {
        if (g_Project) {
            g_Project->save();
            delete g_Project;
            g_Project = nullptr;
        }
        return CODE_OK;
    }

    int verifyProjectFolder(const char* folder) {
        if (fs::exists(folder) && fs::is_directory(folder)) {
            auto begin = fs::directory_iterator{folder};
            if (begin != fs::directory_iterator{}) {
                // has files
                auto path = fs::path(folder);
                path /= "project.yaml";
                logDebug(path.c_str());

                if (!fs::exists(path) || !fs::is_regular_file(path)) {
                   return CODE_FAIL; 
                }

                // load it
                std::ifstream fin(path);
                if (!fin.is_open()) {
                    return CODE_FAIL;
                }

                auto root = YAML::Load(fin);
                auto n = root[whoAmI];
                if (!n.IsDefined() || n.as<std::string>() != sightIsProject) {
                    return CODE_FAIL;
                }
            }
        }
        return CODE_OK;
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

    uint addTypeInfo(TypeInfo const& info, bool merge) {
        return g_Project->addTypeInfo(info, merge);
    }

    std::tuple<const char**, size_t> getIconTypeStrings() {
        return std::make_tuple(iconTypeStrings, std::size(iconTypeStrings));
    }

    const char* getIconTypeName(IconType iconType) {
        return iconTypeStrings[static_cast<int>(iconType) - static_cast<int>(IconType::Flow)];
    }

    bool checkTypeCompatibility(uint type1, uint type2) {
        if (type1 == type2) {
            return true;
        }

        return type2 == IntTypeObject && type1 != IntTypeProcess;
    }

    TypeInfoRender::TypeInfoRender()
    {
        this->data.i = 0;
    }

    TypeInfoRender::TypeInfoRender(TypeInfoRender const& rhs)
    {
        // copy 
        operator=(rhs);
    }

    TypeInfoRender::~TypeInfoRender()
    {
        freeData();
    }

    void TypeInfoRender::initData() {
        switch (kind) {
        case TypeInfoRenderKind::Default:
            break;
        case TypeInfoRenderKind::ComboBox:
            auto& data = this->data.comboBox;
            data.selected = 0;
            data.list = new std::vector<std::string>();
            break;
        }
    }

    void TypeInfoRender::freeData() {
        if (this->kind == TypeInfoRenderKind::ComboBox) {
            if (this->data.comboBox.list) {
                delete this->data.comboBox.list;
                this->data.comboBox.list = nullptr;
            }
            this->data.comboBox.selected = 0;
        }
    }

    void TypeInfoRender::operator()(const char* labelBuf, SightNodePort* port, std::function<void()> onValueChange) const {
        SightNodeValue& value = port->value;
        auto oldValue = port->value;
        auto & options = port->options;
        switch (kind) {
        case TypeInfoRenderKind::Default:
            ImGui::Text(" ");
            break;
        case TypeInfoRenderKind::ComboBox:
            auto& data = this->data.comboBox;
            if (data.list->empty()) {
                ImGui::Text("combo-no-data");
                break;
            }

            if (options.readonly) {
                ImGui::BeginDisabled();
            }
            // this will not working on graph, but works on inspector.
            // maybe i will fix it on someday.
            // fixme: https://github.com/thedmd/imgui-node-editor/issues/101
            if (ImGui::BeginCombo(labelBuf, data.list->at(value.u.i).c_str())) {
                auto & list = *data.list;
                for( int i = 0; i < list.size(); i++){
                    if (ImGui::Selectable(list[i].c_str(), value.u.i == i)) {
                        if (value.u.i != i) {
                            value.u.i = i;
                            // onNodePortValueChange(port);
                            onValueChange();
                        }
                    }
                }

                ImGui::EndCombo();
            }

            if (options.readonly) {
                ImGui::EndDisabled();
            }

            break;
        }
    }

    TypeInfoRender::operator bool() {
        return kind != TypeInfoRenderKind::Default;
    }

    TypeInfoRender& TypeInfoRender::operator=(TypeInfoRender const& rhs) {
        // copy
        freeData();
        this->kind = rhs.kind;
        initData();

        switch (kind) {
        case TypeInfoRenderKind::Default:
            break;
        case TypeInfoRenderKind::ComboBox:
            auto& data = this->data.comboBox;
            auto & targetData = rhs.data;

            data.list->assign(targetData.comboBox.list->begin(), targetData.comboBox.list->end()); 
            data.selected = targetData.comboBox.selected;
            break;
        }
        return *this;
    }

    void TypeInfo::initValue(SightNodeValue & value) const {
        switch (render.kind) {
        case TypeInfoRenderKind::Default:
            break;
        case TypeInfoRenderKind::ComboBox:
            value.u.i = render.data.comboBox.selected;
            break;
        }
    }

    void TypeInfo::mergeFrom(TypeInfo const& rhs) {
        // id, name do not change.
        this->fromNode = rhs.fromNode;
        this->render = rhs.render;
        // if (rhs.style) {
        //     this->style = typeStyleArray.add((*rhs.style));
        // }
    }

    std::string TypeInfo::getSimpleName() const {
        return getLastAfter(name, ".");
    }

    bool TypeInfo::writeToJson(SightNodeValue const& value, crude_json::value & parent, std::string_view key) const {
        switch (render.kind) {
            case TypeInfoRenderKind::Default:
                break;
            case TypeInfoRenderKind::ComboBox:
                parent[key.data()] = crude_json::value(static_cast<double>(value.u.i));
                return true;
        }
        
        return false;
    }

    void TypeStyle::init() {
        this->iconType = IconType::Circle;
        this->color = IM_COL32_WHITE;
    }

    SightEntityField::SightEntityField(std::string name, std::string type, std::string defaultValue)
        : name(std::move(name))
        , type(std::move(type))
        , defaultValue(std::move(defaultValue)) {
    }

    void SightEntity::fixTemplateAddress() {
        auto simpleName = this->getSimpleName();

        if (!startsWith(templateAddress, "entity/")) {
            templateAddress = "entity/" + templateAddress;
        }
        if (!endsWith(templateAddress, simpleName)) {
            if (!endsWith(templateAddress, "/")) {
                templateAddress += "/";
            }
            templateAddress += simpleName;
        }
    }

    int SightEntity::effect() const {
        auto &entity = *this;
        this->typeId = sight::getIntType(entity.name, true);
    
        // update to template
        return addTemplateNode(entity);
    }

    std::string SightEntity::getSimpleName() const {
        return getLastAfter(name, ".");
    }

    std::string SightEntity::getPackageName() const {
        auto pos = name.rfind(".");
        if (pos == std::string::npos) {
            return {};    // no package name
        }

        return name.substr(0, pos);
    }

    SightEntity::operator SightNodeTemplateAddress() const {
        SightNodeTemplateAddress address;
        address.name = this->templateAddress;
        address.templateNode = new SightJsNode();
        
        auto & node = *address.templateNode;
        node.nodeName = this->getSimpleName();
        node.fullTemplateAddress = this->templateAddress;

        for( const auto& item: this->fields){
            SightJsNodePort port{
                item.name,
                item.options.portType
            };
            port.type = getIntType(item.type);
            port.value.setType(port.type);
            if (!item.defaultValue.empty()) {
                port.value.setValue(item.defaultValue);
            }
            port.options = item.options.portOptions;

            node.addPort(port);
        }
        node.options.titleBarPortType = sight::getIntType(this->name, true);
        registerEntityFunctions(node);
        return address;
    }

    int SightEntityFieldOptions::portTypeValue() const {
        return static_cast<int>(portType);
    }

    ProjectFile::ProjectFile(ProjectFileType fileType, std::string path, std::string filename)
        : fileType(fileType),
          path(std::move(path)),
          filename(std::move(filename))
    {
        
    }

    bool ProjectFile::deleteFile() {
        if (fileType == ProjectFileType::Deleted) {
            return true;
        }

        if (fileType == ProjectFileType::Directory) {
            logWarning("delete file, unsupported type, path: $0", this->path);
            return false;
        }

        if (fileType == ProjectFileType::Regular) {
            if (std::filesystem::remove(this->path)) {
                this->fileType = ProjectFileType::Deleted;
                return true;
            }
        } else if (fileType == ProjectFileType::Graph) {
            // delete this and json file
            // close graph first.
            auto deleteAsGraph = [this]() -> bool{
                std::string tmp = removeExt(this->path);
                tmp += ".json";
                return std::filesystem::remove(this->path) && std::filesystem::remove(tmp);
            };
            auto graphOpened = this->isGraph(currentGraph());
            if (graphOpened) {
                disposeGraph();
            }
            if (deleteAsGraph()) {
                this->fileType = ProjectFileType::Deleted;
                return true;
            }
        }
        return false;
    }

    bool ProjectFile::rename(std::string_view newName, v8::Isolate* isolate) {

        if (fileType == ProjectFileType::Deleted || fileType == ProjectFileType::Plugin) {
            return false;
        }

        if (fileType == ProjectFileType::Graph) {
            if (!isValidPath(newName)) {
                logDebug("$0 is not a valid path!", newName);
                return false;
            }

            std::filesystem::path newPath(newName);
            if (newPath.has_extension()) {
                auto str = newPath.extension().generic_string();
                if (str == "yaml") {
                
                } else if (str == "json") {
                    // change to yaml
                    newPath.replace_extension("yaml");
                } else {
                    logError("wrong ext name: $0", str);
                    return false;
                }
            } else {
                newPath += ".yaml";
            }

            logDebug("newPath: $0", newPath.generic_string());

            // rename yaml and json, and graph name
            auto openedGraph = currentGraph();
            bool isCurrentGraph = false;
            if (this->isGraph(openedGraph)) {
                disposeGraph();
                isCurrentGraph = true;
            }

            // load this.path as ifstream
            std::ifstream ifs(this->path);
            if (ifs.good()) {
                auto node = YAML::Load(ifs);
                node["settings"]["graphName"] = removeExt(newPath.filename().generic_string());

                // write back to file
                YAML::Emitter emitter;
                emitter << node;
                ifs.close();

                std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
                outToFile << emitter.c_str() << std::endl;
                outToFile.close();
            }

            auto jsonPath = removeExt(this->path);
            jsonPath += ".json";

            std::filesystem::path newJsonPath(newPath);
            newJsonPath.replace_extension("json");

            // rename
            try {
                std::filesystem::rename(this->path, newPath);
                std::filesystem::rename(jsonPath, newJsonPath);
            } catch (std::filesystem::filesystem_error& e) {
                logError("rename file failed, path: $0, error: $1", this->path, e.what());
                return false;
            }


            if (isCurrentGraph) {
                // open
                auto project = currentProject();
                project->openGraph(this->path, isolate);
            }
            this->refreshInfo(newPath.generic_string());
            return true;

        } else {
            // just rename
            try {
                std::filesystem::rename(this->path, newName);
                this->refreshInfo(newName);
            } catch (std::filesystem::filesystem_error& e) {
                logError("rename file failed, path: $0, error: $1", this->path, e.what());
                return false;
            }
        }
        return false;
    }

    bool ProjectFile::isGraph(SightNodeGraph* graph) const {
        if (!graph) {
            return false;
        }

        return std::filesystem::path(graph->getFilePath()) == std::filesystem::path(this->path);
    }

    void ProjectFile::refreshInfo(std::string_view newPath) {
        this->path = newPath;
        this->filename = std::filesystem::path(newPath).filename().generic_string();
        this->filename = removeExt(this->filename);
    }

    ProjectFile* ProjectFile::findChild(std::string_view filename) {
        // loop of this->files
        for (auto& file : this->files) {
            if (file.filename == filename) {
                return &file;
            }
        }

        return nullptr;
    }

    ProjectFile* ProjectFile::createChildDirectory(std::string_view directoryName) {
        // todo
        assert(this->fileType == ProjectFileType::Directory);

        std::filesystem::path path(this->path);
        try {
            std::filesystem::create_directory(path / directoryName);
        } catch (const std::exception& e) {
            logError("create directory failed, path: $0, error: $1", this->path, e.what());
        }

        return addChildDirectory(directoryName);
    }

    ProjectFile* ProjectFile::addChildDirectory(std::string_view directoryName) {
        // Only add child directory item to this.files
        assert(this->fileType == ProjectFileType::Directory);

        std::filesystem::path path(this->path);
        path /= directoryName;

        this->files.emplace_back(ProjectFileType::Directory, path.generic_string(), std::string(directoryName));
        return &this->files.back();

    }


}