#include "sight_ui.h"
#include "dbg.h"
#include "sight_node_editor.h"
#include "sight.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_project.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <future>

#include <GLFW/glfw3.h>

#define COMMON_LANGUAGE_KEYS g_UIStatus->languageKeys->commonKeys
#define WINDOW_LANGUAGE_KEYS g_UIStatus->languageKeys->windowNames

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


static sight::UIStatus* g_UIStatus = nullptr;

namespace sight {

    namespace {
        // private members and functions.

        void showMainFileMenu(UIStatus& uiStatus){
            if (ImGui::BeginMenu("New")){
                if (ImGui::MenuItem("File")) {

                }
                if (ImGui::MenuItem("Graph")) {

                }
                ImGui::Separator();
                if (ImGui::MenuItem("Entity")) {
                    if (g_UIStatus->windowStatus.createEntity) {
                        ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.createEntity);
                    } else {
                        g_UIStatus->windowStatus.createEntity = true;
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Open", "Ctrl+O")) {
                dbg("open graph file");
                changeGraph("./simple1");
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")){
                getCurrentGraph()->save();
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Options")) {
                // show options window

            }

            if (ImGui::MenuItem("Exit")){
                uiStatus.closeWindow = true;
            }
        }

        void showMainEditMenu(){
            if (ImGui::MenuItem("Undo")){

            }

            if (ImGui::MenuItem("Redo")) {

            }
        }

        void showMainViewMenu(){
            if (ImGui::MenuItem("View")) {

            }
            if (ImGui::BeginMenu("Graph")) {
                if (ImGui::MenuItem("Entity")) {
                    // show entity graph
                    changeGraph("./entity", true);
                }
                ImGui::EndMenu();
            }
        }

        void showMainBuildMenu(){
            if (ImGui::MenuItem("Build")) {
                currentProject()->build();
            }
            if (ImGui::MenuItem("Rebuild")) {
                currentProject()->rebuild();
            }
            if (ImGui::MenuItem("Clean")) {
                currentProject()->clean();
            }
            if (ImGui::MenuItem("Parse Graph")) {
                addJsCommand(JsCommandType::ParseGraph, "./simple.yaml");
            }
        }

        void showMainCustomMenu(){
            if (ImGui::MenuItem("Trigger")) {
                // addJsCommand(JsCommandType::File, "/Volumes/mac_extend/Project/sight/scripts/nodes.js");
//                testParser();
                parseSource("a = 99 + 102;");
            }
            if (ImGui::MenuItem("Crash")) {
                // produce a crash for test.
                int *p = NULL;
                *p = 1;
            }
        }

        void showMainMenuBar(UIStatus& uiStatus){
            if (ImGui::BeginMainMenuBar()){
                if (ImGui::BeginMenu("File")) {
                    showMainFileMenu(uiStatus);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit")) {
                    showMainEditMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("View")) {
                    showMainViewMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Build")) {
                    showMainBuildMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Custom")) {
                    showMainCustomMenu();
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void showDemoWindow(){
            ImGui::Begin("Test Window");
            ImGui::Text("this is first line.");
            ImGui::End();
        }

        void showHierarchyWindow(bool needInit){
            if (needInit) {
                ImGui::SetNextWindowPos(ImVec2(0,20));
                ImGui::SetNextWindowSize(ImVec2(300, 285));
            }
            ImGui::Begin(WINDOW_LANGUAGE_KEYS.hierarchy);
            // it's should be what ?
            ImGui::End();
        }

        void showInspectorWindow(bool needInit){
            if (needInit) {
                ImGui::SetNextWindowPos(ImVec2(0,305));
                ImGui::SetNextWindowSize(ImVec2(300, 300));
            }

            ImGui::Begin(WINDOW_LANGUAGE_KEYS.inspector);
            // what's here?
            // it's should be show what user selected.
            ImGui::End();
        }

        void showProjectWindow(bool needInit){
            if (needInit)
            {
                ImGui::SetNextWindowPos(ImVec2(0,605));
                ImGui::SetNextWindowSize(ImVec2(300, 200));
            }
            
            ImGui::Begin(WINDOW_LANGUAGE_KEYS.project);

            // show project files.
            

            ImGui::End();
        }

        void showCreateEntityWindow(){
            int inputTextId = 0;

            auto & createEntityData = g_UIStatus->createEntityData;

            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.createEntity, &g_UIStatus->windowStatus.createEntity)) {
                //LeftLabeledInput(COMMON_LANGUAGE_KEYS.className, g_UIStatus->createEntityData.name, NAME_BUF_SIZE);
                ImGui::InputText(COMMON_LANGUAGE_KEYS.className, g_UIStatus->createEntityData.name, NAME_BUF_SIZE);
                ImGui::InputText(COMMON_LANGUAGE_KEYS.address, g_UIStatus->createEntityData.templateAddress, NAME_BUF_SIZE);
                // maybe need a help marker ?

                ImGui::Separator();
                // first line buttons
                if (ImGui::Button("+")) {
                    createEntityData.resetFieldsStatus(true, true);
                    createEntityData.addField();
                    createEntityData.lastField()->editing = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("-")) {
                    createEntityData.deleteSelected();
                    createEntityData.resetFieldsStatus(true, true);
                }
                ImGui::SameLine();
                if (ImGui::Button("^")) {
                    createEntityData.moveItemUp();
                }
                ImGui::SameLine();
                if (ImGui::Button("v")) {
                    createEntityData.moveItemDown();
                }
                ImGui::SameLine();
                if (ImGui::Button("c")) {
                    createEntityData.resetFieldsStatus(true, true);
                }


                // fields
                char buf[NAME_BUF_SIZE] = {0};
                if (ImGui::BeginTable("fields", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldType);
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldName);
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldDefaultValue);
                    ImGui::TableHeadersRow();

                    auto p = createEntityData.first;
                    while (p) {
                        // show
                        ImGui::TableNextRow();

                        if (p->editing) {
                            ImGui::TableSetColumnIndex(0);
                            sprintf(buf, "##.%d", inputTextId++);
                            ImGui::InputText(buf, p->type, NAME_BUF_SIZE);

                            ImGui::TableSetColumnIndex(1);
                            sprintf(buf, "##.%d", inputTextId++);
                            ImGui::InputText(buf, p->name, NAME_BUF_SIZE);

                            ImGui::TableSetColumnIndex(2);
                            sprintf(buf, "##.%d", inputTextId++);
                            ImGui::InputText(buf, p->defaultValue, NAME_BUF_SIZE);

                        } else {
                            ImGui::TableSetColumnIndex(0);
                            bool oldSelected = p->selected;
                            sprintf(buf, "##.%d", inputTextId++);
                            if (ImGui::Selectable(buf, &p->selected, ImGuiSelectableFlags_SpanAllColumns)) {
                                if (!ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKeyModFlags_Ctrl))) {
                                    g_UIStatus->createEntityData.resetFieldsStatus(true, true);
                                }
                                p->selected = true;
                                if (oldSelected) {
                                    p->editing = true;
                                }
                            }
                            ImGui::SameLine();
                            ImGui::Text("%s", p->type);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::Text("%s", p->name);

                            ImGui::TableSetColumnIndex(2);
                            ImGui::Text("%s", p->defaultValue);
                        }

                        p = p->next;
                    }

                    ImGui::EndTable();
                }

                ImGui::Dummy(ImVec2(0,3));
                ImGui::Separator();
                // preview
                ImGui::Text("%s", COMMON_LANGUAGE_KEYS.preview);
                ImGui::PushStyleColor(ImGuiCol_Text, g_UIStatus->uiColors->readonlyText);
                ImGui::InputTextMultiline("##preview", buf, NAME_BUF_SIZE, ImVec2(0,0), ImGuiInputTextFlags_ReadOnly);
                ImGui::PopStyleColor();

                // buttons
                if (ImGui::Button(COMMON_LANGUAGE_KEYS.ok)) {
                    // add entity
                    int r = addEntity(createEntityData);
                    if (r == 0) {
                        createEntityData.reset();
                        g_UIStatus->windowStatus.createEntity = false;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                    g_UIStatus->windowStatus.createEntity = false;
                }
            }

            ImGui::End();
        }
    }

    void mainWindowFrame(UIStatus & uiStatus) {
        showMainMenuBar(uiStatus);

        //showDemoWindow();
        // windows
        showHierarchyWindow(uiStatus.needInit);
        showInspectorWindow(uiStatus.needInit);
        showProjectWindow(uiStatus.needInit);
        showNodeEditorGraph(uiStatus);

        if (uiStatus.windowStatus.createEntity) {
            showCreateEntityWindow();
        }
        if (uiStatus.windowStatus.testWindow) {
            showDemoWindow();
        }

    }

    void runUICommand(UICommand *command){
        dbg(command->type);
        switch (command->type) {
            case UICommandType::UICommandHolder:
                break;
            case UICommandType::COMMON:
                break;
            case UICommandType::JsEndInit: {
                g_UIStatus->loadingStatus.jsThread = true;
                dbg("js end init.");
                break;
            }
            case UICommandType::AddNode:
            {
                // command->args.needFree = false;
                auto* nodePointer = (SightNode**) command->args.data;
                auto size = command->args.dataLength;
                for (int i = 0; i < size; ++i) {
                    addNode(nodePointer[i]);
                }
                break;
            }
            case UICommandType::AddTemplateNode:{
                auto *pointer = (SightNodeTemplateAddress**) command->args.data;
                auto size = command->args.dataLength;
                for (int i = 0; i < size; ++i) {
                    auto p = pointer[i];
                    addTemplateNode(*p);
                    delete p;
                }
                break;
            } 

        }

        command->args.dispose();
    }

    void runUICommandCallback(uv_async_t *handle){
        auto *command = (UICommand *)handle->data;
        runUICommand(command);
        free(command);
    }

    int initOpenGL() {

        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return CODE_FAIL;

        // GL 3.2 + GLSL 150
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#endif

        return CODE_OK;
    }

    int showLoadingWindow(){
        dbg("start loading");
        // Create window with graphics context
        const int width = 640;
        const int height = 360;

        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWwindow* window = glfwCreateWindow(width, height, "sight - loading", nullptr, NULL);
        if (window == nullptr)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 150";
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Our state
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // init ...
        g_UIStatus = new UIStatus({
                                          true,
                                          &io,
                                          false,
                                  });
        UIStatus & uiStatus = *g_UIStatus;
        uiStatus.uvAsync = new uv_async_t();

        // uv loop init
        auto uvLoop = (uv_loop_t*)malloc(sizeof(uv_loop_t));
        uiStatus.uvLoop = uvLoop;
        uv_loop_init(uvLoop);
        uv_async_init(uvLoop, uiStatus.uvAsync, runUICommandCallback);

        g_UIStatus->languageKeys = loadLanguage("");
        g_UIStatus->uiColors = new UIColors();

        // load plugins
        addJsCommand(JsCommandType::InitParser);
        addJsCommand(JsCommandType::InitPluginManager);
        addJsCommand(JsCommandType::EndInit);


        // init node editor.
        initNodeEditor();

        uint progress = 0;
        
        // Main loop
        while (!glfwWindowShouldClose(window))
        {
            if (g_UIStatus->isLoadingOver()) {
                break;
            }

            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();


            ImGui::SetNextWindowPos(ImVec2(0,0));
            ImGui::SetNextWindowSize(ImVec2(width, height));
            ImGui::Begin("loading", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::Text("sight is loading...");
            
            ImGui::Text("past frame: %d", ++progress);
            ImGui::End();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);

            uv_run(uvLoop, UV_RUN_NOWAIT);
        }

        // load template nodes.
        for (int i = 0; i < 30; ++i) {
            uv_run(uvLoop, UV_RUN_NOWAIT);
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);

        dbg("end loading");
        return 0;
    }

    int showMainWindow(){
        if (!g_UIStatus) {
            return CODE_FAIL;
        }

        // Create window with graphics context
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        GLFWwindow* window = glfwCreateWindow(1600, 900, "sight - a low-code tool", nullptr, NULL);
        if (window == nullptr)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        g_UIStatus->io = &io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 150";
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Our state
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        

        changeGraph("./simple");

        auto uvLoop = g_UIStatus->uvLoop;
        // Main loop
        while (!glfwWindowShouldClose(window) && !g_UIStatus->closeWindow)
        {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            mainWindowFrame(*g_UIStatus);


            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);

            g_UIStatus->needInit = false;
            // printf("before uv run\n");
            uv_run(uvLoop, UV_RUN_NOWAIT);
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);

        return 0;
    }

    void destroyWindows() {
        destroyNodeEditor();
        glfwTerminate();

        exitSight();

        delete g_UIStatus;
        g_UIStatus = nullptr;
    }

    void addUICommand(UICommandType type, int argInt) {
        UICommand command = {
                type,
                {
                        .argInt = argInt
                }
        };

        addUICommand(command);
    }


    int addUICommand(UICommandType type, void *data, size_t dataLength, bool needFree) {
        UICommand command = {
                type,
                {
                        .needFree = needFree,
                        .data =  data,
                        .dataLength = dataLength,
                }
        };

        return addUICommand(command);
    }

    int addUICommand(UICommand &command) {
        auto async = g_UIStatus->uvAsync;
        async->data = copyObject(&command, sizeof(UICommand));
        uv_async_send(async);
        printf("addUICommand ...\n");
        return 0;
    }

    bool LeftLabeledInput(const char *label, char *buf, size_t bufSize) {
        ImGui::Text("%s", label);
        ImGui::SameLine();
        return ImGui::InputText("", buf, bufSize);
    }


    void UICreateEntity::addField() {
        auto p = (struct EntityField*) calloc(1, sizeof (struct EntityField));

        auto t = findAttachTo();
        if (!t) {
            first = p;
        } else {
            t->next = p;
            p->prev = t;
        }
    }

    EntityField *UICreateEntity::findAttachTo() {
        EntityField* c = first;
        while ( c ) {
            if (!c->next) {
                break;
            }

            c = c->next;
        }

        return c;
    }

    void UICreateEntity::resetFieldsStatus(bool editing, bool selected) {
        auto c = this->first;
        while (c) {
            if (editing) {
                c->editing = false;
            }
            if (selected) {
                c->selected = false;
            }

            c = c->next;
        }
    }

    EntityField *UICreateEntity::lastField() {
        auto c = first;
        while (c) {
            if (!c->next) {
                break;
            }
            c = c->next;
        }
        return c;
    }

    int UICreateEntity::deleteSelected() {
        auto c = first;
        auto count = 0;
        while (c) {
            if (c->selected) {
                // delete
                count++;
                auto t = c;
                if (c->prev) {
                    c->prev->next = c->next;
                    if (c->next) {
                        c->next->prev = c->prev;
                    }
                } else {
                    // no previous element, this is the first element.
                    first = c->next;
                    if (first) {
                        first->prev = nullptr;
                    }
                }

                free(t);
            }
            c = c->next;
        }

        return count;
    }

    void UICreateEntity::moveItemUp() {
        int statusCode = 0;
        auto p = findSelectedEntity(&statusCode, true);

        if (!p || !p->prev) {
            return;
        }

        // swap p and p->prev
        auto a = p->prev;
        if (a->prev) {
            a->prev->next = p;
            p->prev = a->prev;
            auto b = p->next;

            p->next = a;
            a->prev = p;

            a->next = b;
            if (b) {
                b->prev = a;
            }
        } else {
            // a is the first element.
            first = p;
            p->prev = nullptr;
            auto b = p->next;

            a->prev = p;
            p->next = a;

            a->next = b;
            if (b) {
                b->prev = a;
            }
        }
    }

    void UICreateEntity::moveItemDown() {
        int statusCode = 0;
        auto p = findSelectedEntity(&statusCode, true);

        if (!p || !p->next) {
            return;
        }

        // swap p and p->next
        auto a = p->next;
        if (a->next) {
            a->next->prev = p;
            p->next = a->next;
            auto b = p->prev;

            p->prev = a;
            a->next = p;

            a->prev = b;
            if (b) {
                b->next = a;
            } else {
                first = a;
            }

        } else {
            // a is the last element.
            p->next = nullptr;
            auto b = p->prev;

            p->prev = a;
            a->next = p;

            a->prev = b;
            if (b) {
                b->next = a;
            } else {
                first = a;
            }
        }
    }

    EntityField *UICreateEntity::findSelectedEntity(int *statusCode, bool resetOthers) {
        auto c = first;

        EntityField *r = nullptr;
        while (c) {
            if (c->selected) {
                if (r) {
                    *statusCode = -1;
                    if (resetOthers) {
                        c->selected = false;
                    }
                } else {
                    r = c;
                }
            }

            c = c->next;
        }

        return r;
    }

    void UICreateEntity::loadFrom(struct SightNode *node) {
        if (!node) {
            return;
        }

        sprintf(this->name, "%s", node->nodeName.c_str());

        // For entity node, inputs is same as outputs, so just foreach one is fine.
        for (const auto &item : node->inputPorts) {
            addField();

            auto p = lastField();
            sprintf(p->name, "%s", item.portName.c_str());
            sprintf(p->type, "%s", getTypeName(item.type).c_str());
            sprintf(p->defaultValue, "%s", item.getDefaultValue());
        }

    }

    void UICreateEntity::reset() {
        sprintf(this->name, "");
        sprintf(this->templateAddress, "");

        auto c = first;
        while (c) {
            auto t = c;
            c = c->next;

            free(t);
        }

        first = nullptr;
    }


    UIColors::UIColors() {
        grey = IM_COL32(128,128,128,255);

        readonlyText = grey;
    }

    UIStatus::~UIStatus() {
        if (uvLoop) {
            uv_loop_close(uvLoop);
            free(uvLoop);
            uvLoop = nullptr;
        }

        if (uvAsync) {
            delete uvAsync;
            uvAsync = nullptr;
        }

        if (uiColors) {
            delete uiColors;
            uiColors = nullptr;
        }

        if (languageKeys) {
            delete languageKeys;
            languageKeys = nullptr;
        }

    }

    bool UIStatus::isLoadingOver() const{
        return this->loadingStatus.isLoadingOver();
    }

    bool LoadingStatus::isLoadingOver() const {
        return jsThread;
    }

}
