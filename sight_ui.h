// Main window ui
//
#pragma once
#include "imgui.h"
#include "sight_colors.h"
#include "uv.h"

#include "sight.h"
#include "sight_widgets.h"
#include "sight_language.h"
#include "sight_project.h"
#include "sight_nodes.h"

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include <functional>
#include <string>
#include <sys/types.h>
#include <v8.h>
#include <string_view>
#include <vector>


namespace sight {

    struct SightNode;
    struct SightNodeConnection;

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
        PluginReloadOver, 

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
        bool editing = false;

        SightEntityFieldOptions fieldOptions;
    };

    /**
     * cache for create entity window.
     */
    struct UICreateEntity {
        bool edit = false;
        bool useRawTemplateAddress = false;     // use raw template address string ?
        bool wantUpdateUsedEntity = false;
        SightEntity editingEntity{};
        char name[NAME_BUF_SIZE] = {0};
        char templateAddress[NAME_BUF_SIZE] = {0};
        char parentEntity[NAME_BUF_SIZE] = {0};
        // struct EntityField* first = nullptr;
        std::vector<EntityField> fields;
        std::string showInfoEntityName;
        int selectedFieldIndex = -1;

        void addField();

        /**
         * @brief 
         * 
         * @param editing  reset editing ?
         * @param selected  reset selected ?
         */
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

        bool isEditMode() const;

    private:
    };

    struct UIColors {
        ImU32 readonlyText = KNOWNIMGUICOLOR_GRAY;
        // ImU32 nodeIdText = IM_COL32(40,56,69,255);
        ImVec4 nodeIdText = rgb(51, 184, 255);
        ImVec4 errorText = rgba(239, 35, 60, 255);

        UIColors();
    };

    struct LoadingStatus {
        bool jsThread = false;
        bool nodeStyle = false;

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

        absl::btree_set<std::string> selectedFiles;
        // node or port or connection
        absl::btree_set<uint> selectedNodeOrLinks;

        std::string getProjectPath() const;

        /**
         * @brief Get the Selected Node object
         * If multiple items is selected, then return a !!random!! one
         * @return SightNode* nullptr if no one node is selected.
         */
        SightNode* getSelectedNode() const;

        /**
         * @brief Get the Selected Connection object
         * If multiple items is selected, then return a !!random!! one
         * @return SightNodeConnection* nullptr if no one connection is selected.
         */
        SightNodeConnection* getSelectedConnection() const;

        int getSelected(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections) const;

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

    struct EntityOperations{
        absl::flat_hash_map<std::string, CommonOperation> map;
        std::vector<std::string> names;
        int selected = 0;
        bool init = false;

        bool addOperation(const char* name, const char* desc, ScriptFunctionWrapper::Function const& f, bool replace = true);

        void reset();
    };

    struct GenerateResultData{
        std::string source;
        std::string text;
    };

    struct UIStatus {
        bool needInit = false;
        ImGuiIO* io;
        bool closeWindow = false;
        struct UIWindowStatus& windowStatus;
        struct UICreateEntity createEntityData;
        // struct SightEntity createEntityData;
        struct LoadingStatus loadingStatus;
        class Selection selection;
        struct UIBuffer buffer;
        struct ModalAskData modalAskData;
        struct ModalSaveData modalSaveData;
        struct ModalAlertData modalAlertData;
        struct ToastController toastController;
        struct GenerateResultData generateResultData;

        struct LanguageKeys* languageKeys = nullptr;
        struct UIColors* uiColors = nullptr;
        struct KeyBindings* keybindings = nullptr;

        // others
        struct EntityOperations entityOperations;

        // for execute js code in ui thread.
        v8::Isolate* isolate = nullptr;
        v8::ArrayBuffer::Allocator* arrayBufferAllocator = nullptr;
        v8::Persistent<v8::Context, v8::CopyablePersistentTraits<v8::Context>> v8GlobalContext{};

        // async message
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
     * @brief 
     * 
     * @param type 
     * @param args do not dispose this command!
     */
    void addUICommand(UICommandType type, CommandArgs&& args);

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

    void uiOpenProject(bool& folderError, std::string& lastOpenFolder, bool callLoadSuccess = true);

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

    void openGenerateResultWindow(std::string const& source, std::string const& text);

    void openEntityInfoWindow(std::string_view entityName);


    void inline showOrFocusWindow(bool& flag, std::string_view windowName){
        if (flag) {
            ImGui::SetWindowFocus(windowName.data());
        } else {
            flag = true;
        }
    }

}
