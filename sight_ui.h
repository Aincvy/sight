// Main window ui
//
#pragma once
#include "imgui.h"
#include "uv.h"

#include "sight.h"
#include "sight_language.h"

#include "absl/container/flat_hash_set.h"

namespace sight {

    struct SightNode;
    struct SightNodeConnection;

    inline ImVec4 rgba(int r, int g, int b, int a){
        return ImVec4{ static_cast<float>(r) / 255.f, static_cast<float>(g) / 255.f, static_cast<float>(b) / 255.f, static_cast<float>(a) / 255.f };
    }

    inline ImVec4 rgb(int r, int g,int b){
        return rgba(r,g,b, 255);
    }

    enum class UICommandType{
        UICommandHolder,

        // common ui part
        COMMON = 100,
        JsEndInit,

        // node editor part
        AddNode = 200,
        AddTemplateNode = 201,
    };

    struct UICommand {
        UICommandType type = UICommandType::UICommandHolder;

        CommandArgs args;
    };

    struct EntityField {
        char name[NAME_BUF_SIZE] = {0};
        char type[NAME_BUF_SIZE] = {0};
        char defaultValue[NAME_BUF_SIZE] = {0};
        bool selected = false;
        bool editing = false;

        struct EntityField* next = nullptr;
        struct EntityField* prev = nullptr;
    };

    /**
     * cache for create entity window.
     */
    struct UICreateEntity {
        char name[NAME_BUF_SIZE] = {0};
        char templateAddress[NAME_BUF_SIZE] = {0};
        struct EntityField* first = nullptr;



        void addField();

        void resetFieldsStatus(bool editing = false, bool selected = false);

        EntityField* lastField();

        /**
         *
         * @return number.
         */
        int deleteSelected();

        /**
         * move selected item up
         */
        void moveItemUp();

        /**
         * move selected item down
         */
        void moveItemDown();

        void loadFrom(struct SightNode* node);

        void reset();

    private:

        /**
         * For internal use.
         * @return
         */
        EntityField* findAttachTo();

        /**
         *
         * @param statusCode if has more than 1 select item, it will be -1, otherwise 0.
         * @param resetOthers
         * @return The first selected element. First by index,not user's selection index.
         */
        EntityField* findSelectedEntity(int *statusCode, bool resetOthers = false);

    };

    struct UIWindowStatus {
        bool nodeGraph = false;
        bool createEntity = false;
        bool testWindow = false;
    };

    struct UIColors {

        ImU32 grey = IM_COL32(128,128,128,255);
        ImU32 white = IM_COL32(255,255,255,255);

        ImU32 readonlyText = grey;
        // ImU32 nodeIdText = IM_COL32(40,56,69,255);
        ImVec4 nodeIdText = rgb(51, 184, 255);

        UIColors();
    };

    struct LoadingStatus {
        bool jsThread = false;

        bool isLoadingOver() const;
    };

    class Project;

    /**
     * @brief select items, path
     * Not thread safe.
     */
    class Selection {
        // selected project path, default is project root path.
        std::string projectPath;
        
    public:
        absl::flat_hash_set<std::string> selectedFiles;
        // selected node 
        SightNode* node = nullptr;
        SightNodeConnection* connection = nullptr;

        std::string getProjectPath() const;


        friend void onProjectLoadSuccess(Project* project);
    };


    struct UIStatus {
        bool needInit = false;
        ImGuiIO* io;
        bool closeWindow = false;
        struct UIWindowStatus windowStatus;
        struct UICreateEntity createEntityData;
        struct LoadingStatus loadingStatus;
        class Selection selection;

        struct LanguageKeys* languageKeys = nullptr;
        struct UIColors* uiColors = nullptr;

        uv_loop_t *uvLoop = nullptr;
        uv_async_t* uvAsync = nullptr;

        ~UIStatus();

        bool isLoadingOver() const;
    };


    int initOpenGL();

    int showLoadingWindow();

    /**
     * init window function.
     * @return
     */
    int showMainWindow();

    /**
     * this will stop glfw
     */
    void destroyWindows();

    /**
     * add a ui command
     * @param type
     * @param argInt
     */
    void addUICommand(UICommandType type, int argInt = 0);

    /**
     *
     * @param type
     * @param data
     * @param needFree
     * @return
     */
    int addUICommand(UICommandType type, void *data, size_t dataLength = 0, bool needFree = true);

    /**
     *
     * @param command
     * @return
     */
    int addUICommand(UICommand & command);

    /**
     * for draw ui.
     * *It has a question, the text is not aligned with the input.*
     * @param label
     * @param buf
     * @param bufSize
     * @return
     */
    bool LeftLabeledInput(const char* label, char* buf, size_t bufSize);

    /**
     * @brief 
     * 
     * @return UIStatus*  current active global ui status
     */
    UIStatus* currentUIStatus();

}
