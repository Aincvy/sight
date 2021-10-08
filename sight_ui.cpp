#include "dbg.h"
#include "imgui_node_editor.h"
#include "sight_defines.h"
#include "sight_keybindings.h"
#include "sight_ui.h"
#include "sight_node_editor.h"
#include "sight.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_project.h"
#include "sight_util.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sight_widgets.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <stdio.h>
#include <future>

#include <filesystem>

#include <GLFW/glfw3.h>
#include <string>
#include <sys/types.h>
#include <vector>

#define COMMON_LANGUAGE_KEYS g_UIStatus->languageKeys->commonKeys
#define WINDOW_LANGUAGE_KEYS g_UIStatus->languageKeys->windowNames
#define MENU_LANGUAGE_KEYS g_UIStatus->languageKeys->menuKeys

// todo change this
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


static sight::UIStatus* g_UIStatus = nullptr;

namespace sight {

    using directory_iterator = std::filesystem::directory_iterator;

    namespace {
        // private members and functions.

        /**
         * @brief save whatever file/graph, if it was opened and edited.
         * 
         */
        void saveAnyThing(){
            getCurrentGraph()->save();
        }

        void showMainFileMenu(UIStatus& uiStatus){
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS._new)) {
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.entity)) {
                }
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.graph)) {
                    g_UIStatus->windowStatus.popupGraphName = true;
                }

                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.graphByPath)) {
                    std::string path = saveFileDialog(currentProject()->pathGraphFolder().c_str());
                    if (!path.empty()) {
                        // create
                        currentProject()->createGraph(path.c_str(), false);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.entity)) {
                    if (g_UIStatus->windowStatus.createEntity) {
                        ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.createEntity);
                    } else {
                        g_UIStatus->windowStatus.createEntity = true;
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.open)) {
                dbg("open graph file");
                
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.save, g_UIStatus->keybindings->saveFile.tipsText())) {
                dbg("save graph");
                saveAnyThing();
            }

            ImGui::Separator();
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.options)) {
                // show options window
            }

            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.exit)) {
                uiStatus.closeWindow = true;
            }
        }

        void showMainEditMenu(){
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.undo)) {
            }

            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.redo)) {
            }
        }

        void showMainViewMenu(){
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.view)) {
            }
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.graph)) {
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.entity)) {
                    // show entity graph
                    changeGraph("./entity", true);
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.layout)) {
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.reset)) {
                    g_UIStatus->windowStatus.layoutReset = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.windows)) {
                if (ImGui::MenuItem("Demo")) {
                    g_UIStatus->windowStatus.testWindow = true;
                }

                ImGui::EndMenu();
            }
        }

        void showMainProjectMenu(){
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.build)) {
                currentProject()->build();
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.rebuild)) {
                currentProject()->rebuild();
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.clean)) {
                currentProject()->clean();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.parseGraph)) {
                addJsCommand(JsCommandType::ParseGraph, "./simple.yaml");
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.projectSaveConfig)) {
                currentProject()->saveConfigFile();
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.reload, "Ctrl+R")) {
                currentProject()->buildFilesCache();
                dbg("reload");
            }
        }

        auto abc() {
            struct {
                int a;
                float b;
            } s {101,201};
            return s;
        }

        void t() {
            auto [a, b] = abc();
            dbg(a, b);
        }

        void showMainCustomMenu(){
            if (ImGui::MenuItem("Trigger")) {
                // auto p = findTemplateNode("test/http/HttpGetReqNode");
                // dbg(p);
                // if (p) {
                //     dbg(serializeJsNode(*p));
                // }
                t();
            }
            if (ImGui::MenuItem("Crash")) {
                // produce a crash for test.
                int *p = NULL;
                *p = 1;
            }
        }

        void showHelpMenu(){
            if (ImGui::MenuItem("About")) {
                dbg("sight WIP v0.1");
            }
            if (ImGui::MenuItem("Github")) {
                openUrlWithDefaultBrowser("https://github.com/Aincvy/sight");
            }
        }

        void showMainMenuBar(UIStatus& uiStatus){
            if (ImGui::BeginMainMenuBar()){
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.file)) {
                    showMainFileMenu(uiStatus);
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.edit)) {
                    showMainEditMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.view)) {
                    showMainViewMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.project)) {
                    showMainProjectMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.custom)) {
                    showMainCustomMenu();
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.help)) {
                    showHelpMenu();
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }
        }

        void showDemoWindow(bool needInit){
            ImGui::Begin("Test Window", &g_UIStatus->windowStatus.testWindow);
            ImGui::Text("this is first line.");
            ImGui::Text(u8"中文");

            static const char* a[] = {
                "A",
                "B",
                "C",
                "D"
            };
            static int index = 0;
            if (ImGui::BeginCombo("Combo Test", a[index])) {
                for( int i = 0; i < std::size(a); i++){
                    if (ImGui::Selectable(a[i], i == index)) {
                        index = i;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Text(u8"在吗，这里是下一行");

            ImGui::End();
        }

        void showHierarchyWindow(){
            if (g_UIStatus->windowStatus.layoutReset) {
                ImGui::SetNextWindowPos(ImVec2(0,20));
                ImGui::SetNextWindowSize(ImVec2(300, 285));
            }

            auto & selection = g_UIStatus->selection;

            ImGui::Begin(WINDOW_LANGUAGE_KEYS.hierarchy);
            // check and update node editor status
            if (!isNodeEditorReady()) {
                ImGui::End();
                return;
            }

            int selectedCount = ed::GetSelectedObjectCount();
            if (selectedCount > 0) {
                selection.selectedItems.clear();

                std::vector<ed::NodeId> selectedNodes;
                std::vector<ed::LinkId> selectedLinks;
                selectedNodes.resize(selectedCount);
                selectedLinks.resize(selectedCount);

                int c = ed::GetSelectedNodes(selectedNodes.data(), selectedCount);
                int d = ed::GetSelectedLinks(selectedLinks.data(), selectedCount);

                selectedNodes.resize(c);
                selectedLinks.resize(d);

                if (c == 1) {
                    selection.node = getCurrentGraph()->findNode(static_cast<uint>(selectedNodes[0].Get()));
                    if (selection.node) {
                        sprintf(g_UIStatus->buffer.inspectorNodeName, "%s", selection.node->nodeName.c_str());
                    }
                }

                for (const auto & item : selectedNodes) {
                    selection.selectedItems.insert(static_cast<uint>(item.Get()));    
                }
                
            } else {
                selection.resetSelectedNodes();
            }
            
            auto graph = getCurrentGraph();
            if (graph) {
                for (const auto & node : graph->getNodes()) {
                    ImGui::TextColored(g_UIStatus->uiColors->nodeIdText, "%d ", node.nodeId);
                    ImGui::SameLine();
                
                    auto nodeId = node.nodeId;
                    auto findResult = std::find(selection.selectedItems.begin(), selection.selectedItems.end(), nodeId);
                    bool isSelected = findResult != selection.selectedItems.end();
                    if (Selectable(static_cast<int>(nodeId), node.nodeName.c_str(), isSelected)) {
                        auto pointer = graph->findNode(nodeId);
                        if (!pointer) {
                            dbg("error" , nodeId);
                        } else {
                            bool anySelect = selection.node != nullptr;
                            bool appendNode = false;
                            if (anySelect) {
                                if (!g_UIStatus->io->KeyCtrl) {
                                    // not control
                                    selection.selectedItems.clear();
                                    ed::ClearSelection();
                                }  else {
                                    appendNode = true;
                                }
                            }
                            
                            selection.node = pointer;
                            selection.selectedItems.insert(nodeId);
                            sprintf(g_UIStatus->buffer.inspectorNodeName, "%s", node.nodeName.c_str());

                            ed::SelectNode(nodeId, appendNode);
                            ed::NavigateToSelection();
                        }
                    }
                }
            }

            ImGui::End();
        }

        void showInspectorWindow(){
            if (g_UIStatus->windowStatus.layoutReset) {
                ImGui::SetNextWindowPos(ImVec2(0,305));
                ImGui::SetNextWindowSize(ImVec2(300, 300));
            }

            ImGui::Begin(WINDOW_LANGUAGE_KEYS.inspector);
            // what's here?
            // it's should be show what user selected.
            auto & selection = g_UIStatus->selection;
            auto node = selection.node;
            if (selection.selectedItems.size() > 1) {
                ImGui::Text("%zu nodes is selected.", selection.selectedItems.size());
            } else {
                if (node) {
                    // show node info 
                    ImGui::Text("node id: ");
                    ImGui::SameLine();
                    ImGui::TextColored(g_UIStatus->uiColors->nodeIdText, "%d ", node->nodeId);
                    ImGui::Text("name: " );
                    ImGui::SameLine();
                    auto & nameBuf = g_UIStatus->buffer.inspectorNodeName;
                    if (ImGui::InputText("## Inspector.node.name", nameBuf, std::size(nameBuf))) {
                        dbg("InputText");
                        // node->nodeName = nameBuf;
                    }
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        dbg("1");
                        node->nodeName = nameBuf;
                    }

                    // fields
                    ImGui::Text("Ports");
                    ImGui::Separator();
                    showNodePorts(node);
                }
            }
            

            ImGui::End();
        }

        void showProjectFolder(ProjectFile const& folder){
            auto selection = &g_UIStatus->selection;

            for (const auto& item : folder.files) {
                if (item.fileType == ProjectFileType::Directory) {
                    if (ImGui::TreeNode(item.path.c_str(), "<dir> %s", item.filename.c_str())) {
                        showProjectFolder(item);
                        ImGui::TreePop();
                    }
                    if (ImGui::IsItemClicked()) {
                        selection->selectedFiles.clear();
                    }
                } else {

                    bool selected = selection->selectedFiles.contains(item.path);
                    std::string name;
                    switch (item.fileType) {
                        case ProjectFileType::Graph:
                            name = "<graph> ";
                            name += item.filename;
                            name += "##";
                            name += item.path;
                        break;
                        case ProjectFileType::Regular:
                        case ProjectFileType::Plugin:
                        case ProjectFileType::Directory:
                            name = strJoin(item.filename, item.path);
                            break;
                    }
                    if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                        dbg(item.path);

                        if (ImGui::IsMouseDoubleClicked(0)) {
                            auto g = getCurrentGraph();
                            if (g && item.path == g->getFilePath()) {
                                // same file, do nothing.
                                return;
                            }

                            selection->selectedFiles.clear();
                            // dbg("double click, open file", item.path);
                            if (item.fileType == ProjectFileType::Graph) {
                                // currentProject()->openGraph(item.path.c_str(), false);
                                uiChangeGraph(item.path.c_str());
                            } else {
                                dbg("unHandle file type", item.fileType);
                            }
                        } else {
                            
                            if (!g_UIStatus->io->KeyCtrl) {
                                selection->selectedFiles.clear();
                            }

                            if (!selected || !g_UIStatus->io->KeyCtrl) {
                                selection->selectedFiles.insert(item.path);
                            }
                        }
                    }
                }
            }
        }

        void showProjectWindow(){
            if (g_UIStatus->windowStatus.layoutReset)
            {
                ImGui::SetNextWindowPos(ImVec2(0,605));
                ImGui::SetNextWindowSize(ImVec2(300, 200));
            }
            
            ImGui::Begin(WINDOW_LANGUAGE_KEYS.project);
            showProjectFolder(currentProject()->getFileCache());

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

    void showModals(){
        // modals
        auto& windowStatus = g_UIStatus->windowStatus;
        if (windowStatus.popupGraphName) {
            windowStatus.popupGraphName = false;
            ImGui::OpenPopup("graphName");

            // Always center this window when appearing
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }

        if (ImGui::BeginPopupModal("graphName", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Please input a graph name");
            ImGui::Separator();

            ImGui::InputText("name", g_UIStatus->buffer.littleName, std::size(g_UIStatus->buffer.littleName));
            if (buttonOK(COMMON_LANGUAGE_KEYS.ok)) {
                // create

                if (strlen(g_UIStatus->buffer.littleName) > 0) {
                    currentProject()->createGraph(g_UIStatus->buffer.littleName);
                    *g_UIStatus->buffer.littleName = '\0';
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        auto& modalAskData = g_UIStatus->modalAskData;
        if (windowStatus.popupAskModal) {
            windowStatus.popupAskModal = false;

            if (!endsWith(modalAskData.title, MyUILabels::modalAskData)) {
                modalAskData.title += MyUILabels::modalAskData;
            }
            ImGui::OpenPopup(MyUILabels::modalAskData);

            // Always center this window when appearing
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }
        if (ImGui::BeginPopupModal(modalAskData.title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", modalAskData.content.c_str());
            ImGui::Dummy(ImVec2(0, 18));

            if (buttonOK(COMMON_LANGUAGE_KEYS.ok)) {
                modalAskData.callback(true);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                modalAskData.callback(false);
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        auto& modalSaveData = g_UIStatus->modalSaveData;
        if (windowStatus.popupSaveModal) {
            windowStatus.popupSaveModal = false;

            if (!endsWith(modalSaveData.title, MyUILabels::modalSaveData)) {
                modalSaveData.title += MyUILabels::modalSaveData;
            }
            ImGui::OpenPopup(MyUILabels::modalSaveData);

            // Always center this window when appearing
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }

        if (ImGui::BeginPopupModal(modalSaveData.title.c_str())) {
            ImGui::Text("%s", modalSaveData.content.c_str());
            ImGui::Dummy(ImVec2(0,18));

            if (buttonOK(MENU_LANGUAGE_KEYS.save)) {
                modalSaveData.callback(SaveOperationResult::Save);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                modalSaveData.callback(SaveOperationResult::Cancel);
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(COMMON_LANGUAGE_KEYS.drop)) {
                modalSaveData.callback(SaveOperationResult::Drop);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    /**
     * @brief Handle some common keyboard input
     * 
     */
    void handleKeyboardInput(){
        if (!g_UIStatus->keybindings->controlKey.isKeyDown()) {
            return;
        }

        // dbg("got control key");
        if (g_UIStatus->keybindings->saveFile) {
            dbg("save graph");
            saveAnyThing();
        }
    }

    void mainWindowFrame(UIStatus & uiStatus) {
        showModals();
        handleKeyboardInput();
        showMainMenuBar(uiStatus);
        showProjectWindow();

        // HierarchyWindow and InspectorWindow will use node editor data.
        nodeEditorFrameBegin();

        // windows
        showHierarchyWindow();
        showInspectorWindow();
        showNodeEditorGraph(uiStatus);

        if (uiStatus.windowStatus.createEntity) {
            showCreateEntityWindow();
        }
        if (uiStatus.windowStatus.testWindow) {
            showDemoWindow(false);
        }

        nodeEditorFrameEnd();

        if (uiStatus.windowStatus.layoutReset) {
            uiStatus.windowStatus.layoutReset = false;
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

        // init node editor.
        initNodeEditor();

        // init keys
        initKeys();
        g_UIStatus->keybindings = loadKeyBindings("./keybindings.yaml");

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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontDefault();
        ImFontConfig config;
        config.MergeMode = true;
        auto fontPointer = io.Fonts->AddFontFromFileTTF((resourceFolder + "font/FangZhengKaiTiJianTi-1.ttf").c_str(), 16, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        io.Fonts->Build();

        g_UIStatus->io = &io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        const char* glsl_version = "#version 150";
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Our state
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

        // project path 
        auto project = currentProject();
        if (project) {
            onProjectAndUILoadSuccess(project);
        }

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
                .data = data,
                .dataLength = dataLength,
            }
        };

        return addUICommand(command);
    }

    int addUICommand(UICommand &command) {
        auto async = g_UIStatus->uvAsync;
        async->data = copyObject(&command, sizeof(UICommand));
        uv_async_send(async);
        dbg(command.type);
        return 0;
    }

    bool LeftLabeledInput(const char *label, char *buf, size_t bufSize) {
        ImGui::Text("%s", label);
        ImGui::SameLine();
        return ImGui::InputText("", buf, bufSize);
    }

    UIStatus* currentUIStatus() {
        return g_UIStatus;
    }

    void showNodePorts(SightNode* node, bool showField, bool showValue, bool showOutput, bool showInput) {
        if (!showField && !showValue && !showOutput && !showInput) {
            return;
        }

        auto nodeWork = [](std::vector<SightNodePort> & list, bool showValue, bool alwaysShow) {
            for( auto& item: list){
                if (alwaysShow || (showValue && item.options.showValue)) {
                    ImGui::Text("%s: ", item.portName.c_str());
                    ImGui::SameLine();
                    showNodePortValue(&item);
                }
            }
        };

        nodeWork(node->fields, showValue, showField);
        nodeWork(node->outputPorts, showValue, showOutput);
        nodeWork(node->inputPorts, showValue, showInput);

    }

    void uiChangeGraph(const char* path ) {
        auto g = getCurrentGraph(); 
        if (!g) {
            return;
        }

        if (!g->editing) {
            // 
            currentProject()->openGraph(path, false);
            return;
        }

        std::string stringPath(path);
        openSaveModal("Save File?", "Current Graph do not save, do you want save file?", [stringPath](SaveOperationResult r) {
            if (r == SaveOperationResult::Drop) {
                currentProject()->openGraph(stringPath.c_str(), false);
            } else if (r == SaveOperationResult::Save) {
                getCurrentGraph()->save();
                currentProject()->openGraph(stringPath.c_str(), false);
            }
        });
    }

    void openSaveModal(const char* title, const char* content, std::function<void(SaveOperationResult)> const& callback) {
        auto& askData = g_UIStatus->modalSaveData;
        askData.title = title;
        askData.content = content;
        askData.callback = callback;

        g_UIStatus->windowStatus.popupSaveModal = true;
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

    std::string Selection::getProjectPath() const {
        return this->projectPath;
    }

    void Selection::resetSelectedNodes() {
        this->node = nullptr;
        this->selectedItems.clear();

    }

}
