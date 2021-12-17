// Main window ui
//
#pragma once
#include "imgui.h"
#include "sight_defines.h"
#include "uv.h"

#include "sight.h"
#include "sight_widgets.h"
#include "sight_language.h"

#include "absl/container/flat_hash_set.h"
#include <functional>
#include <string>
#include <sys/types.h>
#include <v8.h>
#include <string_view>


namespace sight {

    struct SightNode;
    struct SightNodeConnection;
    struct SightEntity;

    enum class UICommandType{
        UICommandHolder,

        // common ui part
        COMMON = 100,
        JsEndInit,

        // node editor part
        AddNode = 200,
        AddTemplateNode = 201,

        // script part

        // clean data after used.
        RegScriptGlobalFunctions = 300,
        RunScriptFile,

    };

    /**
     * @brief Imgui ids
     * 
     */
    struct MyUILabels{
        // static ids
        static constexpr const char* modalAskData = "###modalAskData";
        static constexpr const char* modalSaveData = "###modalSaveData";
        static constexpr const char* modalAlertData = "###modalAlertData";

        // dynamic ids, strcat on program load.
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
        bool edit = false;
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

        void loadFrom(SightEntity const& info);

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
        bool aboutWindow = false;
        bool projectSettingsWindow = false;
        bool entityListWindow = false;

        bool layoutReset = false;

        // popups
        bool popupGraphName = false;
        bool popupAskModal = false;
        bool popupSaveModal = false;
        bool popupAlertModal = false;
    };

    struct UIColors {

        ImU32 grey = IM_COL32(128,128,128,255);
        ImU32 white = IM_COL32(255,255,255,255);

        ImU32 readonlyText = grey;
        // ImU32 nodeIdText = IM_COL32(40,56,69,255);
        ImVec4 nodeIdText = rgb(51, 184, 255);
        ImVec4 errorText = rgba(239, 35, 60, 255);

        UIColors();
    };

    struct LoadingStatus {
        bool jsThread = false;

        bool isLoadingOver() const;
    };

    class Project;
    struct KeyBindings;

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
        // node or port or connection 
        absl::flat_hash_set<uint> selectedItems;

        std::string getProjectPath() const;

        void resetSelectedNodes();

        friend void onProjectAndUILoadSuccess(Project* project);
    };

    struct UIBuffer {
        char inspectorNodeName[LITTLE_NAME_BUF_SIZE]{0};

        char littleName[LITTLE_NAME_BUF_SIZE] {0};
        char name[NAME_BUF_SIZE]{0};
        char entityListSearch[TINY_NAME_BUF_SIZE]{ 0 };
    };

    struct ModalAskData {
        std::string title;
        std::string content;

        std::function<void(bool)> callback;
    };

    struct ModalAlertData {
        std::string title;
        std::string content;
    };

    enum class SaveOperationResult {
        // save file
        Save,
        // do not do anything
        Cancel,
        // do not save file
        Drop,
    };

    struct ModalSaveData {
        std::string title;
        std::string content;

        std::function<void(SaveOperationResult)> callback;
    };

    struct UIStatus {
        bool needInit = false;
        ImGuiIO* io;
        bool closeWindow = false;
        struct UIWindowStatus windowStatus;
        struct UICreateEntity createEntityData;
        struct LoadingStatus loadingStatus;
        class Selection selection;
        struct UIBuffer buffer;
        struct ModalAskData modalAskData;
        struct ModalSaveData modalSaveData;
        struct ModalAlertData modalAlertData;

        struct LanguageKeys* languageKeys = nullptr;
        struct UIColors* uiColors = nullptr;
        struct KeyBindings* keybindings = nullptr;

        // for execute js code in ui thread.
        v8::Isolate* isolate = nullptr;
        v8::ArrayBuffer::Allocator* arrayBufferAllocator = nullptr;
        v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context>> v8GlobalContext{};

        uv_loop_t* uvLoop = nullptr;
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

    int addUICommand(UICommandType type, const char *argString, int length = 0, bool needFree = true);

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

    /**
     * @brief 
     * 
     * @param node 
     * @param showField    if true, show all fields.
     * @param showValue    if true, then node port config `showValue=true`'s port will be showed.
     * @param showOutput   if true, show all output ports.
     * @param showInput    if true, show all input ports.
     */
    void showNodePorts(SightNode* node, bool showField = true, bool showValue = true, bool showOutput = false, bool showInput = false);

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    void uiChangeGraph(const char* path );

    /**
     * @brief open a save modal on next frame.
     * 
     * @param title 
     * @param content 
     * @param callback 
     */
    void openSaveModal(const char* title, const char* content, std::function<void(SaveOperationResult)> const& callback);

    bool isUICommandFree();

    void openAskModal(std::string_view title, std::string_view content, std::function<void(bool)> callback);

    void openAlertModal(std::string_view title = "Alert!", std::string_view content = "");

}
