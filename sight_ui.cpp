#include "dbg.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"
#include "sight_defines.h"
#include "sight_keybindings.h"
#include "sight_plugin.h"
#include "sight_ui.h"
#include "sight_nodes.h"
#include "sight_ui_node_editor.h"
#include "sight.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_project.h"
#include "sight_util.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sight_widgets.h"
#include "v8pp/convert.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <stdio.h>
#include <future>
#include <atomic>

#include <filesystem>

#include <GLFW/glfw3.h>
#include <string>
#include <sys/types.h>
#include <vector>

#include <absl/strings/match.h>


#define COMMON_LANGUAGE_KEYS g_UIStatus->languageKeys->commonKeys
#define WINDOW_LANGUAGE_KEYS g_UIStatus->languageKeys->windowNames
#define MENU_LANGUAGE_KEYS g_UIStatus->languageKeys->menuKeys

// todo change this
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


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
            if ((i = getCurrentGraph()->save()) != CODE_OK) {
                dbg(i);
            }
            if ((i = currentProject()->save()) != CODE_OK) {
                dbg(i);
            }
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
                        currentProject()->createGraph(path.c_str(), false);
                    }
                }
                ImGui::Separator();
                if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.entity)) {
                    activeWindowCreateEntity();
                }

                ImGui::EndMenu();
            }

            // if (ImGui::MenuItem(MENU_LANGUAGE_KEYS.open)) {
            //     dbg("open graph file");
                
            // }
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
            if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.build)) {
                // currentProject()->build();
                auto p = currentProject();
                for (const auto& item : p->getBuildTargetMap()) {
                    if (ImGui::MenuItem(item.first.c_str())) {
                        dbg(item.first);
                        addJsCommand(JsCommandType::ProjectBuild, item.first.c_str());
                    }
                }

                ImGui::EndMenu();
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
                    dbg("not open any graph");
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
                dbg("reload");
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

        void showMainCustomMenu(){
            if (ImGui::MenuItem("Trigger")) {
                // auto p = findTemplateNode("test/http/HttpGetReqNode");
                // dbg(p);
                // if (p) {
                //     dbg(serializeJsNode(*p));
                // }
                // addJsCommand(JsCommandType::Test, "/Volumes/mac_extend/Project/sight/scripts/run_in_test.js");
                currentProject()->parseAllGraphs();
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
                if (ImGui::BeginMenu(MENU_LANGUAGE_KEYS.entity)) {
                    showEntityMenu();
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
                            if (item.fileType == ProjectFileType::Graph) {
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
                if (ImGui::ArrowButton("^", ImGuiDir_Up)) {
                    createEntityData.moveItemUp();
                }
                ImGui::SameLine();
                if (ImGui::ArrowButton("v", ImGuiDir_Down)) {
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
                    if (createEntityData.edit) {
                        openAlertModal("Alert!", "Edit is not impl.");
                    } else {
                        // add entity
                        int r = addEntity(createEntityData);
                        if (r == 0) {
                            createEntityData.reset();
                            g_UIStatus->windowStatus.createEntity = false;
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button(COMMON_LANGUAGE_KEYS.cancel)) {
                    g_UIStatus->windowStatus.createEntity = false;
                }
                if (createEntityData.edit) {
                    // In edit mode
                    ImGui::SameLine();
                    ImGui::Text("Edit Mode");
                }
                ImGui::End();
            }
        }

        void showEntityListWindow(){
            if (ImGui::Begin("Entity List", &g_UIStatus->windowStatus.entityListWindow)) {
                ImGui::InputText("Filter", g_UIStatus->buffer.entityListSearch, std::size(g_UIStatus->buffer.entityListSearch));
                ImGui::SameLine(ImGui::GetWindowWidth() - 60);
                
                if (ImGui::Button("Create")) {
                    dbg(1);
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

                    ImGui::Text("%8s", info.simpleName.c_str());
                    ImGui::SameLine();
                    ImGui::Text("%18s", name.c_str());
                    ImGui::SameLine();
                    // ImGui::SetNextItemWidth(float item_width)
                    // maybe use google icon fonts will be better.
                    std::string editButtonLabel{"Edit##edit-"};
                    editButtonLabel += name;
                    if (ImGui::Button(editButtonLabel.c_str())) {
                        // edit
                        auto& createEntityData = g_UIStatus->createEntityData;
                        createEntityData.loadFrom(info);
                        createEntityData.edit = true;

                        activeWindowCreateEntity();
                    }
                    ImGui::SameLine();
                    std::string delButtonLabel{ "Del##del-" };
                    delButtonLabel += name;
                    if (ImGui::Button(delButtonLabel.c_str())) {
                        delFullName = name;
                    }
                }

                if (!delFullName.empty()) {
                    // check if used.
                    std::string pathOut;
                    if (p->isAnyGraphHasTemplate(delFullName, &pathOut)) {
                        // used, cannot be deleted.
                        std::string contentString = { "Used in graph: " };
                        contentString += pathOut;
                        openAlertModal("Alert! Cannot be deleted!", contentString);
                    } else {
                        // delete ask
                        std::string str{ "! You only can delete the NOT-USED entity !\nYou will delete: " };
                        str += delFullName;
                        openAskModal("Delete entity", str.c_str(), [p, delFullName](bool f) {
                            dbg(f);
                            if (f) {
                                p->delEntity(delFullName);
                            }
                        });
                    }
                }
                ImGui::End();
            }
        }

        void showAboutWindow(){
            if (ImGui::Begin(WINDOW_LANGUAGE_KEYS.about, &g_UIStatus->windowStatus.aboutWindow)) {
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
                        auto value = map[key];
                        ImGui::Text("%s", key.c_str());
                        ImGui::Text("%7s: %s", "Author", value->getAuthor());
                        ImGui::Text("%7s: %s", "Version", value->getVersion());
                        ImGui::Text("%7s: %s", "Path", value->getPath());
                        ImGui::Text("%7s: %s", "Status", value->getPluginStatusString());
                        
                        constexpr const char* reloadLabel = "Reload";
                        std::string labelBuf = reloadLabel;
                        labelBuf += "##";
                        labelBuf += key;

                        if (ImGui::Button(labelBuf.c_str())) {
                            // send reload 
                            addJsCommand(JsCommandType::PluginReload, strdup(key.c_str()), key.length(), true);
                        }
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
                    dbg(g_UIStatus->buffer.littleName);
                    currentProject()->createGraph(g_UIStatus->buffer.littleName);
                    g_UIStatus->buffer.littleName[0] = '\0';
                }
                ImGui::CloseCurrentPopup();
            }

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
                auto isolate = g_UIStatus->isolate;
                auto module = v8::Object::New(isolate);
                if (runJsFile(isolate, command->args.argString, nullptr, module) == CODE_OK) {
                    // 
                    dbg("ok");
                    registerToGlobal(isolate, module->Get(isolate->GetCurrentContext(), v8pp::to_v8(isolate, "globals")).ToLocalChecked());
                };

                break;
            }

            }

        command->args.dispose();
        uiCommandFree = true;
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

        // init ui v8
        dbg("init ui v8 Isolate");
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
        context->Global()->Set(context, v8pp::to_v8(isolate, "sight"), module.new_instance()).ToChecked();

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

        // create js context
        auto isolate = g_UIStatus->isolate;
        v8::Isolate::Scope isolateScope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = g_UIStatus->v8GlobalContext.Get(isolate);
        v8::Context::Scope contextScope(context);
        dbg("v8 runtime init over.");

        // Create window with graphics context
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        GLFWwindow* window = glfwCreateWindow(1600, 900, "sight - a code generate tool", nullptr, NULL);
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
            ImGui::SetCurrentFont(fontPointer);
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
        destroyNodeEditor(true, true);
        glfwTerminate();

        g_UIStatus->v8GlobalContext.Reset();
        delete g_UIStatus->arrayBufferAllocator;
        g_UIStatus->arrayBufferAllocator = nullptr;
        g_UIStatus->isolate->Dispose();
        g_UIStatus->isolate = nullptr;
    
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
                    ImGui::Text("%7s: ", item.portName.c_str());
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
        // if (!g) {
        //     return;
        // }

        auto openGraphFunc = [path]() {
            char tmp[NAME_BUF_SIZE]{0};
            auto t = currentProject()->openGraph(path, false, tmp);
            if (t) {
                // open succeed.
                changeNodeEditorGraph(tmp);
            }
        };
        if (g == nullptr || !g->editing) {
            // 
            // currentProject()->openGraph(path, false);
            openGraphFunc();
            return;
        }

        std::string stringPath(path);
        openSaveModal("Save File?", "Current Graph do not save, do you want save file?", [stringPath, openGraphFunc,g](SaveOperationResult r) {
            if (r == SaveOperationResult::Cancel) {
                return;
            } else if (r == SaveOperationResult::Save) {
                g->save();
            }
            openGraphFunc();

            // if (r == SaveOperationResult::Drop) {
            //     currentProject()->openGraph(stringPath.c_str(), false);
            // } else if (r == SaveOperationResult::Save) {
            //     getCurrentGraph()->save();
            //     currentProject()->openGraph(stringPath.c_str(), false);
            // }
        });
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
        snprintf(this->name, std::size(this->name), "%s", info.simpleName.c_str());
        snprintf(this->templateAddress, std::size(this->templateAddress), "%s", info.templateAddress.c_str());

        for( const auto& item: info.fields){
            addField();

            auto p = lastField();
            snprintf(p->name, std::size(p->name), "%s", item.name.c_str());
            snprintf(p->type, std::size(p->type), "%s", item.type.c_str());
            snprintf(p->defaultValue, std::size(p->defaultValue), "%s", item.defaultValue.c_str());
        }
    }

    void UICreateEntity::reset() {
        edit = false;
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
