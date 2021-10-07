//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_project.h"
#include "dbg.h"
#include "imgui.h"
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
#include <tuple>
#include <vector>

namespace sight {

    using directory_iterator = std::filesystem::directory_iterator;

    static Project* g_Project = nullptr;
    static TypeInfo emptyTypeInfo;

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
        addType("int", IntTypeInt);
        addType("float", IntTypeFloat);
        addType("double", IntTypeDouble);
        addType("long", IntTypeLong);
        addType("bool", IntTypeBool);
        addType("Color", IntTypeColor);
        addType("Vector3", IntTypeVector3);
        addType("Vector4", IntTypeVector4);
        addType("Process", IntTypeProcess);
        addType("String", IntTypeString);
        addType("Object", IntTypeObject);
        addType("Number", IntTypeFloat);

        // type alias
        typeMap["process"] = IntTypeProcess;
        typeMap["string"] = IntTypeString;
        typeMap["object"] = IntTypeObject;
        typeMap["number"] = IntTypeFloat;
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
        lastOpenGraph = targetPath;
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
        auto a = typeInfoMap.find(type);
        if (a != typeInfoMap.end()) {
            return a->second.name;
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
        return this->addTypeInfo({
            .name = name,
            .intValue = v,
        });
    }

    uint Project::addTypeInfo(TypeInfo info) {
        if (info.intValue <= 0) {
            info.intValue = ++typeIdIncr;
        }

        typeInfoMap[info.intValue] = info;
        typeMap[info.name] = info.intValue;
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
        return std::make_tuple(typeInfo, find);
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

    uint addTypeInfo(TypeInfo const& info) {
        return g_Project->addTypeInfo(info);
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

    void TypeInfoRender::operator()(const char* labelBuf, SightNodeValue& value) const{
        switch (kind) {
        case TypeInfoRenderKind::Default:
            break;
        case TypeInfoRenderKind::ComboBox:
            auto& data = this->data.comboBox;
            if (data.list->empty()) {
                ImGui::Text("combo-no-data");
                break;
            }

            // this will not working on graph, but works on inspector.
            // maybe i will fix it on someday.
            // fixme: https://github.com/thedmd/imgui-node-editor/issues/101
            if (ImGui::BeginCombo(labelBuf, data.list->at(value.i).c_str())) {
                auto & list = *data.list;
                for( int i = 0; i < list.size(); i++){
                    if (ImGui::Selectable(list[i].c_str(), value.i == i)) {
                        value.i = i;
                    }
                }

                ImGui::EndCombo();
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
            value.i = render.data.comboBox.selected;
            break;
        }
    }
    
}