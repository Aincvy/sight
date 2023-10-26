
#include "sight.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "imgui_node_editor.h"

#include "sight_terminal.h"
#include "sight_defines.h"
#include "sight_external_widgets.h"
#include "sight_keybindings.h"
#include "sight_plugin.h"
#include "sight_ui.h"
#include "sight_nodes.h"
#include "sight_ui_node_editor.h"
#include "sight_ui_project.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_project.h"
#include "sight_util.h"
#include "sight_undo.h"
#include "sight_colors.h"
#include "sight_log.h"
#include "sight_widgets.h"
#include "sight_render.h"

#include "v8pp/convert.hpp"

#include "IconsMaterialDesign.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <stdio.h>
#include <future>
#include <atomic>
#include <thread>

#include <filesystem>
#include <functional>

#include <string>
#include <string_view>
#include <sys/types.h>
#include <uv.h>
#include <vector>

#include <absl/strings/match.h>

#include "crude_json.h"

#ifdef NOT_WIN32
#    include <sys/termios.h>
#endif


#    define COMMON_LANGUAGE_KEYS g_UIStatus->languageKeys->commonKeys
#    define WINDOW_LANGUAGE_KEYS g_UIStatus->languageKeys->windowNames
#    define MENU_LANGUAGE_KEYS g_UIStatus->languageKeys->menuKeys


namespace ed = ax::NodeEditor;

static sight::UIStatus* g_UIStatus = nullptr;
static std::atomic<bool> uiCommandFree = true;

namespace sight {

    using directory_iterator = std::filesystem::directory_iterator;

    namespace {
        // private members and functions.

        /**
         * @brief save whatever file/graph, if it was opened and edited.
         * 
         */
        void saveAnyThing(){
            int i ;
            if ((i = currentGraph()->save()) != CODE_OK) {
                logDebug(i);
            }
            if ((i = currentProject()->save()) != CODE_OK) {
                logDebug(i);
            }
            saveSightSettings();
        }

        void activeWindowCreateEntity(){
            if (g_UIStatus->windowStatus.createEntity) {
                ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.createEntity);
            } else {
                g_UIStatus->windowStatus.createEntity = true;
            }
        }

        void showMainFileMenu(UIStatus& uiStatus){
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS._new)) {
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.graph)) {
                    g_UIStatus->windowStatus.popupGraphName = true;
                }

                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.graphByPath)) {
                    std::string path = saveFileDialog(currentProject()->pathGraphFolder().c_str());
                    if (!path.empty()) {
                        // create
                        currentProject()->createGraph(path.c_str());
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.entity)) {
                    activeWindowCreateEntity();
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.openProject)) {
                bool folderError;
                std::string lastOpenFolder;
                uiOpenProject(folderError, lastOpenFolder);
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.save)) {
                currentGraph()->save();
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.saveAll, g_UIStatus->keybindings->saveFile.tipsText())) {
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
            auto bindings = g_UIStatus->keybindings;
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.undo, bindings->undo.tipsText(), false, isUndoEnable())) {
                undo();
            }

            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.redo, bindings->redo.tipsText(), false, isRedoEnable())) {
                redo();
            }
        }

        void showMainViewMenu(){
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.view)) {
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
                if (ImGui::MenuItem("Terminal")) {
                    g_UIStatus->windowStatus.terminalWindow = true;
                }
                if (ImGui::MenuItem("CodeSetSettings")) {
                    g_UIStatus->windowStatus.codeSetSettingsWindow = true;
                }

                ImGui::EndMenu();
            }
        }

        void showMainProjectMenu(){
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.build)) {
                // currentProject()->build();
                auto p = currentProject();
                for (const auto& item : p->getBuildTargetMap()) {
                    if (ImGui::MenuItem(item.first.c_str())) {
                        logDebug(item.first);
                        addJsCommand(JsCommandType::ProjectBuild, item.first.c_str());
                    }
                }

                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("codeSetBuild")) {
                addJsCommand(JsCommandType::ProjectCodeSetBuild);
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.rebuild)) {
                // currentProject()->rebuild();
                addJsCommand(JsCommandType::ProjectRebuild);
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.clean)) {
                // currentProject()->clean();
                addJsCommand(JsCommandType::ProjectClean);
            }
            ImGui::Separator();
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.parseGraph)) {
                auto lastOpenGraph = currentProject()->getLastOpenGraph();
                if (lastOpenGraph.empty()) {
                    logDebug("not open any graph");
                } else {
                    saveAnyThing();
                    addJsCommand(JsCommandType::ParseGraph, strdup((lastOpenGraph + ".yaml").c_str()), 0, true);
                }
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.projectSaveConfig)) {
                currentProject()->saveConfigFile();
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.reload, "Ctrl+R")) {
                currentProject()->buildFilesCache();
                logDebug("reload");
            }
            if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.settings)) {
                if (g_UIStatus->windowStatus.projectSettingsWindow) {
                    ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.projectSettings);
                } else {
                    g_UIStatus->windowStatus.projectSettingsWindow = true;
                }
            }
        }

        void showEntityMenu(){
            if (ImGui::MenuItem("Create")) {
                activeWindowCreateEntity();
            }
            if (ImGui::MenuItem("List")) {
                g_UIStatus->windowStatus.entityListWindow = true;
            }
        }

        void showObjectMenu() {
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.entity)) {
                showEntityMenu();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.graph)) {
                auto graph = currentGraph();
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.parseGraph)) {
                    logError("parseGraph Not impl!");
                } 
                if (ImGui::MenuItem("VerifyId")) {
                    int c = currentGraph()->verifyId();
                    if (c == CODE_OK) {
                        g_UIStatus->toastController.toast(ICON_MD_SENTIMENT_SATISFIED " Id Fine!", "");
                    } else {
                        g_UIStatus->toastController.toast(ICON_MD_ERROR " Id has Error!", "");
                    }
                }
                if (ImGui::MenuItem("CheckAndFixId")) {
                    auto graph = currentGraph();
                    graph->checkAndFixIdError();
                    graph->markBroken(false);
                    g_UIStatus->toastController.toast(ICON_MD_INFO " Fix Over!", "");
                }
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.reload)) {
                    uiReloadGraph();
                }

                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.saveAsJson, graph)) {
                    if (ImGui::MenuItem("...")) {
                        uiGraphToJson(graph, "", true);
                    }

                    ImGui::Separator();     // 添加一个分割线

                    // 循环并添加 graph->getSaveAsJsonHistory() 中的项目
                    const std::vector<std::string>& history = graph->getSaveAsJsonHistory();
                    for (const std::string& item : history) {
                        if (ImGui::MenuItem(item.c_str())) {
                            // addJsCommand(JsCommandType::GraphToJsonData, CommandArgs::copyFrom(item.c_str()));
                            uiGraphToJson(graph, item.c_str());
                        }
                    }

                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
        }

        void showMainCustomMenu() {
            if (ImGui::MenuItem("Trigger")) {
                trace("trace msg");
                logDebug("123");
                logInfo("info msg");
                logWarning("warning msg");
                logError(5.555f);
                                     
                std::string source = R"(
let a = [1,2,3];
for(const item of a) {
  console.log(item);
}
                )";
                parseSource(source);
            }
            if (ImGui::MenuItem("Crash")) {
                // produce a crash for test.
                int* p = NULL;
                *p = 1;
            }
            if (ImGui::MenuItem("Test1")) {
                auto g = currentGraph();

                crude_json::value a;
                a["hello"] = 1.0;
                a["world"] = 2.0;

                crude_json::value root;
                root["a"] = a;
                root["version"] = "1.1";
                auto jsonStr = root.dump(2);
                logDebug("json: $0", jsonStr);
            }
        }

        void showHelpMenu(){
            if (ImGui::MenuItem("About")) {
                logDebug("sight WIP v0.1");
                if (g_UIStatus->windowStatus.aboutWindow) {
                    ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.about);
                } else {
                    g_UIStatus->windowStatus.aboutWindow = true;
                }
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
                if (ImGui::BeginMenu("Object")) {
                    showObjectMenu();
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
            // terminal test
            // static bool initTerminal = false;
            // TerminalRuntimeArgs customCommand;
            // static ImTerm::terminal<TerminalCommands>* terminal = nullptr;
            // if (!initTerminal) {
            //     initTerminal = true;
            //     terminal = new ImTerm::terminal<TerminalCommands>(customCommand);
            //     terminal->add_text("1");
            //     terminal->add_text("1");
            //     terminal->add_text("1");
            //     terminal->add_text("1");
            // } else {
            //     initTerminal = terminal->show();
            // }

            // a
            ImGui::Begin("Test Window", &g_UIStatus->windowStatus.testWindow);
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
            // ImGui::Text(u8"在吗，这里是下一行");

            ImGui::Button(ICON_MD_SEARCH " Search");
            ImGui::Text(ICON_MD_REPEAT " Repeat");
            ImGui::Button(ICON_MD_REPEAT_ONE " 重复");

            const ImU32 col = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
            const ImU32 bg = ImGui::GetColorU32(ImGuiCol_Button);

            ImGui::Spinner("##spinner", 15, 6, col);
            ImGui::BufferingBar("##buffer_bar", 0.7f, ImVec2(400, 6), bg, col);

            ImGui::LoadingIndicatorCircle("circle", 20.f, ImColor(col), ImColor(bg), 12, 6);

            static char c = '1';
            if (ImGui::Button("Toast")) {
                std::string title = "Toast";
                title += (c++);
                g_UIStatus->toastController.toast(title, "Content");
            }

            ImGui::End();

        }

        void updateSelectedNodeFromED(){
            auto& selection = g_UIStatus->selection;

            int selectedCount = ed::GetSelectedObjectCount();
            if (selectedCount > 0) {
                selection.selectedNodeOrLinks.clear();

                std::vector<ed::NodeId> selectedNodes;
                std::vector<ed::LinkId> selectedLinks;
                selectedNodes.resize(selectedCount);
                selectedLinks.resize(selectedCount);

                int c = ed::GetSelectedNodes(selectedNodes.data(), selectedCount);
                int d = ed::GetSelectedLinks(selectedLinks.data(), selectedCount);
                selectedNodes.resize(c);
                selectedLinks.resize(d);

                uint lastNodeId = 0;
                for (const auto& item : selectedNodes) {
                    lastNodeId = static_cast<uint>(item.Get());
                    selection.selectedNodeOrLinks.insert(lastNodeId);
                }
                for (const auto& item : selectedLinks) {
                    selection.selectedNodeOrLinks.insert(static_cast<uint>(item.Get()));
                }

                // update node name buffer.
                if (selectedNodes.size() == 1) {
                    auto n = currentGraph()->findNode(lastNodeId);
                    if (n) {
                        sprintf(g_UIStatus->buffer.inspectorNodeName, "%s", n->nodeName.c_str());
                    }
                }
            } else {
                selection.selectedNodeOrLinks.clear();
            }
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

            updateSelectedNodeFromED();
            
            auto graph = currentGraph();
            if (graph) {
                for (const auto & node : graph->getNodes()) {
                    ImGui::TextColored(g_UIStatus->uiColors->nodeIdText, "%d ", node.nodeId);
                    ImGui::SameLine();
                
                    auto nodeId = node.nodeId;
                    bool isSelected = selection.selectedNodeOrLinks.contains(nodeId);
                    if (Selectable(static_cast<int>(nodeId), node.nodeName.c_str(), isSelected)) {
                        auto pointer = graph->findNode(nodeId);
                        assert(pointer);

                        bool anySelect = !selection.selectedNodeOrLinks.empty();
                        bool appendNode = false;
                        if (anySelect) {
                            if (!g_UIStatus->keybindings->controlKey.isKeyDown()) {
                                // not control
                                selection.selectedNodeOrLinks.clear();
                                ed::ClearSelection();
                            } else {
                                appendNode = true;
                            }
                        }
                        if (isSelected && appendNode) {
                            // deselect 
                            selection.selectedNodeOrLinks.erase(nodeId);
                            ed::DeselectNode(nodeId);
                        } else {
                            selection.selectedNodeOrLinks.insert(nodeId);
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

            if (!ImGui::Begin(WINDOW_LANGUAGE_KEYS.inspector)) {
                ImGui::End();
                return;
            }

            auto graph = currentGraph();
            auto & selection = g_UIStatus->selection;
            if(graph == nullptr) {
                // 
            } else if (selection.selectedNodeOrLinks.empty()) {
                // show settings
                showGraphSettings();
            } else if (selection.selectedNodeOrLinks.size() > 1) {
                ImGui::Text("%zu nodes is selected.", selection.selectedNodeOrLinks.size());
                ImGui::TextColored(colorRed, "Now, we do not support mutiple items edit.");
            } else {
                auto node = selection.getSelectedNode();
                SightNodeConnection* connection = nullptr;
                if (node) {
                    // show node info
                    ImGui::Text("node id: ");
                    ImGui::SameLine();
                    ImGui::TextColored(g_UIStatus->uiColors->nodeIdText, "%d ", node->nodeId);
                    ImGui::Text("name: " );
                    ImGui::SameLine();
                    auto & nameBuf = g_UIStatus->buffer.inspectorNodeName;
                    ImGui::InputText("## node.name", nameBuf, std::size(nameBuf));
                    if (ImGui::IsItemDeactivatedAfterEdit()) {
                        node->nodeName = nameBuf;
                        node->graph->markDirty();
                    }
            
                    // fields
                    ImGui::Text("Ports");
                    ImGui::Separator();
                    showNodePorts(node);
                    showNodeComponents(node->componentContainer, node, node->graph, node->getNodeId(), true);
                    showComponentContextMenu();
                }  
                if ((connection = selection.getSelectedConnection())) {
                    if (node) {
                        ImGui::Separator();
                        ImGui::Text("Connection Info");
                    }
                    ImGui::Text("connection id: ");
                    ImGui::SameLine();
                    ImGui::TextColored(g_UIStatus->uiColors->nodeIdText, "%d ", connection->connectionId);

                    leftText("priority: ");
                    if (ImGui::InputInt("## connection.priority", &connection->priority)) {
                        graph->markDirty();
                        connection->findLeftPort()->sortConnections();
                        connection->findRightPort()->sortConnections();
                    }
                    leftText("generateCode: ");
                    if (ImGui::Checkbox("##generateCode", &connection->generateCode)) {
                        graph->markDirty();
                    }

                    showNodeComponents(connection->componentContainer, nullptr, connection->graph, connection->connectionId, true);
                    showComponentContextMenu();
                }
            }

            ImGui::End();
        }

        void showProjectFolder(ProjectFile const& folder){
            auto selection = &g_UIStatus->selection;

            for (const auto& item : folder.files) {
                if (item.fileType == ProjectFileType::Directory) {
                    if (ImGui::TreeNode(item.path.c_str(), "%s %s", ICON_MD_FOLDER, item.filename.c_str())) {
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
                            // name = "<graph> ";
                            // name = "\xee\xae\xbb ";
                            name = ICON_MD_POLYLINE " ";
                            name += item.filename;
                            name += "##";
                            name += item.path;
                        break;
                        case ProjectFileType::Regular:
                            name = ICON_MD_INSERT_DRIVE_FILE " ";
                            name += strJoin(item.filename, item.path);
                        break;
                        case ProjectFileType::Plugin:
                        case ProjectFileType::Directory:
                            name = strJoin(item.filename, item.path);
                            break;
                    }
                    if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                        logDebug(item.path);

                        if (ImGui::IsMouseDoubleClicked(0)) {
                            auto g = currentGraph();
                            if (g && item.path == g->getFilePath()) {
                                // same file, do nothing.
                                return;
                            }

                            selection->selectedFiles.clear();
                            if (item.fileType == ProjectFileType::Graph) {
                                uiChangeGraph(item.path.c_str());
                            } else {
                                logDebug("unHandle file type: $0", static_cast<int>(item.fileType));
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

            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.project)) {
                showProjectFolder(currentProject()->getFileCache());    
            }

            ImGui::End();
        }

        void showGenerateResultWindow(){
            static float showTipsTime = 0;
            static std::string tipsText{};

            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.generateResult, &g_UIStatus->windowStatus.generateResultWindow)) {
                auto & data =g_UIStatus->generateResultData;
                ImGui::Text("%s", data.source.c_str());
                const auto windowWidth = ImGui::GetWindowWidth() - 13;
                const auto windowHeight = ImGui::GetWindowHeight();
                const auto lineHeight = ImGui::GetFrameHeightWithSpacing() + 2.5f;
                const auto now = ImGui::GetTime();

                ImGui::InputTextMultiline("##text", data.text.data(), data.text.size(), ImVec2(windowWidth, windowHeight - lineHeight*3), ImGuiInputTextFlags_ReadOnly);
                if (ImGui::Button(ICON_MD_CONTENT_COPY)) {
                    ImGui::SetClipboardText(data.text.c_str());
                    tipsText = "Copy Success!";
                    showTipsTime = now + 2.5f;
                }
                if (showTipsTime > now) {
                    ImGui::SameLine();
                    ImGui::Text("%s", tipsText.c_str());
                }

            }
            ImGui::End();
        }

        void showCreateEntityWindow(){
            int inputTextId = 0;

            auto& createEntityData = g_UIStatus->createEntityData;

            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.createEntity, &g_UIStatus->windowStatus.createEntity)) {
                static std::string nameBuf{};
                auto fullName = g_UIStatus->createEntityData.name;
                if (ImGui::InputText(COMMON_LANGUAGE_KEYS.fullName, fullName, std::size(createEntityData.name))) {
                    // update nameBuf
                    nameBuf = getLastAfter(fullName, ".");
                }
                ImGui::InputText(COMMON_LANGUAGE_KEYS.className, nameBuf.data(), std::size(nameBuf), ImGuiInputTextFlags_ReadOnly);
                ImGui::SameLine();
                ImGui::Text("(%s)", COMMON_LANGUAGE_KEYS.readonly);
                ImGui::InputText(COMMON_LANGUAGE_KEYS.address, createEntityData.templateAddress, std::size(createEntityData.templateAddress));
                ImGui::SameLine();
                ImGui::Checkbox("Raw", &createEntityData.useRawTemplateAddress);
                helpMarker("Use raw string as template address? Which means do not add prefix and suffix.");
                ImGui::InputText("parent", createEntityData.parentEntity, std::size(createEntityData.parentEntity));
                helpMarker("Type full entity name, NOT entity's template address!\nInherit fields only!\nFor now, update parent entity DO-NOT affect children!");

                ImGui::Separator();
                // first line buttons
                if (ImGui::Button(ICON_MD_ADD)) {
                    createEntityData.resetFieldsStatus(true, true);
                    createEntityData.addField();
                    createEntityData.lastField()->editing = true;
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_MD_REMOVE)) {
                    createEntityData.deleteSelected();
                }
                ImGui::SameLine();
                if (ImGui::ArrowButton("^", ImGuiDir_Up)) {
                    createEntityData.moveItemUp();
                }
                ImGui::SameLine();
                if (ImGui::ArrowButton("v", ImGuiDir_Down)) {
                    createEntityData.moveItemDown();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_MD_EDIT_OFF)) {
                    createEntityData.resetFieldsStatus(true, true);
                }

                // fields
                char buf[NAME_BUF_SIZE] = {0};
                if (ImGui::BeginTable("fields", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldType);
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldName);
                    ImGui::TableSetupColumn(COMMON_LANGUAGE_KEYS.fieldDefaultValue);
                    ImGui::TableHeadersRow();

                    auto index = -1;
                    for(auto& item: createEntityData.fields){
                        index++;
                        auto p = &item;

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
                            bool oldSelected = createEntityData.selectedFieldIndex == index;
                            sprintf(buf, "##.%d", inputTextId++);
                            if (ImGui::Selectable(buf, oldSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                                if (!ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftCtrl))) {
                                    g_UIStatus->createEntityData.resetFieldsStatus(true, true);
                                }
                                createEntityData.selectedFieldIndex = index;
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
                    }

                    ImGui::EndTable();
                }

                ImGui::Dummy(ImVec2(0, 3));
                ImGui::Text("Options");
                if (createEntityData.selectedFieldIndex >= 0) {
                    auto& field = createEntityData.fields[createEntityData.selectedFieldIndex];
                    showNodePortType(field.fieldOptions.portType);
                    showPortOptions(field.fieldOptions.portOptions);
                }

                ImGui::Dummy(ImVec2(0,3));
                ImGui::Separator();

                auto resetDataAndCloseWindow = []() {
                    g_UIStatus->createEntityData.reset();
                    g_UIStatus->windowStatus.createEntity = false;
                };

                // buttons
                if (ImGui::Button(COMMON_LANGUAGE_KEYS.ok)) {
                    if (createEntityData.isEditMode()) {
                        // 
                        if (nameBuf.empty()) {
                            nameBuf = getLastAfter(fullName, ".");
                        }
                        
                        // todo fix with raw
                        std::string nowTemplateAddress = createEntityData.templateAddress;
                        if (!endsWith(nowTemplateAddress, nameBuf)) {
                            // name changed
                            auto simpleName = createEntityData.editingEntity.getSimpleName();
                            auto pos = nowTemplateAddress.rfind(simpleName);
                            if (pos != std::string::npos) {
                                nowTemplateAddress.erase(pos) += nameBuf;
                                // sprintf(createEntityData.templateAddress, "%s", nowTemplateAddress.c_str());
                                assign(createEntityData.templateAddress, nowTemplateAddress);
                            }
                        }

                        auto p = currentProject();
                        if (checkTemplateNodeIsUsed(p, createEntityData.editingEntity.templateAddress)) {
                            // 
                            if (createEntityData.editingEntity.templateAddress == createEntityData.templateAddress) {
                                // only allow force update when template address does not changed.
                                createEntityData.wantUpdateUsedEntity = true;
                            }
                        }  else {
                            // not used entity.
                            if (updateEntity(createEntityData) == CODE_OK) {
                                resetDataAndCloseWindow();
                            } else {
                                g_UIStatus->toastController.toast(ICON_MD_ERROR " Error", "update entity failed.");
                            }
                        }
                    } else {
                        // add entity
                        int r = addEntity(createEntityData);
                        if (r == 0) {
                            resetDataAndCloseWindow();
                        } else {
                            logDebug(r);
                            if (r == CODE_FAIL) {
                                g_UIStatus->toastController.error().toast("Add entity failed!", "");
                            } else if (r == CODE_CONVERT_ENTITY_FAILED) {
                                g_UIStatus->toastController.error().toast("Add entity failed!", "Convert to SightEntity failed!Perhaps parent name error!");
                            } else {
                                g_UIStatus->toastController.error().toast("Add entity failed!", "Unkown reason.");
                            }
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                    resetDataAndCloseWindow();
                }

                if (createEntityData.isEditMode()) {
                    // In edit mode
                    // ImGui::SameLine();
                    ImGui::Text("In Edit Mode");

                    if (createEntityData.wantUpdateUsedEntity) {
                        ImGui::SameLine();
                        if (ImGui::Button("Force Update")) {
                            // update
                            auto askCallBack = [&createEntityData](bool f) {
                                if (!f) {
                                    return;
                                }

                                int code = CODE_OK;
                                if ((code = updateEntity(createEntityData)) == CODE_OK) {
                                    g_UIStatus->toastController.toast("Update entity success " ICON_MD_DONE, "");
                                    uiReloadGraph();
                                } else {
                                    
                                    g_UIStatus->toastController.toast("Update entity fail " ICON_MD_ERROR, "Code: " + std::to_string(code));
                                }
                            };

                            openAskModal(ICON_MD_QUESTION_MARK " Force Update?", "It maybe broken graph,do you really want to update entity?\nAnd it can't be undo.", askCallBack);
                        }
                    }
                }
                showEntityOperations(nullptr, &createEntityData);
            }
            ImGui::End();
        }

        void showEntityInfoWindow(){
            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.entityInfo, &g_UIStatus->windowStatus.entityInfoWindow)) {
                auto& name = g_UIStatus->createEntityData.showInfoEntityName;
                if (name.empty()) {
                    ImGui::Text("Select an entity to show info.");
                } else {
                    auto entityData = currentProject()->getSightEntity(name);
                    if (entityData) {
                        ImGui::Text("%s", entityData->name.c_str());
                        showEntityOperations(entityData);
                    } else {
                        ImGui::Text("Entity not found: ");
                        ImGui::SameLine();
                        ImGui::Text("%s", name.c_str());
                    }

                    // operations
                    ImGui::Text("Operations: ");
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MD_EDIT "##edit-")) {
                        // edit
                        uiEditEntity(*entityData);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MD_DELETE "##delete-")) {
                        uiDeleteEntity(name);
                        g_UIStatus->createEntityData.showInfoEntityName.clear();
                    }
                }   
            }
            ImGui::End();
        }

        void showEntityListWindow(){
            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.entityList, &g_UIStatus->windowStatus.entityListWindow)) {
                ImGui::InputText(ICON_MD_SEARCH "##Filter", g_UIStatus->buffer.entityListSearch, std::size(g_UIStatus->buffer.entityListSearch));
                const auto windowWidth = ImGui::GetWindowWidth();
                constexpr const auto buttonSize = 26;
                constexpr const auto buttonInterval = 5;

                ImGui::SameLine(windowWidth - 60);

                if (ImGui::Button("Create")) {
                    g_UIStatus->createEntityData.reset();
                    activeWindowCreateEntity();
                }
                ImGui::Separator();

                auto p = currentProject();
                // also be a template address.
                std::string delFullName{};
                auto entityListSearch = g_UIStatus->buffer.entityListSearch;
                
                for( const auto& [name, info]: p->getEntitiesMap()){
                    if (strlen(entityListSearch) > 0) {
                        if (!absl::StrContains(name, entityListSearch)) {
                            continue;
                        }
                    }

                    ImGui::Text("%8s", info.getSimpleName().c_str());
                    ImGui::SameLine();
                    ImGui::Text("%18s", name.c_str());

                    auto width = windowWidth - buttonSize - buttonInterval;
                    ImGui::SameLine(width);
                    std::string infoLabel{ ICON_MD_INFO "##info-" };
                    infoLabel += name;
                    if (ImGui::Button(infoLabel.c_str())) {
                        openEntityInfoWindow(name);
                    }

                    width = width - buttonSize - buttonInterval;
                    ImGui::SameLine(width);
                    std::string editButtonLabel{ICON_MD_EDIT "##edit-"};
                    editButtonLabel += name;
                    if (ImGui::Button(editButtonLabel.c_str())) {
                        // edit
                        uiEditEntity(info);
                    }

                    width = width - buttonSize - buttonInterval;
                    ImGui::SameLine(width);
                    std::string delButtonLabel{ICON_MD_DELETE "##del-" };
                    delButtonLabel += name;
                    if (ImGui::Button(delButtonLabel.c_str())) {
                        delFullName = name;
                    }

                    
                }

                if (!delFullName.empty()) {
                    uiDeleteEntity(delFullName);
                }
            }
            ImGui::End();
        }

        void showAboutWindow(){
            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.about, &g_UIStatus->windowStatus.aboutWindow)) {
                ImGui::Text("ImGui version: %s", ImGui::GetVersion());
                ImGui::Text("v8 version: %s", v8::V8::GetVersion());
                ImGui::Separator();

                ImGui::Text("Sight WIP v0.1");
                ImGui::Text("Author: aincvy(aincvy@gmail.com)");
                ImGui::Dummy(ImVec2(0,15));
                if (ImGui::Button("Close")) {
                    g_UIStatus->windowStatus.aboutWindow = false;
                }
                ImGui::End();
            }
        }

        void showProjectSettingsWindow(){
            static bool panelBasic = true, panelTypes = false, panelPlugins = false;

            auto resetFlags = [&](){
                panelBasic = false;
                panelTypes = false;
                panelPlugins = false;
            };

            ImGui::SetNextWindowSize(ImVec2(600,440), ImGuiCond_FirstUseEver);
            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.projectSettings, &g_UIStatus->windowStatus.projectSettingsWindow)) {
                
                // left panel
                ImGui::BeginChild("left panel", ImVec2(150,0), true);
                if (ImGui::Selectable("Basic", panelBasic)) {
                    resetFlags();
                    panelBasic = true;
                }
                if (ImGui::Selectable("Types", panelTypes)) {
                    resetFlags();
                    panelTypes = true;
                }
                if (ImGui::Selectable("Plugins", panelPlugins)) {
                    resetFlags();
                    panelPlugins = true;
                }
                ImGui::EndChild();
                ImGui::SameLine();

                // right panel
                ImGui::BeginGroup();
                ImGui::BeginChild("right panel", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
                if (panelBasic) {
                    ImGui::Text("Basic Info");
                } else if (panelTypes) {
                    ImGui::Text("Types");
                } else if (panelPlugins) {
                    ImGui::Text("Plugins");
                }
                ImGui::Separator();
                if (panelBasic) {
                    // show some project basic info.
                    auto p = currentProject();
                    ImGui::Text("Path: %s", p->getBaseDir().c_str());
                    ImGui::Text("Loaded Plugins: %u", pluginManager()->getLoadedPluginCount());
                    
                    auto memUsage = currentProcessUsageInfo();
                    ImGui::Text("Memory Usage.");
                    ImGui::Text("   Virtual Memory(mb): %.2f", memUsage.virtualMemBytes / 1024.0 / 1024);
                    ImGui::Text("  Resident Memory(mb): %.2f", memUsage.residentMemBytes / 1024.0 / 1024);

                } else if (panelTypes) {
                    // show types.
                    auto typeInfoMap =currentProject()->getTypeInfoMap();
                    for( const auto& [key, value]: typeInfoMap){
                        ImGui::Text("%4d: %8s", key, value.name.c_str());
                        // show style
                        if (value.style) {
                            auto style = value.style;
                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(120);
                            char labelBuf[LITTLE_NAME_BUF_SIZE]{ 0 };
                            sprintf(labelBuf, "##style.iconType-%d", key);
                            if (ImGui::BeginCombo(labelBuf, getIconTypeName(style->iconType))) {
                                auto [iconNames, iconSize] = getIconTypeStrings();
                                for (int i = 0; i < iconSize; i++) {
                                    auto tmpIndex = static_cast<int>(IconType::Flow);
                                    if (ImGui::Selectable(iconNames[i], i == static_cast<int>(style->iconType) - tmpIndex)) {
                                        style->iconType = static_cast<IconType>(tmpIndex + i);
                                    }
                                }
                                
                                ImGui::EndCombo();
                            }

                            ImGui::SameLine();

                            // ImGui::ColorEdit4("##style.color", ImColor());
                            ImGui::SetNextItemWidth(180);
                            ImVec4 color = ImColor(style->color);
                            char colorLabelBuf[LITTLE_NAME_BUF_SIZE]{0};
                            sprintf(colorLabelBuf, "##style.color-%d", key);
                            if (ImGui::ColorEdit3(colorLabelBuf, (float*)&color)) {
                                style->color = ImColor(color);
                            }
                        }
                    }
                } else if (panelPlugins) {
                    // show loaded plugins.
                    auto map = pluginManager()->getSnapshotMap();
                    auto pluginNames = pluginManager()->getSortedPluginNames();
                    for (const auto& key : pluginNames) {
                        auto plugin = map[key];
                        ImGui::Text("%s", key.c_str());
                        ImGui::Text("%7s: %s", "Author", plugin->getAuthor());
                        ImGui::Text("%7s: %s", "Version", plugin->getVersion());
                        ImGui::Text("%7s: %s", "Path", plugin->getPath());
                        ImGui::Text("%7s: %s", "Status", plugin->getPluginStatusString());
                        
                        constexpr const char* reloadLabel = ICON_MD_REFRESH;
                        std::string labelBuf = reloadLabel;
                        labelBuf += "##reload-";
                        labelBuf += key;        
                        ImGui::BeginDisabled(!plugin->isReloadAble());
                        if (ImGui::Button(labelBuf.c_str())) {
                            // send reload 
                            addJsCommand(JsCommandType::PluginReload, strdup(key.c_str()), key.length(), true);
                        }
                        ImGui::EndDisabled();

                        // open url
                        labelBuf = ICON_MD_OPEN_IN_BROWSER;
                        labelBuf += "##openUrl-";
                        labelBuf += key;
                        ImGui::SameLine();
                        ImGui::BeginDisabled(plugin->isUrlEmpty());
                        if (ImGui::Button(labelBuf.c_str())) {
                            // open 
                            openUrlWithDefaultBrowser(plugin->getUrl());
                        }
                        ImGui::EndDisabled();

                        ImGui::Separator();
                    }
                }
                ImGui::EndChild();
                ImGui::EndGroup();

                ImGui::End();
            }
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
                    logDebug(g_UIStatus->buffer.littleName);
                    auto p = currentProject();
                    std::string path = p->pathGraph(g_UIStatus->buffer.littleName);
                    uiChangeGraph(path);
                    p->buildFilesCache();
                    g_UIStatus->buffer.littleName[0] = '\0';
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
            return;
        }

        // ask
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
            return;
        }

        // alert
        auto& modalAlertData = g_UIStatus->modalAlertData;
        if (windowStatus.popupAlertModal) {
            windowStatus.popupAlertModal = false;

            if (!endsWith(modalAlertData.title, MyUILabels::modalAlertData)) {
                modalAlertData.title += MyUILabels::modalAlertData;
            }
            ImGui::OpenPopup(MyUILabels::modalAlertData);

            // Always center this window when appearing
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }
        if (ImGui::BeginPopupModal(modalAlertData.title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", modalAlertData.content.c_str());
            ImGui::Dummy(ImVec2(0, 18));

            if (buttonOK(COMMON_LANGUAGE_KEYS.ok)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
            return;
        }

        // save
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
            return;
        }
    }

    /**
     * @brief Handle some common keyboard input
     * 
     */
    void handleKeyboardInput(){
        auto keybindings = g_UIStatus->keybindings;
        if (!keybindings->controlKey.isKeyDown()) {
            return;
        }

        // logDebug("got control key");
        if (keybindings->saveFile) {
            logDebug("save graph");
            saveAnyThing();
        } else if (keybindings->undo) {
            undo();
        } else if (keybindings->redo) {
            redo();  
        } else if (keybindings->copy) {
            auto & selectedNodeOrLinks =g_UIStatus->selection.selectedNodeOrLinks;
            if (selectedNodeOrLinks.size() == 1) {
                auto n = g_UIStatus->selection.getSelectedNode();
                if (n) {
                    ImGui::SetClipboardText(CopyText::from(*n).c_str());
                }
            } else {
                // multiple
                std::vector<SightNode*> nodes;
                std::vector<SightNodeConnection> connections;
                g_UIStatus->selection.getSelected(nodes, connections);
                if (!nodes.empty()) {
                    ImGui::SetClipboardText(CopyText::from(nodes, connections).c_str());
                }
            }
        }
        
    }

    void showStatusBar(){
        //
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
        float height = ImGui::GetFrameHeight();
        
        auto& data = g_UIStatus->statusBarData;
        if (ImGui::BeginViewportSideBar("##MainStatusBar", NULL, ImGuiDir_Down, height, window_flags)) {
            if (ImGui::BeginMenuBar()) {
                if (!data.logText.empty()) {
                    // show
                    switch (data.logLevel) {
                    case LogLevel::Warning:
                        ImGui::TextColored(Colors::warning, ICON_MD_INFO "%s", data.logText.c_str());
                        break;
                    case LogLevel::Error:
                        ImGui::TextColored(Colors::error, ICON_MD_INFO "%s", data.logText.c_str());
                        break;
                    default:
                        ImGui::Text(ICON_MD_INFO "%s", data.logText.c_str());
                        break;
                    }
                }

                // start from right-most.
                auto width = ImGui::GetWindowWidth();
                ImGui::SameLine(width - 35);
                ImGui::PushStyleColor(ImGuiCol_Button, ImColor().Value);
                if (ImGui::Button(ICON_MD_PLAY_ARROW)) {
                    logDebug("want to build, but not impl");
                }
                ImGui::PopStyleColor();

                ImGui::EndMenuBar();
                // ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            }
        }
        ImGui::End();
    }

    void mainWindowFrame(UIStatus & uiStatus) {
        // HierarchyWindow and InspectorWindow will use node editor data.
        nodeEditorFrameBegin();

        showModals();
        handleKeyboardInput();
        showMainMenuBar(uiStatus);
        showProjectWindow();

        // // windows
        showHierarchyWindow();
        showInspectorWindow();
        showNodeEditorGraph(uiStatus);

        auto& windowStatus = uiStatus.windowStatus;
        if (windowStatus.createEntity) {
            showCreateEntityWindow();
        }
        if (windowStatus.testWindow) {
            showDemoWindow(false);
        }
        if (windowStatus.aboutWindow) {
            showAboutWindow();
        }
        if (windowStatus.projectSettingsWindow) {
            showProjectSettingsWindow();
        }
        if (windowStatus.entityListWindow) {
            showEntityListWindow();
        }
        if (windowStatus.generateResultWindow) {
            showGenerateResultWindow();
        }
        if (windowStatus.entityInfoWindow) {
            showEntityInfoWindow();
        }
        if (windowStatus.terminalWindow) {
            auto& terminal = uiStatus.terminalData.terminal;

            terminal->use_default_size();
            windowStatus.terminalWindow = terminal->show();
        }
        if(windowStatus.codeSetSettingsWindow){
            showCodeSetSettingsWindow();
        }

        nodeEditorFrameEnd();

        uiStatus.toastController.render();
        showStatusBar();

        if (uiStatus.windowStatus.layoutReset) {
            uiStatus.windowStatus.layoutReset = false;
        }
    }

    void runUICommand(UICommand *command){
        logDebug(static_cast<int>(command->type));
        switch (command->type) {
            case UICommandType::UICommandHolder:
                break;
            case UICommandType::COMMON:
                break;
            case UICommandType::JsEndInit: {
                logDebug("js end init.");
                g_UIStatus->loadingStatus.jsThread = true;
                break;
            }
            case UICommandType::AddNode:
            {
                // command->args.needFree = false;
                auto* nodePointer = (SightNode**) command->args.data;
                auto size = command->args.dataLength;
                for (int i = 0; i < size; ++i) {
                    uiAddNode(nodePointer[i]);
                }
                // pluginManager()->getPluginStatus().addNodesFinished = true;
                break;
            }
            case UICommandType::AddTemplateNode:{
                auto *pointer = (SightNodeTemplateAddress**) command->args.data;
                auto size = command->args.dataLength;
                for (int i = 0; i < size; ++i) {
                    auto p = pointer[i];
                    addTemplateNode(*p);
                    p->dispose();    // free SightJsNode first.
                    delete p;
                }
                // pluginManager()->getPluginStatus().addTemplateNodesFinished = true;
                break;
            } 
            case UICommandType::RegScriptGlobalFunctions: {
                std::map<std::string, std::string>* map = (std::map<std::string, std::string>*) command->args.data;
                command->args.data = nullptr;
                registerToGlobal(g_UIStatus->isolate, map);
                delete map;
                break;
            }
            case UICommandType::RunScriptFile:{
                // 
                auto isolate = g_UIStatus->isolate;
                v8::HandleScope handleScope(isolate);
                auto module = v8::Object::New(isolate);
                if (runJsFile(isolate, command->args.argString, nullptr, module) == CODE_OK) {
                    // 
                    logDebug("ok");
                    registerToGlobal(isolate, module->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "globals")).ToLocalChecked());
                };

                break;
            }
            case UICommandType::PluginReloadOver:
                // std::string msg= "";
                g_UIStatus->toastController.toast("Plugin Reload", command->args.argString);
                break;
            }

        command->args.dispose();
        uiCommandFree = true;
    }

    void runUICommandCallback(uv_async_t *handle){
        auto *command = (UICommand *)handle->data;
        runUICommand(command);
        free(command);
    }


    int showLoadingWindow() {
        logDebug("start loading");
        // Create window with graphics context
        const int width = 640;
        const int height = 360;

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();

        auto sightWindow = initWindow("sight - loading", width, height, [](ImGuiIO& io) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        }, true);
        if (!sightWindow) {
            return CODE_FAIL;
        }

        ImGuiIO& io = ImGui::GetIO();
        // init ...
        g_UIStatus = new UIStatus({
                                    true,
                                    &io,
                                    false,
                                    getSightSettings()->windowStatus,
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

        // init ui v8
        logDebug("init ui v8 Isolate");
        v8::Isolate::CreateParams createParams;
        createParams.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
        g_UIStatus->isolate = v8::Isolate::New(createParams);
        g_UIStatus->arrayBufferAllocator = createParams.array_buffer_allocator;
        auto isolate = g_UIStatus->isolate;
        v8::Isolate::Scope isolateScope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        g_UIStatus->v8GlobalContext.Reset(isolate, context);
        v8::Context::Scope contextScope(context);

        // bind types and functions
        v8pp::module module(isolate);
        bindBaseFunctions(isolate, context, module);
        bindTinyData(isolate, context);
        bindNodeTypes(isolate, context, module);
        bindUIThreadFunctions(context, module);
        context->Global()->Set(context, v8pp::to_v8(isolate, "sight"), module.new_instance()).ToChecked();

        // terminal and log
        g_UIStatus->terminalData.terminal = new ImTerm::terminal<TerminalCommands>(g_UIStatus->terminalData.runtimeArgs);
        g_UIStatus->terminalData.terminal->theme() = ImTerm::themes::cherry;
        auto logWriterFunc = [](LogLevel l, std::string_view msg) {
            if (!g_UIStatus || g_UIStatus->terminalData.stopLogging) {
                return;
            }

            ImTerm::message::severity::severity_t s = ImTerm::message::severity::debug;
            switch (l) {
            case LogLevel::Trace:
                s = ImTerm::message::severity::trace;
                break;
            case LogLevel::Debug:
                break;
            case LogLevel::Info:
                s = ImTerm::message::severity::info;
                break;
            case LogLevel::Warning:
                s = ImTerm::message::severity::warn;
                break;
            case LogLevel::Error:
                s = ImTerm::message::severity::err;
                break;
            }
            g_UIStatus->terminalData.terminal->add_message({s, std::string{msg}, 0, msg.length()});
            g_UIStatus->statusBarData.logLevel = l;
            g_UIStatus->statusBarData.logText = msg;
        };
        registerLogWriter(logWriterFunc);

        // 
        uint progress = 0;
        bool pluginStartLoad = false;
        bool exitFlag = false;
        bool folderError = false;
        std::string lastOpenFolder = ".";


        auto beforeRenderFunc = [uvLoop]() -> int {
            uv_run(uvLoop, UV_RUN_NOWAIT);
            auto project = currentProject();
            if (g_UIStatus->isLoadingOver() && project ) {
                logDebug("project loading over, should be turn into main window..");
                return CODE_FAIL;
            }
            return CODE_OK;
        };

        //
        auto renderFunc = [=,&exitFlag, &progress, &uiStatus, &pluginStartLoad, &folderError, &lastOpenFolder]() {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(width, height));
            ImGui::Begin("loading", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::Text("sight is loading...");
            ImGui::Text("past frame: %d", ++progress);
            ImGui::Separator();
            ImGui::BeginChild("content view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
            ImGui::Dummy(ImVec2(1, 5));
            auto project = currentProject();

            if (project) {
                ImGui::Text("Project: %s", project->getBaseDir().c_str());

                if (!pluginStartLoad) {
                    pluginStartLoad = true;

                    // load plugins
                    addJsCommand(JsCommandType::InitParser);
                    addJsCommand(JsCommandType::InitPluginManager);
                    addJsCommand(JsCommandType::ProjectLoadPlugins);
                    addJsCommand(JsCommandType::EndInit);
                } else {
                    ImGui::Text("Loading plugins...");
                }
            } else {
                ImGui::Text("If project doesn't exist, then will create it.");
                if (ImGui::Button(MENU_LANGUAGE_KEYS.openProject)) {
                    uiOpenProject(folderError, lastOpenFolder, false);
                }

                ImGui::Dummy(ImVec2(1, 5));
                if (lastOpenFolder != ".") {
                    ImGui::Text("Try Open Folder: %s", lastOpenFolder.c_str());
                }
                if (folderError) {
                    ImGui::Text("Folder do not empty, or not a sight project folder, or loading !");
                }
            }

            ImGui::EndChild();

            if (ImGui::Button("Exit")) {
                exitFlag = true;
            }

            //
            if (uiStatus.loadingStatus.jsThread && !uiStatus.loadingStatus.nodeStyle) {
                // update node style.
                // currentNodeStatus()->updateTemplateNodeStyles();    // still has errors...
                uiStatus.loadingStatus.nodeStyle = true;
                logDebug("nodeStyle load over!");
            }

            ImGui::End();
        };

        mainLoopWindow(sightWindow, exitFlag, false, beforeRenderFunc, renderFunc);

        // load template nodes.
        for (int i = 0; i < 30; ++i) {
            uv_run(uvLoop, UV_RUN_NOWAIT);
        }

        // Cleanup
        cleanUpWindow(sightWindow);

        logDebug("end loading");
        return 0;
    }

    int showMainWindow(){
        if (!g_UIStatus) {
            return CODE_FAIL;
        }

        logDebug("start show main window...");

        // create js context
        auto isolate = g_UIStatus->isolate;
        v8::Isolate::Scope isolateScope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = g_UIStatus->v8GlobalContext.Get(isolate);
        v8::Context::Scope contextScope(context);
        logDebug("v8 runtime init over.");

        auto sightSettings = getSightSettings();
        auto sightWindow = initWindow("sight - a code generate tool", sightSettings->lastMainWindowWidth, sightSettings->lastMainWindowHeight, [](ImGuiIO& io) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        });
        // Setup Dear ImGui context

        ImGuiIO& io = ImGui::GetIO();
        
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.Fonts->AddFontDefault();
        ImFontConfig config;
        config.MergeMode = true;
        config.GlyphMinAdvanceX = 13.0f;
        auto fontPointer = io.Fonts->AddFontFromFileTTF((resourceFolder + "font/FangZhengKaiTiJianTi-1.ttf").c_str(), 16.0f, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        // icon
        static const ImWchar icon_ranges[] = { ICON_MIN_MD, ICON_MAX_MD, 0 };
        config.GlyphOffset.y += 5.5f;
        auto iconFontPointer = io.Fonts->AddFontFromFileTTF((resourceFolder + "font/MaterialIconsOutlined-Regular.otf").c_str(), 18.0f, &config, icon_ranges);

        io.Fonts->Build();

        g_UIStatus->io = &io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        

        // project path 
        auto project = currentProject();
        if (project) {
            onProjectAndUILoadSuccess(project);
        }

        initUndo();

        auto uvLoop = g_UIStatus->uvLoop;

        auto beforeRenderFunc = [uvLoop]() -> int {
            uv_run(uvLoop, UV_RUN_NOWAIT);
            return g_UIStatus->closeWindow ? CODE_FAIL : CODE_OK;
        };

        auto renderFunc = []() {
            mainWindowFrame(*g_UIStatus);

            // ImGui::ShowDemoWindow();     // for debug

            g_UIStatus->needInit = false;
        };

        bool exitFlag = false;
        mainLoopWindow(sightWindow, exitFlag, true, beforeRenderFunc, renderFunc);
        // Cleanup
        cleanUpWindow(sightWindow);

        return 0;
    }

    void destroyWindows() {
        destroyNodeEditor(true, true);

        terminateBackend();

        g_UIStatus->entityOperations.reset();
        g_UIStatus->v8GlobalContext.Reset();
        g_UIStatus->isolate->Dispose();
        g_UIStatus->isolate = nullptr;
        delete g_UIStatus->arrayBufferAllocator;
        g_UIStatus->arrayBufferAllocator = nullptr;

        // free uv
        uv_loop_close(g_UIStatus->uvLoop);
        free(g_UIStatus->uvLoop);
        g_UIStatus->uvLoop = nullptr;
        delete g_UIStatus->uvAsync;
        g_UIStatus->uvAsync = nullptr;


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

    void addUICommand(UICommandType type, CommandArgs&& args) {
        UICommand command{ type, args };
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

    int addUICommand(UICommandType type, const char *argString, int length, bool needFree) {
        UICommand command = {
            type,
            {
                .argString = argString,
                .argStringLength = length,
                .needFree = needFree,
            }
        };

        return addUICommand(command);
    }

    int addUICommand(UICommand &command) {
        while (!uiCommandFree) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        uiCommandFree = false;
        auto async = g_UIStatus->uvAsync;
        async->data = copyObject(&command, sizeof(UICommand));
        uv_async_send(async);
        return CODE_OK;
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
                if (alwaysShow || (showValue && item.options.showValue) || item.options.typeList) {
                    ImGui::Text("%7s: ", item.portName.c_str());
                    ImGui::SameLine();
                    if (item.options.typeList)
                    {
                        // 
                        // ImGui::InputText("##name", char *buf, size_t buf_size)
                        // ImGui::SameLine();
                        if (ImGui::Button(ICON_MD_SEARCH "##toggle_type_list"))
                        {
                            item.ownOptions.typeList = !item.ownOptions.typeList;
                        }
                    } else {
                        showNodePortValue(&item);
                    }
                }

                if(item.ownOptions.typeList) {
                    std::string typeName = getTypeName(item.type);
                    if (showTypeList(typeName))
                    {
                        auto tmpType = getIntType(typeName);
                        if(tmpType != 0){
                            logDebug("change port $0 type from $1 to $2, now type name: $3", item.id, item.type, tmpType, typeName);
                            item.type = tmpType;
                            item.ownOptions.typeList = false;
                        }
                    }
                } 
            }
        };

        nodeWork(node->fields, showValue, showField);
        nodeWork(node->outputPorts, showValue, showOutput);
        nodeWork(node->inputPorts, showValue, showInput);

    }

    void uiOpenProject(bool& folderError, std::string& lastOpenFolder, bool callLoadSuccess) {
        // try open 
        int status = CODE_OK;
        std::string path = openFolderDialog(lastOpenFolder.c_str(), &status);
        if (status == CODE_OK) {
            logDebug(path);
            folderError = false;

            lastOpenFolder = path;
            if (verifyProjectFolder(lastOpenFolder.c_str()) != CODE_OK) {
                folderError = true;
            } else {
                // free resource
                auto g = currentGraph();
                if (g) {
                    g->save();
                    disposeGraph();
                    g = nullptr;
                }
                disposeProject();

                if (openProject(lastOpenFolder.c_str(), true, callLoadSuccess) == CODE_OK) {
                    getSightSettings()->lastOpenProject = lastOpenFolder;
                    saveSightSettings();
                } else {
                    folderError = true;
                }
            }
        }
    }

    void openSaveModal(const char* title, const char* content, std::function<void(SaveOperationResult)> const& callback) {
        auto& askData = g_UIStatus->modalSaveData;
        askData.title = title;
        askData.content = content;
        askData.callback = callback;

        g_UIStatus->windowStatus.popupSaveModal = true;
    }

    bool isUICommandFree() {
        return uiCommandFree;
    }

    void openAskModal(std::string_view title, std::string_view content, std::function<void(bool)> callback) {
        auto& data = g_UIStatus->modalAskData;
        data.title = title;
        data.content = content;
        data.callback = callback;

        g_UIStatus->windowStatus.popupAskModal = true;
    }

    void openAlertModal(std::string_view title, std::string_view content) {
        auto& data = g_UIStatus->modalAlertData;
        data.title = title;
        data.content = content;

        g_UIStatus->windowStatus.popupAlertModal = true;
    }

    void openGenerateResultWindow(std::string const& source, std::string const& text) {
        auto &data = g_UIStatus->generateResultData;
        data.source = source;
        data.text = text;
        if (g_UIStatus->windowStatus.generateResultWindow) {
            ImGui::SetWindowFocus(WINDOW_LANGUAGE_KEYS.generateResult);
        } else {
            g_UIStatus->windowStatus.generateResultWindow = true;
        }
    }

    void openEntityInfoWindow(std::string_view entityName) {
        g_UIStatus->createEntityData.showInfoEntityName = entityName;

        showOrFocusWindow(g_UIStatus->windowStatus.entityInfoWindow, WINDOW_LANGUAGE_KEYS.entityInfo);
    }

    void toast(std::string_view title, std::string_view content, float time) {
        g_UIStatus->toastController.info().toast(title, content, time);        
    }

    void UICreateEntity::addField() {
        fields.push_back({});     //  .editing = true
        selectedFieldIndex = fields.size() - 1;
    }

    void UICreateEntity::resetFieldsStatus(bool editing, bool selected) {
        for(auto& item: fields){
            if (editing) {
                item.editing = false;
            }
        }
        if (selected) {
            selectedFieldIndex = -1;
        }
    }

    EntityField *UICreateEntity::lastField() {
        if (fields.empty()) {
            return nullptr;
        }
        return & fields.back();
    }

    int UICreateEntity::deleteSelected() {
        if (selectedFieldIndex >= 0 && selectedFieldIndex < fields.size()) {
            fields.erase(fields.begin() + selectedFieldIndex);
            if (!fields.empty()) {
                if (selectedFieldIndex >= fields.size()) {
                    selectedFieldIndex = fields.size() - 1;
                }
            }
            
            return 1;
        }
        return 0;
    }

    void UICreateEntity::moveItemUp() {
        if (selectedFieldIndex >= 1) {
            std::swap(fields[selectedFieldIndex], fields[selectedFieldIndex - 1]);
            selectedFieldIndex--;
        }
    }

    void UICreateEntity::moveItemDown() {
        if (selectedFieldIndex < fields.size() - 1) {
            std::swap(fields[selectedFieldIndex], fields[selectedFieldIndex + 1]);
            selectedFieldIndex++;
        }
    }

    void UICreateEntity::loadFrom(struct SightNode *node) {
        if (!node) {
            return;
        }
        reset();

        sprintf(this->name, "%s", node->nodeName.c_str());

        // For entity node, inputs is same as outputs, so just foreach one is fine.
        for (const auto &item : node->inputPorts) {
            addField();

            auto p = lastField();
            sprintf(p->name, "%s", item.portName.c_str());
            sprintf(p->type, "%s", getTypeName(item.getType()).c_str());
            sprintf(p->defaultValue, "%s", item.getDefaultValue());
        }

    }

    void UICreateEntity::loadFrom(SightEntity const& info) {
        reset();

        // convert data to  createEntityData
        snprintf(this->name, std::size(this->name), "%s", info.name.c_str());
        snprintf(this->templateAddress, std::size(this->templateAddress), "%s", info.templateAddress.c_str());
        snprintf(this->parentEntity, std::size(this->parentEntity), "%s", info.parentEntity.c_str());

        for( const auto& item: info.fields){
            addField();

            auto p = lastField();
            snprintf(p->name, std::size(p->name), "%s", item.name.c_str());
            snprintf(p->type, std::size(p->type), "%s", item.type.c_str());
            snprintf(p->defaultValue, std::size(p->defaultValue), "%s", item.defaultValue.c_str());

            p->fieldOptions = item.options;
        }
    }

    void UICreateEntity::reset() {
        sprintf(this->name, "");
        sprintf(this->templateAddress, "");
        sprintf(this->parentEntity, "");
        this->editingEntity = {};
        this->edit = false;
        this->useRawTemplateAddress = false;
        this->wantUpdateUsedEntity = false;
        this->selectedFieldIndex = -1;

        fields.clear();
    }

    bool UICreateEntity::isEditMode() const {
        return edit;
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
        return jsThread && nodeStyle;
    }

    std::string Selection::getProjectPath() const {
        return this->projectPath;
    }

    SightNode* Selection::getSelectedNode() const {
        if (selectedNodeOrLinks.empty()) {
            return nullptr;
        }
        return currentGraph()->findNode( *selectedNodeOrLinks.begin() );
    }

    SightNodeConnection* Selection::getSelectedConnection() const {
        if (selectedNodeOrLinks.empty()) {
            return nullptr;
        }

        return currentGraph()->findConnection(*selectedNodeOrLinks.begin());
    }

    int Selection::getSelected(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections) const {
        for (const auto& item : selectedNodeOrLinks) {
            auto thing = currentGraph()->findSightAnyThing(item);
            if (thing.type == SightAnyThingType::Node) {
                auto n = thing.asNode();
                if (n) {
                    nodes.push_back(n);
                }
            } else if (thing.type == SightAnyThingType::Connection) {
                auto c = thing.asConnection();
                if (c) {
                    connections.push_back(*c);
                }
            }
        }

        if (!nodes.empty()) {
            // auto copy link
            auto graph = currentGraph();
            for (const auto& item : nodes) {     // find .......
                for (const auto& port : item->outputPorts) {
                    if (!port.isConnect()) {
                        continue;
                    }

                    for (const auto& connection : port.connections) {
                        SightNodePortConnection c(graph, connection, item);
                        if (!c.target) {
                            continue;
                        }

                        for (const auto& nodePointer : nodes) {
                            if (item == nodePointer) {
                                continue;
                            }

                            if (c.target->node == nodePointer) {
                                connections.push_back(*connection);
                                break;
                            }
                        }
                    }
                }
            }
        }

        return CODE_OK;
    }

    bool EntityOperations::addOperation(const char* name, const char* desc, ScriptFunctionWrapper::Function const& f, bool replace) {
        if (replace) {
            auto old = map.find(name);
            if (old != map.end()) {
                map.erase(old);
            }
        }

        auto [iter, succ] = map.try_emplace(name, name, desc, f);
        if (!succ) {
            return false;
        }

        names.push_back(name);
        return true;
    }

    void EntityOperations::reset() {
        this->map.clear();
        this->names.clear();
        this->selected = 0;
    }

    void uiEditEntity(SightEntity const& info) {
        auto& createEntityData = g_UIStatus->createEntityData;
        createEntityData.loadFrom(info);
        createEntityData.edit = true;
        createEntityData.editingEntity = info;

        activeWindowCreateEntity();
    }

    void showEntityOperations(SightEntity* sightEntity /* = nullptr */, UICreateEntity* createEntityData /* = nullptr */) {
        auto& entityOperations = g_UIStatus->entityOperations;
        checkEntityOperations(entityOperations);
        const auto& operationNames = entityOperations.names;
        
        if (!operationNames.empty()) {
            const auto width = ImGui::GetWindowWidth();
            constexpr const auto comboWidth = 110;
            const char* comboLabel = "##Operations";
            const auto comboLabelWidth = ImGui::CalcTextSize(comboLabel).x;
            const char* generateStr = "Generate";
            const auto generateWidth = ImGui::CalcTextSize(generateStr).x + 15;
            auto selectedOperationName = operationNames[entityOperations.selected].c_str();

            ImGui::SameLine(width - generateWidth);
            if (ImGui::Button(generateStr)) {
                // call generate
                auto& o = entityOperations.map[selectedOperationName];

                std::string text{};
                if (!sightEntity) {
                    assert(createEntityData);
                    SightEntity tmpSightEntity;
                    convert(tmpSightEntity, *createEntityData);
                    sightEntity = &tmpSightEntity;
                    text = o.function.getString(currentUIStatus()->isolate, sightEntity);
                } else {
                    text = o.function.getString(currentUIStatus()->isolate, sightEntity);
                }
                
                text = removeExcessSpaces(text);
                openGenerateResultWindow(generateStr, text);     // todo more info about source
            }
            ImGui::SameLine(width - comboWidth - generateWidth - 5);
            ImGui::SetNextItemWidth(comboWidth);
            if (ImGui::BeginCombo(comboLabel, selectedOperationName)) {
                int tmpIndex = -1;
                for (const auto& item : operationNames) {
                    bool isSelected = (++tmpIndex) == entityOperations.selected;
                    if (ImGui::Selectable(item.c_str(), isSelected)) {
                        entityOperations.selected = tmpIndex;
                        
                        getSightSettings()->lastUseEntityOperation = item;
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            helpMarker(entityOperations.map[selectedOperationName].description.c_str());
        }
    }

    void checkEntityOperations(EntityOperations& entityOperations) {
        const auto& operationNames = entityOperations.names;
        if (!entityOperations.init) {
            entityOperations.init = true;
            // auto lastUse = getSightSettings()->lastUseEntityOperation;
            auto lastUse = currentProject()->getSightCodeSetSettings().entityTemplateFuncName;
            if (!lastUse.empty()) {
                auto iter = std::find(operationNames.begin(), operationNames.end(), lastUse);
                if (iter != operationNames.end()) {
                    entityOperations.selected = iter - operationNames.begin();
                }
            }
        }
    }

    void uiGraphToJson(SightNodeGraph* graph, std::string_view pathArg, bool selectSaveFile) {

        if (selectSaveFile) {
            int status = CODE_OK;
            std::string filename = graph->getName().data();
            filename.append(".json");

            auto path = saveFileDialog(".", &status, filename);
            if (status != CODE_OK) {
                if (status == CODE_USER_CANCELED) {
                    logDebug("select file: User canceled");
                } else {
                    logDebug("select file failed: $0", status);
                }
            } else {
                graph->addSaveAsJsonHistory(path);
                addJsCommand(JsCommandType::GraphToJsonData, CommandArgs::copyFrom(path.c_str()));
            }
        } else {
            addJsCommand(JsCommandType::GraphToJsonData, CommandArgs::copyFrom(pathArg));
        }        
    }

}
