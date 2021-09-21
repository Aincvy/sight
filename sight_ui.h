// Main window ui
//
#pragma once
#include "imgui.h"
#include "uv.h"

#include "sight.h"
#include "sight_language.h"

namespace sight {

    struct SightNode;

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

        ImU32 grey;

        ImU32 readonlyText ;

        UIColors();
    };

    struct LoadingStatus {
        bool jsThread = false;

        bool isLoadingOver() const;
    };

    struct UIStatus {
        bool needInit = false;
        ImGuiIO* io;
        bool closeWindow = false;
        struct UIWindowStatus windowStatus;
        struct UICreateEntity createEntityData;
        struct LoadingStatus loadingStatus;

        struct LanguageKeys* languageKeys = nullptr;
        struct UIColors* uiColors = nullptr;

        uv_loop_t *uvLoop = nullptr;
        uv_async_t* uvAsync = nullptr;

        ~UIStatus();

        bool isLoadingOver() const;
    };

    /**
     *
     */
    class Selection{
    public:

    private:

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

}
