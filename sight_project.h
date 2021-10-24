//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include "atomic"
#include "sys/types.h"

#include "absl/container/btree_map.h"

namespace sight {

    union SightNodeValue;
    class SightNodeGraph;
    struct SightNodePort;

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

    enum class TypeInfoRenderKind {
        Default,
        ComboBox,

    };

    /**
     * @brief Copy from imgui-node-editor/examples/buildprints-example/utilities/drawing.h
     * 
     */
    enum class IconType {
        Flow = 100,
        Circle,
        Square,
        Grid,
        RoundSquare,
        Diamond
    };

    /**
     * @brief Used for port icon.
     * 
     */
    struct TypeStyle {
        uint color = 0;
        IconType iconType = IconType::Circle;

        void init();
    };

    struct TypeInfoRender {
        TypeInfoRenderKind kind = TypeInfoRenderKind::Default;
        // if asIntType > 0, then it's value should be one of TypeIntValues
        u_char asIntType = 0;
        union {
            struct {
                std::vector<std::string>* list;
                int selected;
            } comboBox;
            int i = 0;
        } data;

        TypeInfoRender();
        TypeInfoRender(TypeInfoRender const& rhs);
        ~TypeInfoRender();

        /**
         * @brief init data by kind
         * 
         */
        void initData();
        void freeData();

        void operator()(const char* labelBuf, SightNodePort* port) const;

        /**
         * @brief Does this render has a render function ?
         * 
         * @return true 
         * @return false 
         */
        operator bool();

        TypeInfoRender& operator=(TypeInfoRender const& rhs);
        
    };

    struct TypeInfo {
        // typename
        std::string name;
        // id
        uint intValue = 0;
        // does this type from a node ? ( > 0)
        uint fromNode = 0;

        TypeStyle* style = nullptr;
        TypeInfoRender render;

        void initValue(SightNodeValue & value) const;

        /**
         * @brief Only merge fromNode, render,
         * 
         * @param rhs 
         */
        void mergeFrom(TypeInfo const& rhs);
    };


    struct ProjectConfig {
        std::atomic<uint> nodeOrPortId = 3000;
    };


    /**
     * @brief build target 
     */
    struct BuildTarget {
        std::string name;
        // if has children, then execute them in order.
        std::vector<std::string> children;
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
         * @brief load type style, 
         * 
         * @return int 
         */
        int loadStyleInfo();

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
        int saveStyleInfo();


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

        uint addTypeInfo(TypeInfo info, bool merge = false);

        /**
         * @brief 
         * 
         * @param info if info has a style, and info need merge, then the style will be free.
         * @param merge 
         * @return uint 0=add fail. 
         */
        uint addTypeInfo(TypeInfo info, TypeStyle const& typeStyle, bool merge = false);

        TypeInfo const& getTypeInfo(uint id, bool* isFind = nullptr);

        std::tuple<TypeInfo const&, bool> findTypeInfo(uint id);

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

        absl::btree_map<std::string, BuildTarget> & getBuildTargetMap();

        absl::btree_map<uint, TypeInfo> const& getTypeInfoMap() const;

        std::string getLastOpenGraph() const;

    private:
        std::string baseDir;
        bool createIfNotExist;
        ProjectConfig projectConfig;
        ProjectFile fileCache;

        std::atomic<uint> typeIdIncr;
        absl::btree_map<std::string, uint> typeMap;
        absl::btree_map<uint, TypeInfo> typeInfoMap;

        std::string lastOpenGraph{};

        // key: name, 
        absl::btree_map<std::string, BuildTarget> buildTargetMap;

        // file locations
        std::string pathConfigFile() const;
        std::string pathStyleConfigFile() const;

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

    uint addTypeInfo(TypeInfo const& info, bool merge = false);

    std::tuple<const char**, size_t> getIconTypeStrings();

    const char* getIconTypeName(IconType iconType);



    bool inline isBuiltInType(uint type){
        return type < IntTypeNext;
    }



}