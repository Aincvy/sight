//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <vector>

#include "sight_node_editor.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui_canvas.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

struct LinkInfo {
    ed::LinkId Id;
    ed::PinId InputId;
    ed::PinId OutputId;
};

static ed::EditorContext *g_Context = nullptr;
static bool g_FirstFrame = true;    // Flag set for first frame only, some action need to be executed once.
static ImVector<LinkInfo> g_Links;                // List of live links. It is dynamic unless you want to create read-only view over nodes.
static int g_NextLinkId = 100;     // Counter to help generate link ids. In real application this will probably based on pointer to user data structure.

// 节点列表
static std::vector<sight::SightNode *> g_Nodes;


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


    void Application_Frame() {
        auto &io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ImGui::Separator();

        ed::SetCurrentEditor(g_Context);

        // Start interaction with editor.
        ed::Begin("My Editor", ImVec2(0.0, 0.0f));

        int uniqueId = 1;

        //
        // 1) Commit known data to editor
        //

        // Submit Node A
        ed::NodeId nodeA_Id = uniqueId++;
        ed::PinId nodeA_InputPinId = uniqueId++;
        ed::PinId nodeA_OutputPinId = uniqueId++;

        if (g_FirstFrame)
            ed::SetNodePosition(nodeA_Id, ImVec2(10, 10));
        ed::BeginNode(nodeA_Id);
        ImGui::Text("Node A");
        ed::BeginPin(nodeA_InputPinId, ed::PinKind::Input);
        ImGui::Text("-> In");
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(nodeA_OutputPinId, ed::PinKind::Output);
        ImGui::Text("Out ->");
        ed::EndPin();
        ed::EndNode();

        // Submit Node B
        ed::NodeId nodeB_Id = uniqueId++;
        ed::PinId nodeB_InputPinId1 = uniqueId++;
        ed::PinId nodeB_InputPinId2 = uniqueId++;
        ed::PinId nodeB_OutputPinId = uniqueId++;

        if (g_FirstFrame)
            ed::SetNodePosition(nodeB_Id, ImVec2(210, 60));
        ed::BeginNode(nodeB_Id);
        ImGui::Text("Node B");
        ImGuiEx_BeginColumn();
        ed::BeginPin(nodeB_InputPinId1, ed::PinKind::Input);
        ImGui::Text("-> In1");
        ed::EndPin();
        ed::BeginPin(nodeB_InputPinId2, ed::PinKind::Input);
        ImGui::Text("-> In2");
        ed::EndPin();
        ImGuiEx_NextColumn();
        ed::BeginPin(nodeB_OutputPinId, ed::PinKind::Output);
        ImGui::Text("Out ->");
        ed::EndPin();
        ImGuiEx_EndColumn();
        ed::EndNode();

        // Submit Node C
        ed::NodeId nodeC_Id = uniqueId++;
        ed::PinId nodeC_InputPinId = uniqueId++;
        ed::PinId nodeC_OutputPinId = uniqueId++;

        if (g_FirstFrame)
            ed::SetNodePosition(nodeC_Id, ImVec2(10, 120));
        ed::BeginNode(nodeC_Id);
        ImGui::Text("Node C");
        ed::BeginPin(nodeC_InputPinId, ed::PinKind::Input);
        ImGui::Text("-> In");
        if (ImGui::Button("Button")) {
            // do something
        }
        ed::EndPin();
        ImGui::SameLine();
        ed::BeginPin(nodeC_OutputPinId, ed::PinKind::Output);
        ImGui::Text("Out1 ->");
        ed::EndPin();
        ed::EndNode();

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
                        g_Links.push_back({ed::LinkId(g_NextLinkId++), inputPinId, outputPinId});

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


        // Handle deletion action
        if (ed::BeginDelete()) {
            // There may be many links marked for deletion, let's loop over them.
            ed::LinkId deletedLinkId;
            while (ed::QueryDeletedLink(&deletedLinkId)) {
                // If you agree that link can be deleted, accept deletion.
                if (ed::AcceptDeletedItem()) {
                    // Then remove link from your data.
                    for (auto &link : g_Links) {
                        if (link.Id == deletedLinkId) {
                            g_Links.erase(&link);
                            break;
                        }
                    }
                }

                // You may reject link deletion by calling:
                // ed::RejectDeletedItem();
            }
        }
        ed::EndDelete(); // Wrap up deletion action



        // End of interaction with editor.
        ed::End();

        if (g_FirstFrame)
            ed::NavigateToContent(0.0f);

        ed::SetCurrentEditor(nullptr);

        g_FirstFrame = false;

    }

    namespace {
        // private members and functions

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
            ImGui::Text("%s", node->nodeName.c_str());

            // inputPorts
            ImGuiEx_BeginColumn();
            for (const auto &item : node->inputPorts) {
                ed::BeginPin(item.id, item.kind);
                ImGui::Text("-> %s", item.portName.c_str());
                ed::EndPin();
            }

            ImGuiEx_NextColumn();
            for (const auto &item : node->outputPorts) {
                ed::BeginPin(item.id, item.kind);
                ImGui::Text("%s ->", item.portName.c_str());
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

            ed::SetCurrentEditor(g_Context);

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (const auto *node : g_Nodes) {
                showNode(node);
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
                            g_Links.push_back({ed::LinkId(g_NextLinkId++), inputPinId, outputPinId});

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
        g_Context = ed::CreateEditor(&config);
        return 0;
    }

    int destroyNodeEditor() {
        ed::DestroyEditor(g_Context);
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
        int id = 10000;

        auto *node1 = new SightNode();
        node1->nodeName = "TestNode";
        node1->nodeId = ++id;
        node1->inputPorts.push_back({
                                       "Input",
                                       ++id,
                                       ed::PinKind::Input
                               });
        node1->outputPorts.push_back({
                                       "Output",
                                       ++id,
                                       ed::PinKind::Output
                               });
        g_Nodes.push_back(node1);

        auto *node2 = new SightNode();
        node2->nodeName = "Node2";
        node2->nodeId = ++id;
        node2->inputPorts.push_back({
                                       "Input",
                                       ++id,
                                       ed::PinKind::Input
                               });
        node2->outputPorts.push_back({
                                       "Output1",
                                       ++id,
                                       ed::PinKind::Output
                               });
        node2->outputPorts.push_back({
                                       "Output2",
                                       ++id,
                                       ed::PinKind::Output
                               });
        g_Nodes.push_back(node2);

    }


}