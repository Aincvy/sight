//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <vector>
#include <atomic>
#include <algorithm>

#include "sight_node_editor.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_canvas.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

#include "dbg.h"

#define BACKGROUND_CONTEXT_MENU "BackgroundContextMenu"
#define LINK_CONTEXT_MENU "LinkContextMenu"
#define PIN_CONTEXT_MENU "PinContextMenu"
#define NODE_CONTEXT_MENU "NodeContextMenu"

#define CURRENT_GRAPH g_NodeEditorStatus.graph

namespace ed = ax::NodeEditor;

struct LinkInfo {
    ed::LinkId Id;
    ed::PinId InputId;
    ed::PinId OutputId;
};

static ImVector<LinkInfo> g_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.

// node list
// todo clear data
// static std::vector<sight::SightNode*> g_Nodes;
//
static std::atomic<int> nodeOrPortId(10000);
// node editor status
static sight::NodeEditorStatus g_NodeEditorStatus;

namespace sight {

    void ImGuiEx_BeginColumn() {
        ImGui::BeginGroup();
    }

    void ImGuiEx_NextColumn() {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    void ImGuiEx_EndColumn() {
        ImGui::EndGroup();
    }

    namespace {
        // private members and functions

        void showContextMenu(const ImVec2& openPopupPosition){
            ed::Suspend();

            if (ImGui::BeginPopup(NODE_CONTEXT_MENU)) {
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(PIN_CONTEXT_MENU)) {
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(LINK_CONTEXT_MENU)) {
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(BACKGROUND_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                if (ImGui::BeginMenu("class a")) {
                    if (ImGui::MenuItem("1")) {
                        dbg("click `class a/1`");
                    }

                    if (ImGui::MenuItem("2")) {
                        dbg("click `class a/2`");
                    }

                    ImGui::EndMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("custom operations")) {
                    dbg("custom operations");
                }

                ImGui::EndPopup();
            }

            ed::Resume();
        }

        void showNodePortIcon(ImColor color, bool isConnected = false){
            auto cursorPos = ImGui::GetCursorScreenPos();
            auto drawList  = ImGui::GetWindowDrawList();
            const int iconSize = 20;
            const ImVec2 size(iconSize, iconSize);
            auto rect = ImRect(cursorPos, cursorPos + size);
            auto rect_x         = rect.Min.x;
            auto rect_y         = rect.Min.y;
            auto rect_w         = rect.Max.x - rect.Min.x;
            auto rect_h         = rect.Max.y - rect.Min.y;
            auto rect_center_x  = (rect.Min.x + rect.Max.x) * 0.5f;
            auto rect_center_y  = (rect.Min.y + rect.Max.y) * 0.5f;
            auto rect_center    = ImVec2(rect_center_x, rect_center_y);
            const auto outline_scale  = rect_w / 24.0f;
            const auto extra_segments = static_cast<int>(2 * outline_scale); // for full circle
            // auto color = ImColor(0,99,160,255);
            const auto c = rect_center;
            auto triangleStart = rect_center_x + 0.32f * rect_w;

            drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);

            const auto triangleTip = triangleStart + rect_w * (0.45f - 0.32f);

            drawList->AddTriangleFilled(
                    ImVec2(ceilf(triangleTip), rect_y + rect_h * 0.5f),
                    ImVec2(triangleStart, rect_center_y + 0.15f * rect_h),
                    ImVec2(triangleStart, rect_center_y - 0.15f * rect_h),
                    color);

            ImGui::Dummy(size);
        }

        /**
         * render a node
         * @param node
         * @return
         */
        int showNode(const SightNode *node) {
            if (!node) {
                return -1;
            }

            ed::BeginNode(node->nodeId);
            ImGui::Dummy(ImVec2(7,5));
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0,106,113,255),"%s", node->nodeName.c_str());
            // ImGui::Separator();
            ImGui::Dummy(ImVec2(0,3));

            auto color = ImColor(0,99,160,255);
            // inputPorts
            ImGuiEx_BeginColumn();
            for (const auto &item : node->inputPorts) {
                ed::BeginPin(item.id, ed::PinKind::Input);
                ed::PinPivotAlignment(ImVec2(0, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                showNodePortIcon(color);
                ImGui::SameLine();
                ImGui::Text("%s", item.portName.c_str());
                ed::EndPin();
            }

            ImGuiEx_NextColumn();
            for (const auto &item : node->outputPorts) {
                ed::BeginPin(item.id, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                ImGui::Text("%s", item.portName.c_str());
                ImGui::SameLine();
                showNodePortIcon(color);
                ed::EndPin();
            }

            ImGuiEx_EndColumn();
            ed::EndNode();
            return 0;
        }

        int showNodes(const UIStatus &uiStatus) {
            auto &io = uiStatus.io;

            ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);
            ImGui::Separator();

            ed::SetCurrentEditor(g_NodeEditorStatus.context);

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (const auto &node : CURRENT_GRAPH->nodes) {
                showNode(&node);
            }

            // Submit Links
            for (auto &linkInfo : g_Links)
                ed::Link(linkInfo.Id, linkInfo.InputId, linkInfo.OutputId);

            //
            // 2) Handle interactions
            //

            // Handle creation action, returns true if editor want to create new object (node or link)
            if (ed::BeginCreate()) {
                ed::PinId inputPinId, outputPinId;
                if (ed::QueryNewLink(&inputPinId, &outputPinId)) {
                    // QueryNewLink returns true if editor want to create new link between pins.
                    //
                    // Link can be created only for two valid pins, it is up to you to
                    // validate if connection make sense. Editor is happy to make any.
                    //
                    // Link always goes from input to output. User may choose to drag
                    // link from output pin or input pin. This determine which pin ids
                    // are valid and which are not:
                    //   * input valid, output invalid - user started to drag new ling from input pin
                    //   * input invalid, output valid - user started to drag new ling from output pin
                    //   * input valid, output valid   - user dragged link over other pin, can be validated

                    if (inputPinId && outputPinId) // both are valid, let's accept link
                    {
                        // ed::AcceptNewItem() return true when user release mouse button.
                        if (ed::AcceptNewItem()) {
                            // Since we accepted new link, lets add one to our list of links.
                            g_Links.push_back({ed::LinkId(nextNodeOrPortId()), inputPinId, outputPinId});


                            // Draw new link.
                            ed::Link(g_Links.back().Id, g_Links.back().InputId, g_Links.back().OutputId);
                        }

                        // You may choose to reject connection between these nodes
                        // by calling ed::RejectNewItem(). This will allow editor to give
                        // visual feedback by changing link thickness and color.
                    }
                }
            }
            ed::EndCreate(); // Wraps up object creation action handling.

            auto openPopupPosition = ImGui::GetMousePos();
            ed::Suspend();
            static ed::NodeId contextNodeId      = 0;
            static ed::LinkId contextLinkId      = 0;
            static ed::PinId  contextPinId       = 0;
            if (ed::ShowNodeContextMenu(&contextNodeId)) {
                dbg("ShowNodeContextMenu");
                ImGui::OpenPopup(NODE_CONTEXT_MENU);
            } else if (ed::ShowPinContextMenu(&contextPinId)) {
                dbg("ShowPinContextMenu");
                ImGui::OpenPopup(PIN_CONTEXT_MENU);
            }
            else if (ed::ShowLinkContextMenu(&contextLinkId)) {
                dbg("ShowLinkContextMenu");
                ImGui::OpenPopup(LINK_CONTEXT_MENU);
            }
            else if (ed::ShowBackgroundContextMenu())
            {
                dbg("ShowBackgroundContextMenu");
                ImGui::OpenPopup(BACKGROUND_CONTEXT_MENU);
            }
            ed::Resume();

            showContextMenu(openPopupPosition);
            // End of interaction with editor.
            ed::End();

            if (uiStatus.needInit)
                ed::NavigateToContent(0.0f);

            ed::SetCurrentEditor(nullptr);

            return 0;
        }

    }


    int initNodeEditor() {
        ed::Config config;
        config.SettingsFile = "Simple.json";
        g_NodeEditorStatus.context = ed::CreateEditor(&config);
        g_NodeEditorStatus.createGraph("./path/to/somewhere");

        initTestData();
        return 0;
    }

    int destroyNodeEditor() {
        ed::DestroyEditor(g_NodeEditorStatus.context);
        return 0;
    }

    int showNodeEditorGraph(const UIStatus &uiStatus) {
        if (uiStatus.needInit) {
            ImVec2 startPos = {
                    300, 20
            };
            auto windowSize = uiStatus.io.DisplaySize - startPos;
            ImGui::SetNextWindowPos(startPos);
            ImGui::SetNextWindowSize(windowSize);
        }

        ImGui::Begin("Node Editor Graph", nullptr,
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoSavedSettings);
        showNodes(uiStatus);

        ImGui::End();

        return 0;
    }

    void initTestData() {

        SightNode sightNode1;
        auto node1 = &sightNode1;
        node1->nodeName = "TestNode";
        node1->nodeId = nextNodeOrPortId();
        node1->inputPorts.push_back({
                                            "Input",
                                            nextNodeOrPortId(),
                                            NodePortType::Input
                               });
        node1->outputPorts.push_back({
                                             "Output",
                                             nextNodeOrPortId(),
                                             NodePortType::Output
                               });
        CURRENT_GRAPH->nodes.push_back(sightNode1);
        // g_Nodes.push_back(node1);

        SightNode sightNode2;
        auto *node2 = &sightNode2;
        node2->nodeName = "Node2";
        node2->nodeId = nextNodeOrPortId();
        node2->inputPorts.push_back({
                                            "Input",
                                            nextNodeOrPortId(),
                                            NodePortType::Input
                               });
        node2->outputPorts.push_back({
                                             "Output1",
                                             nextNodeOrPortId(),
                                             NodePortType::Output
                               });
        node2->outputPorts.push_back({
                                             "Output2",
                                             nextNodeOrPortId(),
                                             NodePortType::Output
                               });
        //g_Nodes.push_back(node2);
        CURRENT_GRAPH->nodes.push_back(sightNode2);

    }

    int nextNodeOrPortId() {
        return nodeOrPortId++;
    }

    int addNode(SightNode *node) {
        //g_Nodes.push_back(node);
        CURRENT_GRAPH->addNode(*node);
        delete node;
        return 0;
    }


    void SightNodePort::setKind(int intKind) {
        this->kind = NodePortType(intKind);
    }

    bool SightNodePort::isConnect() const {
        return !connections.empty();
    }

    int SightNodePort::connectionsSize() const {
        return static_cast<int>(connections.size());
    }


    void SightNode::addPort(SightNodePort &port) {
        dbg("add port, id: ", port.id ,", name: ",  port.portName);

        if (port.kind == NodePortType::Input) {
            this->inputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->outputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            // todo
        }
    }

    SightNode *SightNode::clone() const{
        auto p = new SightNode();
        p->copyFrom(this);
        return p;
    }

    SightNode *SightNode::instantiate() {
        if (this->templateNode) {
            return this->templateNode->instantiate();
        }

        auto p = new SightNode();
        p->templateNode = this;
        p->copyFrom(this);

        // generate id
        p->nodeId = nextNodeOrPortId();
        for (auto &item : p->inputPorts) {
            item.id = nextNodeOrPortId();
        }
        for (auto &item : p->outputPorts) {
            item.id = nextNodeOrPortId();
        }

        return p;
    }

    void SightNode::copyFrom(const SightNode *node) {
        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        this->inputPorts.assign(node->inputPorts.begin(), node->inputPorts.end());
        this->outputPorts.assign(node->outputPorts.begin(), node->outputPorts.end());
    }


    void SightJsNode::callFunction(const char *name) {

    }

    void SightNodeConnection::removeRefs() {
        if (left) {
            auto & c = left->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
        if (right) {
            auto & c = right->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
    }

    int SightNodeGraph::saveToFile(const char *path, bool set) {
        // todo save file

        if (set) {
            this->setFilePath(path);
        }
        return 0;
    }

    void SightNodeGraph::dispose() {

    }

    const char *SightNodeGraph::getFilePath() const {
        return this->filepath.c_str();
    }

    void SightNodeGraph::setFilePath(const char* path) {
        this->filepath = path;
    }

    void SightNodeGraph::addNode(const SightNode &node) {
        this->nodes.push_back(node);
    }

    int NodeEditorStatus::createGraph(const char *path) {
        if (graph) {
            return -1;     // has one.
        }

        graph = new SightNodeGraph();
        graph->saveToFile(path, true);
        return 0;
    }

}