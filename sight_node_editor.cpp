//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <atomic>
#include <algorithm>
#include <fstream>

#include "sight_defines.h"
#include "dbg.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_js.h"
#include "sight_node_editor.h"
#include "sight_address.h"
#include "sight_ui.h"
#include "sight_util.h"
#include "sight_project.h"
#include "sight_keybindings.h"

#include "imgui.h"
#include "sight_widgets.h"
#include "v8pp/convert.hpp"
#include "v8pp/class.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_canvas.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"

#include "yaml-cpp/yaml.h"

#define BACKGROUND_CONTEXT_MENU "BackgroundContextMenu"
#define LINK_CONTEXT_MENU "LinkContextMenu"
#define PIN_CONTEXT_MENU "PinContextMenu"
#define NODE_CONTEXT_MENU "NodeContextMenu"

#define CURRENT_GRAPH g_NodeEditorStatus->graph

namespace ed = ax::NodeEditor;

// node editor status
static sight::NodeEditorStatus* g_NodeEditorStatus;

namespace sight {

    // define SightNodeGraph::invalidAnyThingWrapper
    const SightAnyThingWrapper SightNodeGraph::invalidAnyThingWrapper = {
            SightAnyThingType::Invalid,
            static_cast<SightNode*>(nullptr),
    };

    namespace {
        // private members and functions
        
        void onConnect(SightNodeConnection* connection){
            if (!connection) {
                return;
            }

            // notify left, right
            auto left = connection->findLeftPort();
            auto right = connection->findRightPort();
            auto isolate = currentUIStatus()->isolate;

            if (left->templateNodePort) {
                left->templateNodePort->onConnect(isolate, left, JsEventType::Connect, connection);
            }
            if (right->templateNodePort) {
                right->templateNodePort->onConnect(isolate, right, JsEventType::Connect, connection);
            }
        }

        /**
         * @brief Call before real delete.
         * 
         * @param connection 
         */
        void OnDisconnect(SightNodeConnection* connection){
            if (!connection) {
                return;
            }

            // notify left, right
            auto left = connection->findLeftPort();
            auto right = connection->findRightPort();
            auto isolate = currentUIStatus()->isolate;

            if (left->templateNodePort) {
                left->templateNodePort->onDisconnect(isolate, left, JsEventType::Connect, connection);
            }
            if (right->templateNodePort) {
                right->templateNodePort->onDisconnect(isolate, right, JsEventType::Connect, connection);
            }
        }

        // node editor functions
        void showContextMenu(const ImVec2& openPopupPosition, uint nodeId, uint linkId, uint pinId){
            ed::Suspend();

            if (ImGui::BeginPopup(NODE_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                if (ImGui::MenuItem("debugInfo")) {
                    // show debug info.
                    dbg(nodeId);
                    auto nodePointer = CURRENT_GRAPH->findNode(nodeId);
                    dbg(nodePointer);
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(PIN_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(LINK_CONTEXT_MENU)) {
                if (ImGui::MenuItem("itemA")) {
                    dbg("item a");
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(BACKGROUND_CONTEXT_MENU)) {
                for (auto &item : g_NodeEditorStatus->templateAddressList) {
                    item.showContextMenu(openPopupPosition);
                }

                ImGui::Separator();
                if (ImGui::MenuItem("custom operations")) {
                    dbg("custom operations");
                }

                ImGui::EndPopup();
            }

            ed::Resume();
        }

        void showNodePortIcon(IconType iconType, ImColor color, bool isConnected = false) {
            auto cursorPos = ImGui::GetCursorScreenPos();
            auto drawList = ImGui::GetWindowDrawList();
            const int iconSize = 20;
            const ImVec2 size(iconSize, iconSize);
            auto rect = ImRect(cursorPos, cursorPos + size);
            auto rect_x = rect.Min.x;
            auto rect_y = rect.Min.y;
            auto rect_w = rect.Max.x - rect.Min.x;
            auto rect_h = rect.Max.y - rect.Min.y;
            auto rect_center_x = (rect.Min.x + rect.Max.x) * 0.5f;
            auto rect_center_y = (rect.Min.y + rect.Max.y) * 0.5f;
            auto rect_center = ImVec2(rect_center_x, rect_center_y);
            const auto outline_scale = rect_w / 24.0f;
            const auto extra_segments = static_cast<int>(2 * outline_scale);     // for full circle
            auto alpha = 255;
            auto innerColor = ImColor(32, 32, 32, alpha);

            
            if (iconType == IconType::Flow) {
                const auto origin_scale = rect_w / 24.0f;

                const auto offset_x = 1.0f * origin_scale;
                const auto offset_y = 0.0f * origin_scale;
                const auto margin = (isConnected ? 2.0f : 2.0f) * origin_scale;
                const auto rounding = 0.1f * origin_scale;
                const auto tip_round = 0.7f;     // percentage of triangle edge (for tip)
                //const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
                const auto canvas = ImRect(
                    rect.Min.x + margin + offset_x,
                    rect.Min.y + margin + offset_y,
                    rect.Max.x - margin + offset_x,
                    rect.Max.y - margin + offset_y);
                const auto canvas_x = canvas.Min.x;
                const auto canvas_y = canvas.Min.y;
                const auto canvas_w = canvas.Max.x - canvas.Min.x;
                const auto canvas_h = canvas.Max.y - canvas.Min.y;

                const auto left = canvas_x + canvas_w * 0.5f * 0.3f;
                const auto right = canvas_x + canvas_w - canvas_w * 0.5f * 0.3f;
                const auto top = canvas_y + canvas_h * 0.5f * 0.2f;
                const auto bottom = canvas_y + canvas_h - canvas_h * 0.5f * 0.2f;
                const auto center_y = (top + bottom) * 0.5f;
                //const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

                const auto tip_top = ImVec2(canvas_x + canvas_w * 0.5f, top);
                const auto tip_right = ImVec2(right, center_y);
                const auto tip_bottom = ImVec2(canvas_x + canvas_w * 0.5f, bottom);

                drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
                drawList->PathBezierCurveTo(
                    ImVec2(left, top),
                    ImVec2(left, top),
                    ImVec2(left, top) + ImVec2(rounding, 0));
                drawList->PathLineTo(tip_top);
                drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
                drawList->PathBezierCurveTo(
                    tip_right,
                    tip_right,
                    tip_bottom + (tip_right - tip_bottom) * tip_round);
                drawList->PathLineTo(tip_bottom);
                drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
                drawList->PathBezierCurveTo(
                    ImVec2(left, bottom),
                    ImVec2(left, bottom),
                    ImVec2(left, bottom) - ImVec2(0, rounding));

                if (!isConnected) {
                    if (innerColor & 0xFF000000)
                        drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

                    drawList->PathStroke(color, true, 2.0f * outline_scale);
                } else
                    drawList->PathFillConvex(color);
                
            } else {
                bool drawTriangle = true;
                auto triangleStart = rect_center_x + 0.32f * rect_w;
                auto rect_offset = -static_cast<int>(rect_w * 0.25f * 0.25f);

                rect.Min.x += rect_offset;
                rect.Max.x += rect_offset;
                rect_x += rect_offset;
                rect_center_x += rect_offset * 0.5f;
                rect_center.x += rect_offset * 0.5f;

                switch (iconType) {
                case IconType::Square:
                {
                    if (isConnected) {
                        const auto r = 0.5f * rect_w / 2.0f;
                        const auto p0 = rect_center - ImVec2(r, r);
                        const auto p1 = rect_center + ImVec2(r, r);

                        drawList->AddRectFilled(p0, p1, color, 0, 15 + extra_segments);
                    } else {
                        const auto r = 0.5f * rect_w / 2.0f - 0.5f;
                        const auto p0 = rect_center - ImVec2(r, r);
                        const auto p1 = rect_center + ImVec2(r, r);

                        if (innerColor & 0xFF000000)
                            drawList->AddRectFilled(p0, p1, innerColor, 0, 15 + extra_segments);

                        drawList->AddRect(p0, p1, color, 0, 15 + extra_segments, 2.0f * outline_scale);
                    }
                    break;
                }
                case IconType::Grid:
                {
                    const auto r = 0.5f * rect_w / 2.0f;
                    const auto w = ceilf(r / 3.0f);

                    const auto baseTl = ImVec2(floorf(rect_center_x - w * 2.5f), floorf(rect_center_y - w * 2.5f));
                    const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

                    auto tl = baseTl;
                    auto br = baseBr;
                    for (int i = 0; i < 3; ++i) {
                        tl.x = baseTl.x;
                        br.x = baseBr.x;
                        drawList->AddRectFilled(tl, br, color);
                        tl.x += w * 2;
                        br.x += w * 2;
                        if (i != 1 || isConnected)
                            drawList->AddRectFilled(tl, br, color);
                        tl.x += w * 2;
                        br.x += w * 2;
                        drawList->AddRectFilled(tl, br, color);

                        tl.y += w * 2;
                        br.y += w * 2;
                    }

                    triangleStart = br.x + w + 1.0f / 24.0f * rect_w;
                    break;
                }
                case IconType::RoundSquare:
                {
                    drawTriangle = false;
                    if (isConnected) {
                        const auto r = 0.5f * rect_w / 2.0f;
                        const auto cr = r * 0.5f;
                        const auto p0 = rect_center - ImVec2(r, r);
                        const auto p1 = rect_center + ImVec2(r, r);

                        drawList->AddRectFilled(p0, p1, color, cr, 15);
                    } else {
                        const auto r = 0.5f * rect_w / 2.0f - 0.5f;
                        const auto cr = r * 0.5f;
                        const auto p0 = rect_center - ImVec2(r, r);
                        const auto p1 = rect_center + ImVec2(r, r);

                        if (innerColor & 0xFF000000)
                            drawList->AddRectFilled(p0, p1, innerColor, cr, 15);

                        drawList->AddRect(p0, p1, color, cr, 15, 2.0f * outline_scale);
                    }
                    break;
                }
                case IconType::Diamond:
                {
                    drawTriangle = false;
                    if (isConnected) {
                        const auto r = 0.607f * rect_w / 2.0f;
                        const auto c = rect_center;

                        drawList->PathLineTo(c + ImVec2(0, -r));
                        drawList->PathLineTo(c + ImVec2(r, 0));
                        drawList->PathLineTo(c + ImVec2(0, r));
                        drawList->PathLineTo(c + ImVec2(-r, 0));
                        drawList->PathFillConvex(color);
                    } else {
                        const auto r = 0.607f * rect_w / 2.0f - 0.5f;
                        const auto c = rect_center;

                        drawList->PathLineTo(c + ImVec2(0, -r));
                        drawList->PathLineTo(c + ImVec2(r, 0));
                        drawList->PathLineTo(c + ImVec2(0, r));
                        drawList->PathLineTo(c + ImVec2(-r, 0));

                        if (innerColor & 0xFF000000)
                            drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

                        drawList->PathStroke(color, true, 2.0f * outline_scale);
                    }
                    break;
                }
                case IconType::Circle:
                default:
                {
                    const auto c = rect_center;

                    if (!isConnected) {
                        const auto r = 0.5f * rect_w / 2.0f - 0.5f;

                        if (innerColor & 0xFF000000)
                            drawList->AddCircleFilled(c, r, innerColor, 12 + extra_segments);
                        drawList->AddCircle(c, r, color, 12 + extra_segments, 2.0f * outline_scale);
                    } else
                        drawList->AddCircleFilled(c, 0.5f * rect_w / 2.0f, color, 12 + extra_segments);
                    break;
                }
                }

                if (drawTriangle) {
                    const auto triangleTip = triangleStart + rect_w * (0.45f - 0.32f);
                    drawList->AddTriangleFilled(
                        ImVec2(ceilf(triangleTip), rect_y + rect_h * 0.5f),
                        ImVec2(triangleStart, rect_center_y + 0.15f * rect_h),
                        ImVec2(triangleStart, rect_center_y - 0.15f * rect_h),
                        color);
                }
            }

            ImGui::Dummy(size);
        }

        /**
         * render a node
         * @param node
         * @return
         */
        int showNode(SightNode *node) {
            if (!node) {
                return -1;
            }

            auto color = ImColor(0,99,160,255);
            auto project = currentProject();
            auto [typeProcess, _] = project->findTypeInfo(IntTypeProcess);
            // dbg(typeProcess.name, typeProcess.intValue, typeProcess.style);

            ed::BeginNode(node->nodeId);
            ImGui::Dummy(ImVec2(7,5));
            auto chainInPort = node->chainInPort;
            if (chainInPort) {
                ed::BeginPin(chainInPort->id, ed::PinKind::Input);
                showNodePortIcon(typeProcess.style->iconType, typeProcess.style->color, chainInPort->isConnect());
                ed::EndPin();
                ImGui::SameLine();
            }

            ImGui::TextColored(ImVec4(0,106,113,255),"%s", node->nodeName.c_str());

            auto chainOutPort = node->chainOutPort;
            if (chainOutPort) {
                ImGui::SameLine(215);
                ed::BeginPin(chainOutPort->id, ed::PinKind::Output);
                showNodePortIcon(typeProcess.style->iconType, typeProcess.style->color, chainOutPort->isConnect());
                ed::EndPin();
            }
            ImGui::Dummy(ImVec2(0,3));

            // fields
            for (auto &item: node->fields) {
                if (!item.options.show) {
                    continue;
                }

                auto & options = item.options;
                bool showErrorMsg = false;
                if (options.errorMsg.empty()) {
                    ImGui::Text("%8s", item.portName.c_str());
                } else {
                    ImGui::TextColored(currentUIStatus()->uiColors->errorText, "%8s", item.portName.c_str());
                    if (ImGui::IsItemHovered()) {
                        // show error msg
                        showErrorMsg = true;
                    }
                }
                ImGui::SameLine();
                showNodePortValue(&item, true);

                if (showErrorMsg) {
                    ImGui::TextColored(currentUIStatus()->uiColors->errorText, "%8s", options.errorMsg.c_str());
                }
            }

            // inputPorts
            ImGui::BeginGroup();
            bool hasInputShow = false;
            for (auto& item : node->inputPorts) {
                if (item.portName.empty() || !item.options.show) {
                    continue;     // do not show the chain port. (Process port)
                }

                ed::BeginPin(item.id, ed::PinKind::Input);
                ed::PinPivotAlignment(ImVec2(0, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));

                // port icon
                // use fake type here
                auto [tmpTypeInfo, find] = project->findTypeInfo(item.type);
                if (!find || !tmpTypeInfo.style) {
                    showNodePortIcon(IconType::Circle, color, item.isConnect());
                } else {
                    showNodePortIcon(tmpTypeInfo.style->iconType, tmpTypeInfo.style->color, item.isConnect());
                }

                ImGui::SameLine();
                ImGui::Text("%s", item.portName.c_str());
                ed::EndPin();

                hasInputShow = true;
            }
            if (!hasInputShow) {
                ImGui::Dummy(ImVec2(35, 20));
            }

            ImGui::EndGroup();
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (SightNodePort &item : node->outputPorts) {
                if (item.portName.empty() || !item.options.show) {
                    continue;       // do not show the chain port. (Process port)
                }

                // test value
                if (item.type == IntTypeProcess || !item.options.showValue){

                } else {
                    showNodePortValue(&item, true);
                    ImGui::SameLine();
                }

                ed::BeginPin(item.id, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                ImGui::Text("%8s", item.portName.c_str());
                ImGui::SameLine();

                // port icon  
                // use fake type here
                auto [tmpTypeInfo, find] = project->findTypeInfo(item.type);
                if (!find || !tmpTypeInfo.style) {
                    showNodePortIcon(IconType::Circle, color, item.isConnect());
                } else {
                    showNodePortIcon(tmpTypeInfo.style->iconType, tmpTypeInfo.style->color, item.isConnect());
                }
                // ImGui::Dummy(ImVec2(20,20));
                // ImGui::Text("new line?");

                ed::EndPin();
            }

            ImGui::EndGroup();
            ed::EndNode();
            return 0;
        }

        int showNodes(UIStatus const& uiStatus) {
            auto io = uiStatus.io;

            ImGui::Text("FPS: %.2f (%.2gms)", io->Framerate, io->Framerate ? 1000.0f / io->Framerate : 0.0f);
            ImGui::SameLine();
            if (ImGui::Button("test## 133")) {
                dbg("test");
            }
            ImGui::Separator();

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (auto &node : CURRENT_GRAPH->getNodes()) {
                showNode(&node);
            }

            // Submit Links
            for (const auto &connection : CURRENT_GRAPH->getConnections()) {
                ed::Link(connection.connectionId, connection.leftPortId(), connection.rightPortId(), ImColor(connection.leftColor));
            }

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
                            // g_Links.push_back({ed::LinkId(nextNodeOrPortId()), inputPinId, outputPinId});
                            auto id = CURRENT_GRAPH->createConnection(static_cast<int>(inputPinId.Get()), static_cast<int>(outputPinId.Get()));
                            if (id > 0) {
                                // valid connection.
                                auto connection = CURRENT_GRAPH->findConnection(id);
                                onConnect(connection);

                                // Draw new link.
                                ed::Link(id, inputPinId, outputPinId);
                            } else {
                                // may show something to user.
                            }
                        }

                        // You may choose to reject connection between these nodes
                        // by calling ed::RejectNewItem(). This will allow editor to give
                        // visual feedback by changing link thickness and color.
                    }
                }
            }
            ed::EndCreate(); // Wraps up object creation action handling.

            // Handle deletion action
            if (ed::BeginDelete())
            {
                ed::NodeId deletedNodeId;
                while (ed::QueryDeletedNode(&deletedNodeId)) {
                    // ask for node delete.
                    if (ed::AcceptDeletedItem()) {
                        CURRENT_GRAPH->delNode(static_cast<int>(deletedNodeId.Get()));
                    }
                }

                // There may be many links marked for deletion, let's loop over them.
                ed::LinkId deletedLinkId;
                while (ed::QueryDeletedLink(&deletedLinkId))
                {
                    // If you agree that link can be deleted, accept deletion.
                    if (ed::AcceptDeletedItem())
                    {
                        // Then remove link from your data.
                        auto connectionId = static_cast<int>(deletedLinkId.Get());
                        auto connection = CURRENT_GRAPH->findConnection(connectionId);
                        OnDisconnect(connection);
                        CURRENT_GRAPH->delConnection(connectionId);
                    }

                    // You may reject link deletion by calling:
                    // ed::RejectDeletedItem();
                }
            }
            ed::EndDelete(); // Wrap up deletion action
            
            // handle keyboard
            if (uiStatus.keybindings->controlKey.isKeyDown()) {
                if (uiStatus.keybindings->duplicateNode) {
                    dbg("duplicate node");
                    auto tmpNode = uiStatus.selection.node; 
                    if (tmpNode && tmpNode->templateNode) {
                        auto node = tmpNode->templateNode->instantiate();
                        auto pos = ed::GetNodePosition(tmpNode->nodeId);
                        auto size = ed::GetNodeSize(tmpNode->nodeId);
                        ed::SetNodePosition(node->nodeId, ImVec2(pos.x, pos.y + size.y + 10));
                        addNode(node);
                    }
                }
            }

            auto openPopupPosition = ImGui::GetMousePos();
            ed::Suspend();
            static ed::NodeId contextNodeId      = 0;
            static ed::LinkId contextLinkId      = 0;
            static ed::PinId  contextPinId       = 0;
            if (ed::ShowNodeContextMenu(&contextNodeId)) {
                ImGui::OpenPopup(NODE_CONTEXT_MENU);
            } else if (ed::ShowPinContextMenu(&contextPinId)) {
                ImGui::OpenPopup(PIN_CONTEXT_MENU);
            }
            else if (ed::ShowLinkContextMenu(&contextLinkId)) {
                ImGui::OpenPopup(LINK_CONTEXT_MENU);
            }
            else if (ed::ShowBackgroundContextMenu())
            {
                ImGui::OpenPopup(BACKGROUND_CONTEXT_MENU);
            }
            ed::Resume();

            showContextMenu(openPopupPosition, static_cast<uint>(contextNodeId.Get()),
                            static_cast<uint>(contextLinkId.Get()),static_cast<uint>(contextPinId.Get()));
            // End of interaction with editor.
            ed::End();

            if (uiStatus.needInit)
                ed::NavigateToContent(0.0f);


            return 0;
        }

        /**
         * Sync id, value. From src to dst.
         * @param src
         * @param dst
         */
        void syncNodePort(std::vector<SightNodePort> & src, std::vector<SightNodePort> & dst){
            for (auto &item : dst) {
                for (const auto &srcItem : src) {
                    if (srcItem.portName == item.portName) {
                        // same port.
                        item.id = srcItem.id;
                        item.value = srcItem.value;
                        break;
                    }
                }
            }
        }

    }


    int initNodeEditor() {
        g_NodeEditorStatus = new NodeEditorStatus();

        return 0;
    }

    int destroyNodeEditor() {
        disposeGraph();

        delete g_NodeEditorStatus;
        g_NodeEditorStatus = nullptr;
        return 0;
    }

    void nodeEditorFrameBegin() {
        ed::SetCurrentEditor(g_NodeEditorStatus->context);
    }

    void nodeEditorFrameEnd() {
        ed::SetCurrentEditor(nullptr);
    }

    void onNodePortValueChange(SightNodePort* port) {
        auto node = port->node;
        node->graph->editing = true;
        if (!port->templateNodePort) {
            return;
        }

        auto isolate = currentUIStatus()->isolate;
        auto templateNodePort = port->templateNodePort;

        templateNodePort->onValueChange(isolate, port, JsEventType::ValueChange, (void*) & port->oldValue);
        if (port->getType() == IntTypeLargeString) {
            port->oldValue.stringFree();
        }
        port->oldValue = port->value.copy(port->getType());
    }

    void onNodePortAutoComplete(SightNodePort* port){
        if (!port->templateNodePort) {
            return;
        }

        port->templateNodePort->onAutoComplete(currentUIStatus()->isolate, port, JsEventType::AutoComplete);
    }

    void showNodePortValue(SightNodePort* port, bool fromGraph, int width, int type) {
        if (port->type > 0 && port->getType() != port->type) {
            // fake type, do not show it's value.
            return;
        }

        char labelBuf[NAME_BUF_SIZE];
        sprintf(labelBuf, "## %d", port->id);

        ImGui::SetNextItemWidth(width);
        bool usePortType = false;
        if (type <= 0) {
            type = port->getType();
            usePortType = true;
        }

        auto & options = port->options;
        switch (type) {
        case IntTypeFloat:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat(labelBuf, &port->value.f, 0.5f, 0, 0, "%.3f", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeDouble:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            if (ImGui::InputDouble(labelBuf, &port->value.d, 0,0,"%.6f", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeLong:
        case IntTypeInt:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragInt(labelBuf, &port->value.i, 1,0,0,"%d",flags)) {
                // onNodePortValueChange(port, oldValue);
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeString:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            width = width + width / 2.0f;
            bool showed = false;
            if (fromGraph) {
                
            } else {
                // only show in inspector
                if (!options.alternatives.empty()) {
                    // ImGui::Text("%8s", "");
                    // ImGui::SameLine();
                    ImGui::SetNextItemWidth(width);
                    std::string comboLabel = labelBuf;
                    comboLabel += ".combo";
                    if (ImGui::BeginCombo(comboLabel.c_str(), port->value.string, ImGuiComboFlags_NoArrowButton)) {
                        std::string filterLabel = comboLabel + ".filter";
                        static char filterText[NAME_BUF_SIZE] = {0};
                        ImGui::InputText(filterLabel.c_str(), filterText, std::size(filterText));
                        for (const auto& item : options.alternatives) {
                            if (strlen(filterText) > 0 && !startsWith(item, filterText)) {
                                continue;
                            }
                            if (ImGui::Selectable(item.c_str(), item == port->value.string)) {
                                snprintf(port->value.string, std::size(port->value.string), "%s", item.c_str());
                                onNodePortValueChange(port);
                            }
                        }
                        ImGui::EndCombo();
                    }

                    showed = true;
                }
            }

            if (!showed) {
                ImGui::SetNextItemWidth(width);
                if (ImGui::InputText(labelBuf, port->value.string, std::size(port->value.string), flags)) {
                }
                if (ImGui::IsItemActivated()) {
                    onNodePortAutoComplete(port);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    onNodePortValueChange(port);
                }
            }
            
            break;
        }
        case IntTypeBool:
        {
            if (checkBox(labelBuf, &port->value.b, options.readonly)) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeColor:
        {
            int flags = options.readonly ? ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs : 0;
            if (ImGui::ColorEdit3(labelBuf, port->value.vector4, flags)) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeVector3:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            ImGui::SetNextItemWidth(width * 2);
            if (ImGui::DragFloat3(labelBuf, port->value.vector3,1,0,0,"%.3f", flags)) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeVector4:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            ImGui::SetNextItemWidth(width * 2);
            if (ImGui::DragFloat4(labelBuf, port->value.vector4, 1,0,0,"%.3f",flags)) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeChar:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            ImGui::SetNextItemWidth(width / 3.0f);
            if (ImGui::InputText(labelBuf, port->value.string, 2,0)) {
                onNodePortValueChange(port);
            }
            break;
        }
        case IntTypeLargeString:
        {
            if (fromGraph) {
                ImGui::Text("%s", port->value.largeString.pointer);
                helpMarker("Please edit it in external editor or Inspector window.");
            } else {
                int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
                auto nWidth = width + width / 2.0f;
                if (ImGui::InputTextMultiline(labelBuf, port->value.largeString.pointer, port->value.largeString.bufferSize,
                                              ImVec2(nWidth, 150), flags)) {
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    dbg(1);
                    onNodePortValueChange(port);
                }
            }
            break;
        }
        case IntTypeButton:
        {
            if (ImGui::Button("button")) {
                dbg("=1");
            }
            break;
        }
        default:
        {
            bool isFind = false;
            auto& typeInfo = currentProject()->getTypeInfo(static_cast<uint>(type), &isFind);
            if (isFind) {
                // has type info
                if (usePortType && typeInfo.render.asIntType > 0) {
                    showNodePortValue(port, width, typeInfo.render.asIntType);
                } else {
                    typeInfo.render(labelBuf, port);
                }
            } else {
                ImGui::LabelText(labelBuf, "Unknown type of %d", port->getType());
            }
            break;
        }
        }
    }

    int showNodeEditorGraph(UIStatus const& uiStatus) {
        // if (uiStatus.needInit) {
        //     ImVec2 startPos = {
        //             300, 20
        //     };

        //     auto windowSize = uiStatus.io->DisplaySize - startPos;
        //     ImGui::SetNextWindowPos(startPos, ImGuiCond_Once);
        //     ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);
        // }

        ImVec2 startPos = {
            300, 20
        };
        auto windowSize = uiStatus.io->DisplaySize - startPos;
        ImGui::SetNextWindowPos(startPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

        std::string windowTitle("Graph");
        if (CURRENT_GRAPH) {
            windowTitle += " - ";
            windowTitle += CURRENT_GRAPH->getFilePath();
            windowTitle += "###Node Editor Graph";
        }
        
        ImGui::Begin(windowTitle.c_str(), nullptr,
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                     ImGuiWindowFlags_NoSavedSettings);
        if (CURRENT_GRAPH) {
            //return CODE_FAIL;
            showNodes(uiStatus);
        }
    
        ImGui::End();

        return 0;
    }

    uint nextNodeOrPortId() {
        return currentProject()->nextNodeOrPortId();
    }

    int addNode(SightNode *node) {

        CURRENT_GRAPH->addNode(*node);
        delete node;
        return 0;
    }

    SightNodeGraph *getCurrentGraph() {
        return CURRENT_GRAPH;
    }

    void disposeGraph(bool context) {
        // DO NOT delete entity graph.
        if (CURRENT_GRAPH && CURRENT_GRAPH != g_NodeEditorStatus->entityGraph) {
            delete CURRENT_GRAPH;
            CURRENT_GRAPH = nullptr;
        }

        if (context && g_NodeEditorStatus->context) {
            ed::DestroyEditor(g_NodeEditorStatus->context);
            g_NodeEditorStatus->context = nullptr;
        }
    }

    void changeGraph(const char *pathWithoutExt, bool loadEntityGraph) {
        dbg(pathWithoutExt);
        disposeGraph();

        ed::Config config;
        char* configFilePath = g_NodeEditorStatus->contextConfigFileBuf;
        sprintf(configFilePath, "%s.json", pathWithoutExt);
        config.SettingsFile = configFilePath;
        g_NodeEditorStatus->context = ed::CreateEditor(&config);
        
        if (loadEntityGraph) {
            g_NodeEditorStatus->graph = g_NodeEditorStatus->entityGraph;
        } else {
            char buf[NAME_BUF_SIZE];
            sprintf(buf, "%s.yaml", pathWithoutExt);
            g_NodeEditorStatus->loadOrCreateGraph(buf);
        }

        if (CURRENT_GRAPH) {
            CURRENT_GRAPH->callNodeEventsAfterLoad();
        }
        dbg("change over!");
        
    }

    SightNodePort::SightNodePort(NodePortType kind, uint type, const SightJsNodePort* templateNodePort)
        : templateNodePort(templateNodePort) {
        this->kind = kind;
        this->type = type;
    }

    bool SightNodePort::isConnect() const {
        return !connections.empty();
    }

    int SightNodePort::connectionsSize() const {
        return static_cast<int>(connections.size());
    }

    uint SightNodePort::getId() const {
        return id;
    }

    int SightNodePort::clearLinks() {
        auto g = getGraph();
        if (!g) {
            return -1;
        }
        
        std::vector<int> ids;
        std::transform(connections.begin(), connections.end(), std::back_inserter(ids), [](SightNodeConnection* c) {
            return c->connectionId;
        });
        dbg(ids);
        uint count = 0;
        for (const auto& item : ids) {
            if (g->delConnection(item) == CODE_OK) {
                count++;
            }
        }

        return count;
    }

    const char *SightNodePort::getDefaultValue() const {
        return this->value.string;
    }

    uint SightNodePort::getType() const {
        if (templateNodePort) {
            return templateNodePort->type;
        }
        return this->type;
    }


    void SightNode::addPort(const SightNodePort &port) {
        if (port.kind == NodePortType::Input) {
            this->inputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->outputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            this->inputPorts.push_back(port);

            // need change id
            this->outputPorts.push_back(port);
            this->outputPorts.back().id = nextNodeOrPortId();
        } else if (port.kind == NodePortType::Field) {
            this->fields.push_back(port);
        }

    }

    SightNode *SightNode::clone() const{
        auto p = new SightNode();
        p->copyFrom(this, CopyFromType::Clone);
        return p;
    }

    void SightNode::copyFrom(const SightNode* node, CopyFromType copyFromType) {
        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        this->templateNode = node->templateNode;

        auto copyFunc = [copyFromType](std::vector<SightNodePort> const& src, std::vector<SightNodePort>& dst) {
            for (const auto &item : src) {
                dst.push_back(item);

                if (copyFromType == CopyFromType::Instantiate) {
                    // init something
                    auto& back = dst.back();
                    if (back.getType() == IntTypeLargeString) {
                        back.value.stringInit();
                    }
                }
            }
        };

        copyFunc(node->inputPorts, this->inputPorts);
        copyFunc(node->outputPorts, this->outputPorts);
        copyFunc(node->fields, this->fields);

    }

    SightNodePortConnection SightNode::findConnectionByProcess() {
        SightNodeConnection* connection = nullptr;

        for (const auto &outputPort : outputPorts) {
            if (!outputPort.isConnect()) {
                continue;
            }

            if (outputPort.getType() == IntTypeProcess) {
                //
                connection = outputPort.connections.front();
                break;
            } else {
                if (connection) {
                    // has one.
                    connection = nullptr;
                    break;
                } else {
                    connection = outputPort.connections.front();
                }
            }
        }

        if (connection) {
            return SightNodePortConnection(graph, connection, this);
        }
        return {};
    }

    void SightNode::updateChainPortPointer() {
        for (auto &item : inputPorts) {
            if (item.portName.empty()) {
                // let empty name port to chain port.
                chainInPort = &item;
                break;
            }
        }

        for (auto &item : outputPorts) {
            if (item.portName.empty()) {
                // let empty name port to chain port.
                chainOutPort = &item;
                break;
            }
        }

    }

    void SightNode::tryAddChainPorts() {
        auto nodeFunc = [](std::vector<SightNodePort> & list, NodePortType kind){
            bool find = false;
            for (const auto &item : list) {
                if (item.portName.empty()) {
                    find = true;
                    break;
                }
            }

            if (!find) {
                list.push_back({
                    kind,
                    IntTypeProcess,
                });
            }
        };

        nodeFunc(this->inputPorts, NodePortType::Input);
        nodeFunc(this->outputPorts, NodePortType::Output);

        updateChainPortPointer();
    }

    void SightNode::reset() {
        dbg("node reset");
        // call events
        if (templateNode) {
            auto t = dynamic_cast<const SightJsNode*>(templateNode);
            if (t) {
                t->onDestroyed(currentUIStatus()->isolate, this);
            }
        }

        auto nodeFunc = [] (std::vector<SightNodePort> & list){
            for(auto& item: list){
                if (item.getType() == IntTypeLargeString) {
                    item.value.stringFree();
                }
            }
        };

        CALL_NODE_FUNC(this);
    }

    SightNodePortHandle SightNode::findPort(const char* name, int orderSize, int order[]) {
        static constexpr int defaultOrder[] = { 
            static_cast<int>(NodePortType::Field), 
            static_cast<int>(NodePortType::Input), 
            static_cast<int>(NodePortType::Output) 
        };

        if (orderSize <= 0) {
            // use normal methods
            orderSize = 3;
            order = (int*)defaultOrder;
        }

        SightNodePortHandle portHandle = {};
        auto func = [&portHandle, name, this](std::vector<SightNodePort> const& list) {
            for (const auto& item : list) {
                if (item.portName == name) {
                    portHandle.node = this;
                    portHandle.portId = item.id;
                    return true;
                }
            }

            return false;
        };

        for( int i = 0; i < orderSize; i++){
            switch (static_cast<NodePortType>(order[i])) {
            case NodePortType::Input:
                if (func(this->inputPorts)) {
                    goto result;
                }
                break;
            case NodePortType::Output:
                if (func(this->outputPorts)) {
                    goto result;
                }
                break;
            case NodePortType::Field:
                if (func(this->fields)) {
                    goto result;
                }
                break;
            default:
                break;
            }
        }

        result:
        return portHandle;
    }

    uint SightNode::getNodeId() const {
        return this->nodeId;
    }

    SightJsNode::SightJsNode() {

    }

    void SightJsNode::addPort(const SightJsNodePort& port) {
        if (port.kind == NodePortType::Input) {
            this->originalInputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->originalOutputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            this->originalInputPorts.push_back(port);
            this->originalOutputPorts.push_back(port);
        } else if (port.kind == NodePortType::Field) {
            this->originalFields.push_back(port);
        }
    }


    void SightJsNode::compact() {
        
        auto & array = g_NodeEditorStatus->templateNodePortArray;

        for( const auto& item: this->originalFields){
            auto p = array.add(item);
            this->fields.push_back(p);
        }
        this->originalFields.clear();

        for (const auto& item : this->originalInputPorts) {
            auto p = array.add(item);
            this->inputPorts.push_back(p);
        }
        this->originalInputPorts.clear();

        for (const auto& item : this->originalOutputPorts) {
            auto p = array.add(item);
            this->outputPorts.push_back(p);
        }
        this->originalOutputPorts.clear();
    }

    void SightJsNode::compileFunctions(Isolate* isolate) {
        // auto nodeFunc = [isolate](std::vector<SightJsNodePort*> const& list){
        //     for( const auto& item: list){
        //         item->onValueChange.compile(isolate);
        //         item->onAutoComplete.compile(isolate);
        //     }
        // };

        // CALL_NODE_FUNC(this);

        // onInstantiate.compile(isolate);
        // onDestroyed.compile(isolate);
        // onReload.compile(isolate);
    }

    SightNode* SightJsNode::instantiate(bool generateId, bool callEvent) const {
        dbg(this->nodeName);
        auto p = new SightNode();
        
        auto portCopyFunc = [](std::vector<SightJsNodePort*> const& src, std::vector<SightNodePort> & dst){
            for( const auto& item: src){
                auto copy = item->instantiate();
                copy.templateNodePort = item;
                dst.push_back(copy);
                dst.back().oldValue = dst.back().value;
            }
        };
        portCopyFunc(this->inputPorts, p->inputPorts);
        portCopyFunc(this->outputPorts, p->outputPorts);
        portCopyFunc(this->fields, p->fields);
        p->nodeName = this->nodeName;
        p->templateNode = this;
        p->tryAddChainPorts();
        p->graph = CURRENT_GRAPH;

        if (generateId) {
            // generate id
            auto nodeFunc = [](std::vector<SightNodePort>& list) {
                for (auto& item : list) {
                    item.id = nextNodeOrPortId();

                    // check type
                    auto t = item.getType();
                    if (!isBuiltInType(t)) {
                        bool isFind = false;
                        auto typeInfo = currentProject()->getTypeInfo(t, &isFind);
                        if (isFind) {
                            typeInfo.initValue(item.value);
                        }
                    }
                }
            };

            p->nodeId = nextNodeOrPortId();
            CALL_NODE_FUNC(p);
        }

        if (callEvent) {
            auto tmp = dynamic_cast<const SightJsNode*>(this);
            if (tmp) {
                tmp->onInstantiate(currentUIStatus()->isolate, p);
            }
        }
        return p;
    }

    void SightNodeConnection::removeRefs(SightNodeGraph *graph) {
        if (!graph) {
            graph = CURRENT_GRAPH;
        }
        auto leftPort = graph->findPort(leftPortId());
        auto rightPort = graph->findPort(rightPortId());

        if (leftPort) {
            auto & c = leftPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
        if (rightPort) {
            auto & c = rightPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
    }

    uint SightNodeConnection::leftPortId() const {
        return this->left;
    }

    uint SightNodeConnection::rightPortId() const {
        return this->right;
    }

    SightNodePort* SightNodeConnection::findLeftPort() const {
        return left > 0 ? CURRENT_GRAPH->findPort(left) : nullptr;
    }

    SightNodePort* SightNodeConnection::findRightPort() const {
        return right > 0 ? CURRENT_GRAPH->findPort(right) : nullptr;
    }

    int SightNodeGraph::saveToFile(const char *path, bool set,bool saveAnyway) {
        if (set) {
            this->setFilePath(path);
        }
        if (!saveAnyway && isBroken()) {
            return CODE_FAIL;
        }

        // ctor yaml object
        YAML::Emitter out;
        out << YAML::BeginMap;
        // file header
        out << YAML::Key << whoAmI << YAML::Value << "sight-graph";
        // nodes
        out << YAML::Key << "nodes";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &node : nodes) {
            out << YAML::Key << node.nodeId;
            out << YAML::Value;
            out << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << node.nodeName;
            out << YAML::Key << "id" << YAML::Value << node.nodeId;

            if (node.templateNode) {
                auto address = findTemplateNode( &node);
                if (address) {
                    out << YAML::Key << "template" << YAML::Value << address->fullTemplateAddress;
                }
            }

            auto writeFloatArray = [&out](float* p, int size) {
                out << YAML::BeginSeq;
                for( int i = 0; i < size; i++){
                    out << p[i];
                }
                out << YAML::EndSeq;
            };

            auto portWork = [&out, &writeFloatArray](SightNodePort const& item) {
                out << YAML::Key << item.id;
                out << YAML::Value << YAML::BeginMap;
                out << YAML::Key << "name" << YAML::Value << item.portName;
                out << YAML::Key << "type" << YAML::Value << item.type;

                auto type = item.getType();
                out << YAML::Key << "value" << YAML::Value;
                switch (type) {
                case IntTypeFloat:
                    out << item.value.f;
                    break;
                case IntTypeDouble:
                    out << item.value.d;
                    break;
                case IntTypeInt:
                case IntTypeLong:
                    out << item.value.i;
                    break;
                case IntTypeString:
                    out << item.value.string;
                    break;
                case IntTypeLargeString:
                    out << item.value.largeString.pointer;
                    break;
                case IntTypeBool:
                    out << item.value.b;
                    break;
                case IntTypeVector3:
                    writeFloatArray((float*)item.value.vector3, 3);
                    break;
                case IntTypeChar:
                    out << item.value.string[0];
                    break;
                case IntTypeVector4:
                case IntTypeColor:
                    writeFloatArray((float*)item.value.vector4, 4);
                    break;
                default:
                {
                    bool serializeGood = false;
                    if (!isBuiltInType(type)) {
                        //
                        auto [typeInfo, find] = currentProject()->findTypeInfo(type);
                        if (find) {
                            switch (typeInfo.render.kind) {
                            case TypeInfoRenderKind::ComboBox:
                                out << item.value.i;
                                serializeGood = true;
                                break;
                            case TypeInfoRenderKind::Default:
                                break;
                            }
                        }
                    }
                    if (!serializeGood) {
                        out << YAML::Null;
                    }
                    break;
                }
                }

                out << YAML::EndMap;
            };

            // input and output
            out << YAML::Key << "inputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.inputPorts) {
                portWork(item);
            }
            out << YAML::EndMap;

            out << YAML::Key << "outputs";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.outputPorts) {
                portWork(item);
            }
            out << YAML::EndMap;

            out << YAML::Key << "fields";
            out << YAML::Value << YAML::BeginMap;
            for (const auto &item : node.fields) {
                portWork(item);
            }
            out << YAML::EndMap;

            //
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        // connections
        out << YAML::Key << "connections";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &connection : connections) {
            out << YAML::Key << connection.connectionId << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "left" << YAML::Value << connection.leftPortId();
            out << YAML::Key << "right" << YAML::Value << connection.rightPortId();
            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        // other info

        out << YAML::EndMap;       // end of 1st begin map

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        dbg("save over");
        return CODE_OK;
    }

    int SightNodeGraph::load(const char *path, bool isLoadEntity) {
        this->filepath = path;

        std::ifstream fin(path);
        if (!fin.is_open()) {
            return -1;
        }

        int status = CODE_OK;
        try {
            auto root = YAML::Load(fin);

            // nodes
            auto nodeRoot = root["nodes"];
            dbg(nodeRoot.size());

            auto readFloatArray = [](float* p, int size, YAML::Node const& valueNode) {
                if (valueNode.IsSequence()) {
                    for( int i = 0; i < size; i++){
                        auto tmp = valueNode[i];
                        if (tmp.IsDefined()) {
                            p[i] = tmp.as<float>();
                        }
                    }
                }
            };

            auto portWork = [&readFloatArray,this](YAML::detail::iterator_value const& item, NodePortType nodePortType, std::vector<SightNodePort> & list) {
                auto id = item.first.as<uint>();
                auto values = item.second;
                auto portName = values["name"].as<std::string>();
                auto typeNode = values["type"];
                uint type = 0;
                if (typeNode.IsDefined()) {
                    type = typeNode.as<uint>();
                }

                SightNodePort* pointer = nullptr;
                for(auto& item: list){
                    if (item.portName == portName) {
                        pointer = &item;
                        break;
                    }
                }
                if (!pointer) {
                    // todo add node port to list 
                    this->broken = true;
                    dbg(portName, "not found");
                    return CODE_FAIL;
                }

                auto & port = *pointer;
                port.portName = portName;
                port.id = id;
                port.kind = nodePortType;
                port.type = type;

                auto valueNode = values["value"];
                type = port.getType();    // update to real type.
                if (valueNode.IsDefined()) {
                    switch (type) {
                    case IntTypeFloat:
                        port.value.f = valueNode.as<float>();
                        break;
                    case IntTypeDouble:
                        port.value.d = valueNode.as<double>();
                        break;
                    case IntTypeInt:
                    case IntTypeLong:
                        port.value.i = valueNode.as<int>();
                        break;
                    case IntTypeString:
                        sprintf(port.value.string, "%s", valueNode.as<std::string>().c_str());
                        break;
                    case IntTypeLargeString:
                    {
                        std::string tmpString = valueNode.as<std::string>();
                        port.value.stringCheck(tmpString.size());
                        sprintf(port.value.largeString.pointer, "%s", tmpString.c_str());
                        break;
                    }
                    case IntTypeBool:
                        port.value.b = valueNode.as<bool>();
                        break;
                    case IntTypeProcess:
                    case IntTypeButton:
                        // dbg("IntTypeProcess" , portName);
                        break;
                    case IntTypeChar:
                        if (valueNode.IsDefined() && !valueNode.IsNull()) {
                            port.value.string[0] = valueNode.as<char>();
                        }
                        break;
                    case IntTypeVector3:
                        readFloatArray(port.value.vector3, 3, valueNode);
                        break;
                    case IntTypeVector4:
                    case IntTypeColor:
                        readFloatArray(port.value.vector4, 4, valueNode);
                        break;
                    default:
                    {
                        if (!isBuiltInType(type)) {
                            //
                            auto [typeInfo, find] = currentProject()->findTypeInfo(type);
                            if (find) {
                                switch (typeInfo.render.kind) {
                                case TypeInfoRenderKind::ComboBox:
                                    port.value.i = valueNode.as<int>();
                                    break;
                                case TypeInfoRenderKind::Default:
                                default:
                                    dbg("no-render-function", type, getTypeName(type));
                                    break;
                                }
                            }
                        } else {
                            dbg("type error, unHandled", type, getTypeName(type));
                        }
                    } break;
                    }
                }

                port.oldValue = port.value;
                return CODE_OK;
            };

            for (const auto &nodeKeyPair : nodeRoot) {
                auto yamlNode = nodeKeyPair.second;

                auto templateNodeAddress = yamlNode["template"].as<std::string>("");
                auto templateNode = g_NodeEditorStatus->findTemplateNode(templateNodeAddress.c_str());
                if (!templateNode) {
                    dbg(templateNodeAddress, " template not found.");
                    broken = true;
                    status = 1;
                    continue;
                }

                auto nodePointer = templateNode->instantiate(false, false);
                auto & sightNode = *nodePointer;
                sightNode.nodeId = yamlNode["id"].as<int>();
                sightNode.nodeName = yamlNode["name"].as<std::string>();

                auto inputs = yamlNode["inputs"];
                for (const auto &input : inputs) {
                    portWork(input, NodePortType::Input, sightNode.inputPorts);
                }

                auto outputs = yamlNode["outputs"];
                for (const auto &output : outputs) {
                    portWork(output, NodePortType::Output, sightNode.outputPorts);
                }

                auto fields = yamlNode["fields"];
                for (const auto &field : fields) {
                    portWork(field, NodePortType::Field, sightNode.fields);
                }

                if (!templateNodeAddress.empty()) {
                    if (isLoadEntity) {
                        // entity should be into template
                        // fixme: load entity
                        // SightNodeTemplateAddress address = {
                        //         templateNodeAddress,
                        //         sightNode
                        // };
                        // addTemplateNode(address);
                    }

                    
                    if (!templateNode) {
                        dbg(templateNodeAddress, " template not found.");
                        broken = true;
                        status = CODE_FAIL;
                    } 
                } else {
                    dbg(sightNode.nodeName, "entity do not have templateNodeAddress, jump it.");
                }

                sightNode.graph = this;
                addNode( sightNode);
                delete nodePointer;
            }

            // connections
            auto connectionRoot = root["connections"];
            for (const auto &item : connectionRoot) {
                auto connectionId = item.first.as<int>();
                auto left = item.second["left"].as<int>();
                auto right = item.second["right"].as<int>();

                createConnection(left, right, connectionId);
            }

            this->editing = false;
            dbg("load ok");
            return status;
        }catch (const YAML::BadConversion & e){
            fprintf(stderr, "read file %s error!\n", path);
            fprintf(stderr, "%s\n", e.what());
            this->reset();
            return CODE_FILE_ERROR;    // bad file
        } 

        // return 0;
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
        auto p = this->nodes.add(node);
        p->updateChainPortPointer();
        idMap[node.nodeId] = {
                SightAnyThingType::Node,
                p
        };

        auto nodeFunc = [p, this](std::vector<SightNodePort> & list){
            for (auto &item : list) {
                item.node = p;
                idMap[item.id] = {
                        SightAnyThingType::Port,
                        p,
                        item.id
                };
            }
        };
        CALL_NODE_FUNC(p);

        this->editing = true;
    }

    SightNode *SightNodeGraph::findNode(uint id) {
        return findSightAnyThing(id).asNode();
    }

    SightNode* SightNodeGraph::findNode(SightNodeProcessType processType, int* status /* = nullptr */){
        int count = 0;

        SightNode* p = nullptr;
        for (auto &n : nodes){
            if (findTemplateNode(&n)->options.processFlag == processType){
                count++;
                if (!p) {
                    p = &n;
                }
            }
        }

        if (count == 0) {
            SET_INT_VALUE(status, 2);
            return nullptr;
        } else if (count == 1) {
            SET_INT_VALUE(status, 0);
            return p;
        } else {
            SET_INT_VALUE(status, 1);
            return p;
        }

//        return nullptr;
    }

    SightNodeConnection *SightNodeGraph::findConnection(int id) {
        return findSightAnyThing(id).asConnection();
    }

    uint SightNodeGraph::createConnection(uint leftPortId, uint rightPortId, uint connectionId) {
        if (leftPortId == rightPortId) {
            return -2;
        }

        auto left = findPort(leftPortId);
        auto right = findPort(rightPortId);
        if (!left || !right) {
            return -1;
        }
        if (left->kind == right->kind) {
            return -3;
        }

        auto id = connectionId > 0 ? connectionId : nextNodeOrPortId();
        // find color ;
        ImU32 color = IM_COL32_WHITE;
        auto [typeInfo, find] = currentProject()->findTypeInfo(left->getType());
        if (find && typeInfo.style) {
            color = typeInfo.style->color;
        }

        SightNodeConnection connection = {
                id,
                leftPortId,
                rightPortId,
                color,
        };
        addConnection(connection, left, right);
        return id;
    }

    void SightNodeGraph::addConnection(const SightNodeConnection &connection, SightNodePort* left, SightNodePort* right) {
        auto p = this->connections.add(connection);
        idMap[connection.connectionId] = {
                SightAnyThingType::Connection,
                p
        };

        if (!left) {
            left = findPort(connection.leftPortId());
        }
        if (!right) {
            right = findPort(connection.rightPortId());
        }

        left->connections.push_back(p);
        right->connections.push_back(p);
    }

    const SightArray <SightNode> & SightNodeGraph::getNodes() const {
        return this->nodes;
    }

    SightNodeGraphExternalData& SightNodeGraph::getExternalData() {
        return externalData;
    }

    void SightNodeGraph::callNodeEventsAfterLoad() {
        // call event one by one.
        auto isolate = currentUIStatus()->isolate;
        auto nodeFunc = [isolate](std::vector<SightNodePort> & list){
            for(auto& item: list){
                if (item.templateNodePort) {
                    auto templateNodePort = item.templateNodePort;
                    templateNodePort->onValueChange(isolate, &item);
                }
            }
        };

        for(auto& item: this->nodes){
            auto p = &item;
            if (item.templateNode) {
                item.templateNode->onReload(isolate, p);
            }

            // port events
            // CALL_NODE_FUNC(p);
        }
    }

    const SightArray<SightNodeConnection>& SightNodeGraph::getConnections() const {
        return this->connections;
    }

    SightNodePort* SightNodeGraph::findPort(uint id) {
        return findSightAnyThing(id).asPort();
    }

    SightNodePortHandle SightNodeGraph::findPortHandle(uint id) {
        auto t = findSightAnyThing(id);
        if (t.data.portHandle) {
            return t.data.portHandle;
        }
        return {};
    }

    const SightAnyThingWrapper& SightNodeGraph::findSightAnyThing(uint id) {
        std::map<uint, SightAnyThingWrapper>::iterator iter;
        if ((iter = idMap.find(id)) == idMap.end()) {
            return invalidAnyThingWrapper;
        }
        return iter->second;
    }

    int SightNodeGraph::delNode(int id) {
        auto result = std::find_if(nodes.begin(), nodes.end(), [id](const SightNode& n){ return n.nodeId == id; });
        if (result == nodes.end()) {
            return CODE_FAIL;
        }

        nodes.erase(result);
        this->editing = true;
        return CODE_OK;
    }

    int SightNodeGraph::delConnection(int id, bool removeRefs) {
        auto result = std::find_if(connections.begin(), connections.end(),
                                   [id](const SightNodeConnection &c) { return c.connectionId == id; });
        if (result == connections.end()) {
            return CODE_FAIL;
        }

        // deal refs.
        if (removeRefs) {
            result->removeRefs();
        }
        idMap.erase(id);

        connections.erase(result);
        dbg(id);
        return CODE_OK;
    }

    int SightNodeGraph::save() {
        auto c = saveToFile(this->filepath.c_str(), false);
        if (c == CODE_OK) {
            this->editing = false;
        }
        return c;
    }

    SightNodeGraph::SightNodeGraph() {

    }

    SightNodeGraph::~SightNodeGraph() {
        this->dispose();
    }

    void SightNodeGraph::reset() {
        this->nodes.clear();
        this->connections.clear();
        this->idMap.clear();
    }

    SightArray<SightNode> &SightNodeGraph::getNodes() {
        return this->nodes;
    }

    bool SightNodeGraph::isBroken() const {
        return this->broken;
    }

    int NodeEditorStatus::createGraph(const char *path) {
        if (graph) {
            return -1;     // has one.
        }

        graph = new SightNodeGraph();
        graph->saveToFile(path, true);
        return 0;
    }

    int NodeEditorStatus::loadOrCreateGraph(const char *path) {
        if (graph) {
            return -1;
        }

        graph = new SightNodeGraph();
        if (graph->load(path) != CODE_OK) {
            // if (!graph->isBroken()) {
            //     // create one.
            //     graph->save();
            // } else {

            // }

            // create one.
            graph->save();
        }

        return 0;
    }

    SightJsNode* NodeEditorStatus::findTemplateNode(const char* path){
        auto address = resolveAddress(path);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        bool findElement = false;
        SightJsNode* result = nullptr;
        while (pointer) {
            for(auto iter = list->begin(); iter != list->end(); iter++){
                if (iter->name == pointer->part) {
                    if (!pointer->next) {
                        // the node
                        result = iter->templateNode;
                    }

                    findElement = true;
                    list = & iter->children;
                    break;
                }
            }
            if (!findElement || result) {
                break;
            }

            pointer = pointer->next;
        }


        freeAddress(address);
        return result;
    }

    NodeEditorStatus::NodeEditorStatus() {

    }

    NodeEditorStatus::~NodeEditorStatus() {
        // todo free memory.

    }


    SightNode *SightAnyThingWrapper::asNode() const {
        if (type != SightAnyThingType::Node) {
            return nullptr;
        }
        return data.node;
    }

    SightNodeConnection* SightAnyThingWrapper::asConnection() const{
        if (type != SightAnyThingType::Connection) {
            return nullptr;
        }
        return data.connection;
    }

    SightNodePort *SightAnyThingWrapper::asPort() const{
        if (type != SightAnyThingType::Port) {
            return nullptr;
        }
        return data.portHandle.get();
    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNodeConnection* connection)
        :type(type), data(connection)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node)
        :type(type), data(node)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node, uint portId)
        :type(type), data(node, portId)
    {

    }


    void SightNodeTemplateAddress::showContextMenu(const ImVec2 &openPopupPosition) {
        if (children.empty()) {
            if (ImGui::MenuItem(name.c_str())) {
                //
                // dbg("it will create a new node.", this->name);
                auto node = this->templateNode->instantiate();
                ed::SetNodePosition(node->nodeId, openPopupPosition);
                addNode(node);
            }
        } else {
            if (ImGui::BeginMenu(name.c_str())) {
                for (auto &item : children) {
                    item.showContextMenu(openPopupPosition);
                }

                ImGui::EndMenu();
            }
        }
    }

    void SightNodeTemplateAddress::dispose() {
        if (templateNode) {
            // try use array first, if it's not an element of the array, then delete it.
            if (!g_NodeEditorStatus->templateNodeArray.remove(templateNode)) {
                delete templateNode;
            }
            templateNode = nullptr;
        }
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress() {

    }


    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name, SightJsNode* templateNode)
        : name(std::move(name))
        , templateNode(templateNode) {
    }

    SightNodeTemplateAddress::~SightNodeTemplateAddress() {
//        if (templateNode) {
//            delete templateNode;
//            templateNode = nullptr;
//        }
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name) : name(std::move(name)) {

    }

    SightNodePort* SightNodePortHandle::get() const {
        if (!node) {
            return nullptr;
        }

        for (auto &item : node->inputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        for (auto &item : node->outputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        for(auto& item: node->fields){
            if (item.id == portId) {
                return &item;
            }
        }

        return nullptr;
    }

    SightNodePort *SightNodePortHandle::operator->() const {
        return get();
    }

    SightNodePort &SightNodePortHandle::operator*() const {
        return *get();
    }

    SightNodePortHandle::operator bool() const {
        return this->node && this->portId > 0;
    }

    int addTemplateNode(const SightNodeTemplateAddress &templateAddress) {
        dbg(templateAddress.name);
        if (templateAddress.name.empty()) {
            return -1;
        }

        SightJsNode* templateNode = templateAddress.templateNode;
        templateNode->fullTemplateAddress = templateAddress.name;
        auto address = resolveAddress(templateAddress.name);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        while (pointer) {
            bool findElement = false;
            if (!pointer->next) {
                // no next, this is the last element, it should be the node's name.
                templateNode->compact();
                auto tmpPointer = g_NodeEditorStatus->templateNodeArray.add( *templateNode);
                tmpPointer->compileFunctions(currentUIStatus()->isolate);
                list->push_back(SightNodeTemplateAddress(pointer->part, tmpPointer));
                break;
            }

            for(auto iter = list->begin(); iter != list->end(); iter++){
                if (iter->name == pointer->part) {
                    findElement = true;
                    list = & iter->children;
                    break;
                }
            }

            if (!findElement) {
                list->push_back(SightNodeTemplateAddress(pointer->part));
                list = & list->back().children;
            }

            pointer = pointer->next;
        }

        freeAddress(address);
        return 0;
    }

    SightNode *generateNode(const UICreateEntity &createEntityData,SightNode *node) {
        if (!node) {
            node = new SightNode();
        }

        node->nodeName = createEntityData.name;
        node->nodeId = nextNodeOrPortId();         // All node and port's ids need to be initialized.

        auto c = createEntityData.first;
        auto type = getIntType(c->type, true);

        while (c) {
            // input
            // todo entity 
            // SightNodePort port = {
            //         c->name,
            //         nextNodeOrPortId(),
            //         NodePortType::Input,
            //         type,
            //         {},
            //         {},
            //         {},
            //         std::vector<SightNodeConnection*>(),
            // };
            // // todo distinguish type
            // sprintf(port.value.string, "%s", c->defaultValue);
            // node->inputPorts.push_back(port);

            // // output
            // port.id = nextNodeOrPortId();
            // node->outputPorts.push_back(port);

            c = c->next;
        }

        return node;
    }

    int addEntity(const UICreateEntity &createEntityData) {
        SightNode node;
        generateNode(createEntityData, &node);

        // add to templateNode
        SightNodeTemplateAddress templateAddress;
        std::string address = createEntityData.templateAddress;
        const char* prefix = "entity/";
        // check start with entity
        if (address.find(prefix) != 0) {
            address = prefix + address;
        }
        
        // check end with node name.
        if (!endsWith(address, createEntityData.name)) {
            if (!endsWith(address, "/")) {
                address += "/";
            }
            address += createEntityData.name;
        }
        templateAddress.name = address;
        // fixme: add entity
        // templateAddress.templateNode = node;
        // addTemplateNode(templateAddress);

        // convert to SightNode, and add it to entity graph.
        auto graph = g_NodeEditorStatus->entityGraph;
        // let node's template be itself.
        node.templateNode = g_NodeEditorStatus->findTemplateNode(address.c_str());
        dbg(node.nodeName, node.templateNode);
        graph->addNode(node);
        graph->save();
        return 0;
    }


    int loadEntities() {
        if (g_NodeEditorStatus->entityGraph) {
            return 1;
        }

        auto graph = g_NodeEditorStatus->entityGraph = new SightNodeGraph();
        return graph->load("./entity.yaml", true);
    }

    const SightJsNode* findTemplateNode(const SightNode *node) {
        return node->templateNode;
    }

    SightJsNode* findTemplateNode(const char* path) {
        return g_NodeEditorStatus->findTemplateNode(path);
    }

    bool isNodeEditorReady() {
        return g_NodeEditorStatus->context;
    }

    SightAnyThingData::SightAnyThingData() {
        this->node = nullptr;
    }

    SightAnyThingData::SightAnyThingData(SightNodeConnection *connection) {
        this->connection = connection;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node) {
        this->node = node;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node, uint portId) {
        this->portHandle = { node, portId };
    }

    bool SightNodePortConnection::bad() const {
        return !self || !target || !connection;
    }

    SightNodePortConnection::SightNodePortConnection(SightNodeGraph* graph,SightNodeConnection *connection, SightNode* node) {
        auto leftPointer = graph->findPort(connection->left);
        auto rightPointer = graph->findPort(connection->right);
        this->connection = connection;

        if (leftPointer->node == node) {
            self = leftPointer;
            target= rightPointer;

        } else if (rightPointer->node == node) {
            self = rightPointer;
            target = leftPointer;
        }
    }

    SightNodePortConnection::SightNodePortConnection(SightNodePort *self, SightNodePort *target,
                                                     SightNodeConnection *connection) : self(self), target(target),
                                                                                        connection(connection) {}

    void SightNodeValue::stringInit() {
        largeString.bufferSize = NAME_BUF_SIZE * 2;
        largeString.size = 0;
        largeString.pointer = (char*)calloc(1, largeString.bufferSize);

        // dbg(largeString.bufferSize, largeString.pointer, largeString.size);
    }

    void SightNodeValue::stringCheck(size_t needSize) {
        stringFree();
        static constexpr size_t tmpSize = NAME_BUF_SIZE * 2;
        
        largeString.bufferSize = needSize < tmpSize ? tmpSize : static_cast<size_t>(needSize * 1.5f);
        largeString.size = 0;
        largeString.pointer = (char*)calloc(1, largeString.bufferSize);
    }

    void SightNodeValue::stringFree() {
        if (largeString.pointer) {
            free(largeString.pointer);
            largeString.pointer = nullptr;
            largeString.size = largeString.bufferSize = 0;
        }
    }

    void SightNodeValue::stringCopy(std::string const& str) {
        stringCheck(str.size());

        sprintf(largeString.pointer, "%s", str.c_str());
        largeString.size = str.size();
    }

    SightNodeValue SightNodeValue::copy(int type) const {
        SightNodeValue tmp;
        if (type == IntTypeLargeString) {
            // large string
            tmp.stringCopy(this->largeString.pointer);
        } else {
            memcpy(tmp.string, this->string, std::size(this->string));
        }
        return tmp;
    }

    ScriptFunctionWrapper::ScriptFunctionWrapper(Function const& f)
        : function(f) {
    }

    ScriptFunctionWrapper::ScriptFunctionWrapper(std::string const& code)
        : sourceCode(code)
    {
        
    }

    void ScriptFunctionWrapper::compile(Isolate* isolate) {
        if (!function.IsEmpty() || sourceCode.empty()) {
            return;
        }
        using namespace v8;

        // trim(sourceCode);
        // if (!startsWith(sourceCode, "function")) {
        //     sourceCode = "function " + sourceCode;
        // }
        // sourceCode = "return " + sourceCode;
        // dbg(sourceCode);

        // auto context = isolate->GetCurrentContext();
        // HandleScope handleScope(isolate);
        // v8::ScriptCompiler::Source source(v8pp::to_v8(isolate, sourceCode.c_str()));
        // auto mayFunction = v8::ScriptCompiler::CompileFunctionInContext(context, &source, 0, nullptr, 0, nullptr);
        // if (mayFunction.IsEmpty()) {
        //     dbg("compile failed");
        // } else {
        //     auto recv = Object::New(isolate);
        //     auto t1 = mayFunction.ToLocalChecked()->Call(context, recv, 0, nullptr);
        //     if (!t1.IsEmpty()) {
        //         auto t2 = t1.ToLocalChecked();
        //         if (t2->IsFunction()) {
        //             function = Function(isolate, t2.As<v8::Function>());
        //             sourceCode.clear();
        //         }
        //     }
        // }

        auto f = recompileFunction(isolate, sourceCode);
        if (!f.IsEmpty()) {
            function = Function(isolate, f);
            sourceCode.clear();
        }
    }

    void ScriptFunctionWrapper::operator()(Isolate* isolate, SightNode* node) const {
        if (!checkFunction(isolate)) {
            return;
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto recv = v8pp::class_<SightNode>::reference_external(isolate, node);

        Local<Value> args[0];
        auto result = f->Call(context, recv, 0, args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        if (result.IsEmpty()) {
            return;
        }
        
    }

    void ScriptFunctionWrapper::operator()(Isolate* isolate, SightNodePort* thisNodePort, JsEventType eventType, void* pointer) const {
        if (!checkFunction(isolate) || !thisNodePort) {
            return;
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto node = thisNodePort->node;
        auto recv = nodePortValue(isolate, { node, thisNodePort->id});

        // prepare args
        Local<Value> args[2];
        args[0] = v8pp::class_<SightNode>::reference_external(isolate, node);
        args[1] = Object::New(isolate);
        if (eventType == JsEventType::AutoComplete) {

        } else if (eventType == JsEventType::Connect) {
            args[1] = connectionValue(isolate, (SightNodeConnection *)pointer, thisNodePort);
        } else if (eventType == JsEventType::ValueChange) {
            args[1] = getPortValue(isolate, thisNodePort->getType(), *((const SightNodeValue*)pointer));
        }

        // call function
        auto mayResult = f->Call(context, recv, std::size(args), args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        // v8pp::cleanup(isolate);
        if (mayResult.IsEmpty()) {
            return;
        }

        // after call function
        if (eventType == JsEventType::AutoComplete) {
            // 
            auto result = mayResult.ToLocalChecked();
            auto & options = thisNodePort->options;
            options.alternatives.clear();
            if (!result->IsNullOrUndefined()) {
                if (result->IsArray()) {
                    auto list = v8pp::from_v8<std::vector<std::string>>(isolate, result);
                    options.alternatives.assign(list.begin(), list.end());
                } else if (IS_V8_STRING(result)) {
                    options.alternatives.push_back(v8pp::from_v8<std::string>(isolate, result));
                }
            }
        }
    }

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::operator()(Isolate* isolate, SightNode* node, v8::Local<v8::Value> arg1, v8::Local<v8::Value> arg2) const {
        if (!checkFunction(isolate)) {
            return {};
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto recv = v8pp::class_<SightNode>::reference_external(isolate, node);

        Local<Value> args[2];
        args[0] = arg1;
        args[1] = arg2;
        auto result = f->Call(context, recv, std::size(args), args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);

        return result;
    }

    ScriptFunctionWrapper::operator bool() const {
        return !function.IsEmpty() || !sourceCode.empty();
    }

    bool ScriptFunctionWrapper::checkFunction(Isolate* isolate) const {
        if (function.IsEmpty()) {
            // dbg(sourceCode);
            if (!sourceCode.empty()) {
                const_cast<ScriptFunctionWrapper*>(this)->compile(isolate);
            } else {
                return false;
            }

            return !function.IsEmpty();
        }

        return true;
    }

    void SightBaseNodePort::setKind(int intKind) {
        this->kind = NodePortType(intKind);
    }

    uint SightBaseNodePort::getType() const {
        return this->type;
    }

    const char* SightBaseNodePort::getPortName() const {
        return portName.c_str();
    }

    SightNodeGraph* SightNodePort::getGraph() {
        if (this->node) {
            return this->node->graph;
        }
        return nullptr;
    }

    const SightNodeGraph* SightNodePort::getGraph() const {
        return const_cast<SightNodePort*>(this)->getGraph();
    }

    SightJsNodePort::SightJsNodePort(std::string const& name, NodePortType kind)
    {
        this->portName = name;
        this->kind = kind;
    }

    SightNodePort SightJsNodePort::instantiate() const {
        SightNodePort tmp = { this->kind, this->type, this};
        tmp.value = this->value;
        tmp.portName = this->portName;
        return tmp;
    }
}