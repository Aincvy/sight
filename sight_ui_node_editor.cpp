#include "sight_ui_node_editor.h"

#include "dbg.h"
#include "sight_external_widgets.h"

#include "imgui_node_editor.h"
#include "sight.h"
#include "sight_address.h"
#include "sight_keybindings.h"

#include "imgui.h"
#include "sight_nodes.h"
#include "sight_project.h"
#include "sight_undo.h"
#include "sight_widgets.h"
#include <cassert>
#include <iostream>
#include <string_view>
#include <sys/stat.h>
#include <vector>

#include "absl/strings/substitute.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_canvas.h"
#include "imgui_internal.h"

#define BACKGROUND_CONTEXT_MENU "BackgroundContextMenu"
#define LINK_CONTEXT_MENU "LinkContextMenu"
#define PIN_CONTEXT_MENU "PinContextMenu"
#define NODE_CONTEXT_MENU "NodeContextMenu"

static struct {
    ed::EditorContext* context = nullptr;
    // file path buf.
    char contextConfigFileBuf[NAME_BUF_SIZE * 2] = { 0 };

    bool contextMenuCreateNodeOpen = false;
    sight::SightNodePortHandle portForCreateNode;

    ImVec2 nextNodePosition{0,0};

    float lastSyncPositionTime = 0;

    bool isNextNodePositionEmpty(){
        return nextNodePosition.x == 0 && nextNodePosition.y == 0;
    }

    void resetAfterCreateNode(){
        contextMenuCreateNodeOpen = false;
        nextNodePosition = {0,0};
        portForCreateNode = {};
    }

}* g_ContextStatus;


namespace sight {

    namespace {

        void showLabel(const char* label, ImColor color) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
            auto size = ImGui::CalcTextSize(label);

            auto padding = ImGui::GetStyle().FramePadding;
            auto spacing = ImGui::GetStyle().ItemSpacing;

            ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

            auto rectMin = ImGui::GetCursorScreenPos() - padding;
            auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

            auto drawList = ImGui::GetWindowDrawList();
            drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
            ImGui::TextUnformatted(label);
        };

        void onConnect(SightNodeConnection* connection) {
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
        void OnDisconnect(SightNodeConnection* connection) {
            if (!connection) {
                return;
            }

            // notify left, right
            auto left = connection->findLeftPort();
            auto right = connection->findRightPort();
            auto isolate = currentUIStatus()->isolate;

            if (left && left->templateNodePort) {
                left->templateNodePort->onDisconnect(isolate, left, JsEventType::Connect, connection);
            }
            if (right && right->templateNodePort) {
                right->templateNodePort->onDisconnect(isolate, right, JsEventType::Connect, connection);
            }
        }

        void onNodePortAutoComplete(SightNodePort* port) {
            if (!port->templateNodePort) {
                return;
            }

            port->templateNodePort->onAutoComplete(currentUIStatus()->isolate, port, JsEventType::AutoComplete);
        }


        // node editor functions
        
        void checkResetCreateStatus(){
            if (g_ContextStatus->contextMenuCreateNodeOpen) {
                if (!ImGui::IsPopupOpen(BACKGROUND_CONTEXT_MENU)) {
                    g_ContextStatus->resetAfterCreateNode();
                }
            } else {
                if (g_ContextStatus->portForCreateNode) {
                    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                        g_ContextStatus->resetAfterCreateNode();
                    }
                }
            }
        }

        // this function need node editor be suspend
        void showContextMenu(uint nodeId, uint linkId, uint pinId) {
            auto showPortDebugInfo = [](SightNodePort const& item) {
                auto portInfoMsg = absl::Substitute("$0, $1", item.getId(), item.portName);
                dbg(portInfoMsg);
            };

            if (ImGui::BeginPopup(NODE_CONTEXT_MENU)) {
                if (ImGui::MenuItem("DebugInfo")) {
                    // show debug info.
                    dbg(nodeId);
                    auto nodePointer = currentGraph()->findNode(nodeId);
                    dbg(nodePointer);
                    if (nodePointer) {
                        auto nodeFunc = [&showPortDebugInfo](std::vector<SightNodePort> const& list) {
                            if (list.empty()) {
                                return ;
                            }

                            dbg(getNodePortTypeName(list.front().kind));
                            for( const auto& item: list){
                                showPortDebugInfo(item);
                            }
                        };

                        CALL_NODE_FUNC(nodePointer);
                    }
                } else if (ImGui::MenuItem("Copy")) {
                    dbg(nodeId);
                    auto nodePointer = currentGraph()->findNode(nodeId);
                    if (nodePointer) {
                        auto str = CopyText::from(*nodePointer);
                        ImGui::SetClipboardText(str.c_str());
                    }
                }
                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(PIN_CONTEXT_MENU)) {
                auto port = currentGraph()->findPort(pinId);
                if (ImGui::MenuItem("PortDebugInfo")) {
                    if (port) {
                        showPortDebugInfo(*port);
                    } else {
                        dbg("no-port-info");
                    }
                }
                if (ImGui::MenuItem("ShowFlow")) {
                    for( const auto& item: port->connections){
                        ed::Flow(item->connectionId);
                        dbg(item->connectionId);
                    }
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
                if (g_ContextStatus->isNextNodePositionEmpty()) {
                    g_ContextStatus->nextNodePosition = ImGui::GetMousePos();
                }
                for (auto& item : getCurrentNodeStatus()->templateAddressList) {
                    item.showContextMenu();
                }

                ImGui::Separator();
                if (ImGui::MenuItem("custom operations")) {
                    dbg("custom operations");
                }

                ImGui::EndPopup();
            }

        }

        void showNodePortIcon(IconType iconType, ImColor color, bool isConnected = false, int alpha = 255) {
            auto cursorPos = ImGui::GetCursorScreenPos();
            auto drawList = ImGui::GetWindowDrawList();
            constexpr int iconSize = SightNodeFixedStyle::iconSize;
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
            // auto alpha = 255;
            auto innerColor = ImColor(32, 32, 32, alpha);
            color = ImColor(color.Value.x, color.Value.y, color.Value.z, alpha / 255.f);

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
            // ImGui::Dummy(ImVec2(1,1));
        }

        /**
         * render a node
         * @param node
         * @param syncPositionTo  sync position to node editor ?
         * @param syncPositionFrom sync position from node editor ?
         * @return
         */
        int showNode(SightNode* node, bool syncPositionTo = false, bool syncPositionFrom = false) {
            if (!node) {
                return CODE_FAIL;
            }

            auto color = ImColor(0, 99, 160, 255);
            auto project = currentProject();
            auto templateNode = node->templateNode;
            if (!templateNode->isStyleInitialized()) {
                const_cast<SightJsNode*>(templateNode)->updateStyle();
            }
            auto& nodeStyle = templateNode->nodeStyle;

            // filter
            auto getAlpha = [](SightNodePort const& port) {
                constexpr int invalidAlpha = 48;
                constexpr int fullAlpha = 255;
                if (g_ContextStatus->portForCreateNode) {
                    auto filterPort = g_ContextStatus->portForCreateNode.get();
                    if (filterPort->kind == port.kind) {
                        return fullAlpha;
                    }

                    // check type
                    auto filterType = filterPort->getType();
                    uint portType = port.getType();
                    if (filterType > 0) {
                        auto leftType = filterType;
                        auto rightType = portType;
                        if (filterPort->kind == NodePortType::Input) {
                            std::swap(leftType, rightType);
                        }
                        return checkTypeCompatibility(leftType, rightType) ? fullAlpha : invalidAlpha;
                    }
                }

                return fullAlpha;
            };

            if (syncPositionTo && !node->position.isZero()) {
                ed::SetNodePosition(node->nodeId, getNodePos(node));
            }
            ed::BeginNode(node->nodeId);
            // ImGui::Dummy(ImVec2(7,5));
            auto chainInPort = node->chainInPort;
            auto [typeProcess, _] = project->findTypeInfo(chainInPort->type);

            if (chainInPort) {
                ed::BeginPin(chainInPort->id, ed::PinKind::Input);
                showNodePortIcon(typeProcess.style->iconType, typeProcess.style->color, chainInPort->isConnect(),
                                 getAlpha(*chainInPort));
                ed::EndPin();
                ImGui::SameLine();
            }

            ImGui::TextColored(ImVec4(0, 106, 113, 255), "%s", node->nodeName.c_str());

            auto chainOutPort = node->chainOutPort;
            if (chainOutPort) {
                ImGui::SameLine(templateNode->nodeStyle.width - SightNodeFixedStyle::iconSize);
                // ImGui::SameLine(0);
                ed::BeginPin(chainOutPort->id, ed::PinKind::Output);
                showNodePortIcon(typeProcess.style->iconType, typeProcess.style->color, chainOutPort->isConnect(),
                                 getAlpha(*chainOutPort));
                ed::EndPin();
            }
            ImGui::Dummy(ImVec2(0, SightNodeFixedStyle::iconTitlePadding));

            // fields
            for (auto& item : node->fields) {
                if (!item.options.show) {
                    continue;
                }

                auto& options = item.options;
                bool showErrorMsg = false;
                if (options.errorMsg.empty()) {
                    ImGui::Text("%*s", nodeStyle.fieldStype.maxCharSize, item.portName.c_str());
                } else {
                    ImGui::TextColored(currentUIStatus()->uiColors->errorText, "%*s", nodeStyle.fieldStype.maxCharSize, item.portName.c_str());
                    if (ImGui::IsItemHovered()) {
                        // show error msg
                        showErrorMsg = true;
                    }
                }
                ImGui::SameLine();
                showNodePortValue(&item, true, nodeStyle.fieldStype.inputWidth);

                if (showErrorMsg) {
                    ImGui::TextColored(currentUIStatus()->uiColors->errorText, "%8s", options.errorMsg.c_str());
                }
            }

            // inputPorts
            bool isAnyInputShow = false;
            bool isInputGroupShow = false;
            if (node->inputPorts.size() > 1) {
                ImGui::BeginGroup();
                isInputGroupShow = true;
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
                    auto alpha = getAlpha(item);
                    if (!find || !tmpTypeInfo.style) {
                        showNodePortIcon(IconType::Circle, color, item.isConnect(), alpha);
                    } else {
                        showNodePortIcon(tmpTypeInfo.style->iconType, tmpTypeInfo.style->color, item.isConnect(), alpha);
                    }

                    ImGui::SameLine();
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha / 255.f);
                    ImGui::Text("%s", item.portName.c_str());
                    ImGui::PopStyleVar();

                    if (item.type != IntTypeProcess && item.options.showValue) {
                        ImGui::SameLine();
                        showNodePortValue(&item, true, nodeStyle.inputStype.inputWidth);
                    }
                    ed::EndPin();
                    isAnyInputShow = true;
                }
                ImGui::EndGroup();
                ImGui::SameLine();
            }

            ImGui::BeginGroup();
            for (SightNodePort& item : node->outputPorts) {
                if (item.portName.empty() || !item.options.show) {
                    continue;     // do not show the title bar port.
                }

                float currentItemWidth = isNodePortShowValue(item) ? nodeStyle.outputStype.inputWidth : 0;
                // fill space
                currentItemWidth += SightNodeFixedStyle::iconSize;
                currentItemWidth += ImGui::CalcTextSize(std::string(nodeStyle.outputStype.maxCharSize + SightNodeFixedStyle::nameCharsPadding, 'A').c_str()).x;
                if (isAnyInputShow) {
                    currentItemWidth += nodeStyle.inputStype.totalWidth;
                }
                auto spaceX = ImGui::GetStyle().ItemSpacing.x;
                currentItemWidth += spaceX;
                if (isInputGroupShow) {
                    currentItemWidth += spaceX;
                }
                if (nodeStyle.width > 0) {
                    auto x = nodeStyle.width - currentItemWidth;
                    if (x != 0) {
                        ImGui::Dummy(ImVec2(x, 0));
                        ImGui::SameLine();
                    }
                }

                if (isNodePortShowValue(item)) {
                    showNodePortValue(&item, true, nodeStyle.outputStype.inputWidth);
                    ImGui::SameLine();
                }

                ed::BeginPin(item.id, ed::PinKind::Output);
                ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                ed::PinPivotSize(ImVec2(0, 0));
                // if (!node->templateNode->bothPortList.contains(item.portName)) {
                //     ImGui::Text("%8s", item.portName.c_str());
                //     ImGui::SameLine();
                // }
                auto alpha = getAlpha(item);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha / 255.f);
                ImGui::Text("%-*s", nodeStyle.outputStype.maxCharSize, item.portName.c_str());
                ImGui::PopStyleVar();
                ImGui::SameLine();

                // port icon
                // use fake type here
                auto [tmpTypeInfo, find] = project->findTypeInfo(item.type);
                if (!find || !tmpTypeInfo.style) {
                    showNodePortIcon(IconType::Circle, color, item.isConnect(), alpha);
                } else {
                    showNodePortIcon(tmpTypeInfo.style->iconType, tmpTypeInfo.style->color, item.isConnect(), alpha);
                }

                ed::EndPin();
            }

            ImGui::EndGroup();
            ed::EndNode();
            if (syncPositionFrom) {
                setNodePos(node, ed::GetNodePosition(node->nodeId));
            }
            
            return CODE_OK;
        }

        int showNodes(UIStatus const& uiStatus) {
            bool syncPositionTo = false, syncPositionFrom = false;
            auto now = ImGui::GetTime();
            if (g_ContextStatus->lastSyncPositionTime <= 0) {
                g_ContextStatus->lastSyncPositionTime = now;
                syncPositionTo = true;
            } else if (now - g_ContextStatus->lastSyncPositionTime >= 5) {
                g_ContextStatus->lastSyncPositionTime = now;
                dbg("try sync node position");
                syncPositionFrom = true;
            }

            auto graph = currentGraph();
            auto sightSettings = getSightSettings();

            // auto io = uiStatus.io;
            // ImGui::Text("FPS: %.2f (%.2gms)", io->Framerate, io->Framerate ? 1000.0f / io->Framerate : 0.0f);
            // ImGui::SameLine();
            ImGui::SameLine();
            if (ImGui::Button("Parse")) {
                dbg("Not impl");
            }
            if (!sightSettings->autoSave) {
                ImGui::SameLine();
                if (ImGui::Button("Save")) {
                    graph->save();
                }
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto Save", &(sightSettings->autoSave));
            if (graph->isBroken()) {
                ImGui::TextColored(uiStatus.uiColors->errorText, "Graph Broken!");
            }
            ImGui::Separator();

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            for (auto& node : graph->getNodes()) {
                showNode(&node, syncPositionTo, syncPositionFrom);
            }

            // Submit Links
            for (const auto& connection : graph->getConnections()) {
                ed::Link(connection.connectionId, connection.leftPortId(), connection.rightPortId(), ImColor(connection.leftColor));
            }

            //
            // 2) Handle interactions
            //

            // some flags
            bool flagBackgroundContextMenu = false;

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

                    if (inputPinId && outputPinId)     // both are valid, let's accept link
                    {
                        // ed::AcceptNewItem() return true when user release mouse button.
                        auto inputPort = graph->findPort(static_cast<int>(inputPinId.Get()));
                        auto outputPort = graph->findPort(static_cast<int>(outputPinId.Get()));
                        assert(inputPort != nullptr);
                        assert(outputPort != nullptr);

                        if (inputPort->getId() == outputPort->getId()) {
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        } else if (inputPort->kind == outputPort->kind) {
                            showLabel("same kind", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        } else if (!checkTypeCompatibility(inputPort->getType(), outputPort->getType())) {
                            showLabel("incompatibility type", ImColor(45, 32, 32, 180));
                            ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                        }
                        else {
                            if (ed::AcceptNewItem()) {
                                // Since we accepted new link, lets add one to our list of links.
                                int priority = 10;
                                if (inputPort->connectionsSize() >= 1) {
                                    priority = inputPort->connections.back()->priority - 1;
                                }

                                auto id = uiAddConnection(inputPort->getId(), outputPort->getId(), 0, priority);
                                if (id > 0) {
                                    // Draw new link.
                                    ed::Link(id, inputPinId, outputPinId);
                                } else {
                                    // may show something to user.
                                    switch (id) {
                                    case -1:
                                        dbg("left or right is invalid");
                                        break;
                                    case -2:
                                        dbg("same port id");
                                        break;
                                    case -3:
                                        dbg("same kind");
                                        break;
                                    case -4:
                                        dbg("left only can accept one connection");
                                        break;
                                    }
                                }
                            }
                        }

                        // You may choose to reject connection between these nodes
                        // by calling ed::RejectNewItem(). This will allow editor to give
                        // visual feedback by changing link thickness and color.
                    } 
                }

                ed::PinId pinId = 0;
                if (ed::QueryNewNode(&pinId)) {
                    showLabel("+ Create Node", ImColor(32, 45, 32, 180));
                    auto portHandle = graph->findPortHandle(static_cast<int>(pinId.Get()));
                    g_ContextStatus->portForCreateNode = portHandle;

                    if (ed::AcceptNewItem()) {
                        dbg("new node");
                        
                        if (portHandle) {
                            flagBackgroundContextMenu = true;
                        } else {
                            dbg("invalid port handle");
                        }
                    }   
                }
                
            }
            ed::EndCreate();     // Wraps up object creation action handling.

            // Handle deletion action
            if (ed::BeginDelete()) {
                // There may be many links marked for deletion, let's loop over them.
                ed::LinkId deletedLinkId;
                while (ed::QueryDeletedLink(&deletedLinkId)) {
                    // If you agree that link can be deleted, accept deletion.
                    if (ed::AcceptDeletedItem()) {
                        // Then remove link from your data.
                        auto connectionId = static_cast<int>(deletedLinkId.Get());
                        auto connection = currentGraph()->findConnection(connectionId);
                        OnDisconnect(connection);

                        recordUndo(UndoRecordType::Delete, connectionId);
                        graph->delConnection(connectionId, true, &(lastUndoCommand()->connectionData));
                    }

                    // You may reject link deletion by calling:
                    // ed::RejectDeletedItem();
                }

                ed::NodeId deletedNodeId;
                while (ed::QueryDeletedNode(&deletedNodeId)) {
                    // ask for node delete.
                    if (ed::AcceptDeletedItem()) {

                        recordUndo(UndoRecordType::Delete, 0, ed::GetNodePosition(deletedNodeId));
                        graph->delNode(static_cast<int>(deletedNodeId.Get()), &(lastUndoCommand()->nodeData));
                        // lastUndoCommand()->position = ed::GetNodePosition(deletedNodeId);
                    }
                }
            }
            ed::EndDelete();     // Wrap up deletion action

            auto openPopupPosition = ImGui::GetMousePos();     // this line must be before  ed::Suspend();

            // handle keyboard
            auto keybindings = uiStatus.keybindings;
            if (keybindings->controlKey.isKeyDown()) {
                
                if (keybindings->duplicateNode) {
                    dbg("duplicate node");
                    if (uiStatus.selection.selectedNodeOrLinks.size() == 1) {
                        auto tmpNode = uiStatus.selection.getSelectedNode();
                        if (tmpNode && tmpNode->templateNode) {
                            auto node = tmpNode->clone();
                            auto pos = ed::GetNodePosition(tmpNode->nodeId);
                            auto size = ed::GetNodeSize(tmpNode->nodeId);
                            auto nodeId = node->getNodeId();
                            ed::SetNodePosition(node->nodeId, ImVec2(pos.x, pos.y + size.y + 10));
                            uiAddNode(node);

                            // change selected
                            ed::SelectNode(nodeId);
                        }
                    } else {
                        std::vector<SightNode*> nodes;
                        std::vector<SightNodeConnection> connections;
                        uiStatus.selection.getSelected(nodes, connections);

                        if (!nodes.empty()) {
                            std::vector<SightNode*> targetNodes;
                            targetNodes.reserve(nodes.size());
                            for (const auto& item : nodes) {
                                targetNodes.push_back(item->clone(false));
                            }
                            regenerateId(targetNodes, connections, true);
                            ed::ClearSelection();
                            for (auto& item : targetNodes) {
                                ed::SelectNode(item->nodeId, true);
                            }
                            uiAddMultipleNodes(targetNodes, connections, openPopupPosition);
                        }
                    }
                    
                } else if (keybindings->paste) {
                    // try paste node
                    std::string_view str = ImGui::GetClipboardText();
                    if (!str.empty()) {
                        auto c = CopyText::parse(str);
                        if (c.type == CopyTextType::Node) {
                            SightNode* nodePointer = nullptr;
                            c.loadAsNode(nodePointer);
                            if (nodePointer) {
                                // add node
                                ed::SetNodePosition(nodePointer->getNodeId(), openPopupPosition);
                                uiAddNode(nodePointer);     // this function will delete nodePointer
                                nodePointer = nullptr;
                            }
                        } else if (c.type == CopyTextType::Multiple) {
                            std::vector<SightNode*> nodes;
                            std::vector<SightNodeConnection> connections;
                            c.loadAsMultiple(nodes, connections);
                            regenerateId(nodes, connections);
                            uiAddMultipleNodes(nodes, connections, openPopupPosition);
                        }
                    }
                }
            }
            if (keybindings->esc.isKeyDown()) {
                if (g_ContextStatus->contextMenuCreateNodeOpen) {
                    g_ContextStatus->resetAfterCreateNode();
                }
            }

            // right click
            static ed::NodeId contextNodeId = 0;
            static ed::LinkId contextLinkId = 0;
            static ed::PinId contextPinId = 0;

            ed::Suspend();
            if (ed::ShowNodeContextMenu(&contextNodeId)) {
                ImGui::OpenPopup(NODE_CONTEXT_MENU);
            } else if (ed::ShowPinContextMenu(&contextPinId)) {
                ImGui::OpenPopup(PIN_CONTEXT_MENU);
            } else if (ed::ShowLinkContextMenu(&contextLinkId)) {
                ImGui::OpenPopup(LINK_CONTEXT_MENU);
            } else if (ed::ShowBackgroundContextMenu()) {
                flagBackgroundContextMenu = true;
            }

            if (flagBackgroundContextMenu) {
                g_ContextStatus->nextNodePosition = openPopupPosition;
                g_ContextStatus->contextMenuCreateNodeOpen = true;
                ImGui::OpenPopup(BACKGROUND_CONTEXT_MENU);
            }

            showContextMenu(static_cast<uint>(contextNodeId.Get()),
                            static_cast<uint>(contextLinkId.Get()), static_cast<uint>(contextPinId.Get()));
            checkResetCreateStatus();

            ed::Resume();
            // End of interaction with editor.
            ed::End();

            if (uiStatus.needInit)
                ed::NavigateToContent(0);

            if (syncPositionFrom && sightSettings->autoSave) {
                graph->markDirty();
                graph->save();
            }

            return 0;
        }
    }


    int showNodeEditorGraph(UIStatus const& uiStatus) {
        ImVec2 startPos = {
            300, 20
        };
        auto windowSize = uiStatus.io->DisplaySize - startPos;
        ImGui::SetNextWindowPos(startPos, ImGuiCond_Once);
        ImGui::SetNextWindowSize(windowSize, ImGuiCond_Once);

        std::string windowTitle("Graph");
        auto graph = currentGraph();
        if (graph) {
            windowTitle += " - ";
            windowTitle += graph->getFilePath();
            windowTitle += "###Node Editor Graph";
        }

        ImGui::Begin(windowTitle.c_str(), nullptr,
                     ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoSavedSettings);
        if (graph) {
            showNodes(uiStatus);
        }

        ImGui::End();

        return CODE_OK;
    }

    ImVec2 getNodePos(SightNode* node) {
        return convert(node->position);
    }

    ImVec2 getNodePos(SightNode const& node) {
        return convert(node.position);
    }

    void setNodePos(SightNode* node, ImVec2 pos) {
        node->position = convert(pos);
    }

    void setNodePos(SightNode& node, ImVec2 pos) {
        node.position = convert(pos);
    }

    ImVec2 convert(Vector2 v) {
        return {v.x, v.y};
    }

    Vector2 convert(ImVec2 v) {
        return { v.x, v.y };
    }

    int initNodeEditor(bool nodeStatusFlag) {
        g_ContextStatus = new std::remove_pointer<decltype(g_ContextStatus)>::type();

        if (nodeStatusFlag) {
            sight::initNodeStatus();
        }

        return CODE_OK;
    }

    int destroyNodeEditor(bool full, bool nodeStatusFlag) {
        if (!g_ContextStatus) {
            return CODE_FAIL;
        }

        if (g_ContextStatus->context) {
            ed::DestroyEditor(g_ContextStatus->context);
            g_ContextStatus->context = nullptr;
        }

        if (full) {
            delete g_ContextStatus;
            g_ContextStatus = nullptr;
        }
        if (nodeStatusFlag) {
            destoryNodeStatus();
        }
        return CODE_OK;
    }

    int changeNodeEditorGraph(std::string_view pathWithoutExt) {
        ed::Config config;
        char* configFilePath = g_ContextStatus->contextConfigFileBuf;
        sprintf(configFilePath, "%s.json", pathWithoutExt.data());
        config.SettingsFile = configFilePath;
        g_ContextStatus->context = ed::CreateEditor(&config);
        g_ContextStatus->lastSyncPositionTime = 0;
        
        // ed::SetCurrentEditor(g_ContextStatus->context);
        // ed::EnableShortcuts(false);
        // ed::SetCurrentEditor(nullptr);

        return CODE_OK;
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

        auto callOnValueChange = [port]() {
            recordUndo(UndoRecordType::Update, port->getId());
            lastUndoCommand()->portValueData = port->oldValue;

            onNodePortValueChange(port);
        };

        auto& options = port->options;
        switch (type) {
        case IntTypeFloat:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat(labelBuf, &port->value.u.f, 0.5f, 0, 0, "%.3f", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeDouble:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            if (ImGui::InputDouble(labelBuf, &port->value.u.d, 0, 0, "%.6f", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeLong:
        case IntTypeInt:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragInt(labelBuf, &port->value.u.i, 1, 0, 0, "%d", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeString:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            bool showed = false;
            if (fromGraph) {

            } else {
                // only show in inspector
                if (!options.alternatives.empty()) {
                    std::string comboLabel = labelBuf;
                    comboLabel += ".combo";
                    if (ImGui::BeginCombo(comboLabel.c_str(), port->value.u.string, ImGuiComboFlags_NoArrowButton)) {
                        std::string filterLabel = comboLabel + ".filter";
                        static char filterText[NAME_BUF_SIZE] = { 0 };
                        ImGui::InputText(filterLabel.c_str(), filterText, std::size(filterText));
                        for (const auto& item : options.alternatives) {
                            if (strlen(filterText) > 0 && !startsWith(item, filterText)) {
                                continue;
                            }
                            if (ImGui::Selectable(item.c_str(), item == port->value.u.string)) {
                                snprintf(port->value.u.string, std::size(port->value.u.string), "%s", item.c_str());
                                // onNodePortValueChange(port);
                                callOnValueChange();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    showed = true;
                }
            }

            if (!showed) {
                ImGui::SetNextItemWidth(width);
                if (ImGui::InputText(labelBuf, port->value.u.string, std::size(port->value.u.string), flags)) {
                }
                if (ImGui::IsItemActivated()) {
                    onNodePortAutoComplete(port);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    // onNodePortValueChange(port);
                    callOnValueChange();
                }
            }

            break;
        }
        case IntTypeBool:
        {
            if (checkBox(labelBuf, &port->value.u.b, options.readonly)) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeColor:
        {
            int flags = options.readonly ? ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs : 0;
            if (ImGui::ColorEdit3(labelBuf, port->value.u.vector4, flags)) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeVector3:
        {
            // fixme: vector3 and vector4 has a value change bug.
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat3(labelBuf, port->value.u.vector3, 1, 0, 0, "%.3f", flags)) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeVector4:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat4(labelBuf, port->value.u.vector4, 1, 0, 0, "%.3f", flags)) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeChar:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            ImGui::SetNextItemWidth(SightNodeFixedStyle::charTypeLength);
            if (ImGui::InputText(labelBuf, port->value.u.string, 2, 0)) {
                // onNodePortValueChange(port);
                callOnValueChange();
            }
            break;
        }
        case IntTypeLargeString:
        {
            if (fromGraph) {
                ImGui::Text("%s", port->value.u.largeString.pointer);
                helpMarker("Please edit it in external editor or Inspector window.");
            } else {
                int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
                flags |= ImGuiInputTextFlags_CallbackResize;
                auto nWidth = width + width / 2.0f;
                static auto callback = [](ImGuiInputTextCallbackData* data) {
                    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                        auto value = (SightNodeValue*)data->UserData;
                        auto& largeString = value->u.largeString;
                        // request resize
                        // dbg("request resize", largeString.bufferSize, data->BufSize, data->BufTextLen);
                        // if (data->BufSize != largeString.bufferSize) {
                        //     value->stringCheck(data->BufSize);
                        // }
                        if (value->stringCheck(data->BufTextLen)) {
                            data->Buf = value->u.largeString.pointer;
                            dbg("resize", largeString.bufferSize, data->BufSize, data->BufTextLen);
                        }
                    }
                    return 0;
                };
                if (ImGui::InputTextMultiline(labelBuf, port->value.u.largeString.pointer, port->value.u.largeString.bufferSize,
                                              ImVec2(nWidth, 150), flags, callback, &port->value)) {
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    // onNodePortValueChange(port);
                    callOnValueChange();
                }
            }
            break;
        }
        case IntTypeButton:
        {
            // todo power up, multiple buttons.
            if (ImGui::Button("button")) {
                if (port->templateNodePort) {
                    port->templateNodePort->onClick(currentUIStatus()->isolate, port);
                }
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
                    typeInfo.render(labelBuf, port, callOnValueChange);
                }
            } else {
                ImGui::LabelText(labelBuf, "Unknown type of %d", port->getType());
            }
            break;
        }
        }
    }

    void nodeEditorFrameBegin() {
        ed::SetCurrentEditor(g_ContextStatus->context);
    }

    void nodeEditorFrameEnd() {
        ed::SetCurrentEditor(nullptr);
    }

    void SightNodeTemplateAddress::showContextMenu() {
        if (children.empty()) {
            if (ImGui::MenuItem(name.c_str())) {
                //
                // dbg("it will create a new node.", this->name);
                auto node = this->templateNode->instantiate();
                auto nodeId = node->getNodeId();
                node->position = convert(g_ContextStatus->nextNodePosition);
                ed::SetNodePosition(nodeId, g_ContextStatus->nextNodePosition);
                if (uiAddNode(node) == CODE_OK) {
                    node = nullptr;
                    // check port
                    auto graph = currentGraph();
                    if (g_ContextStatus->portForCreateNode) {
                        node = graph->findNode(nodeId);
                        auto port = g_ContextStatus->portForCreateNode.get();
                        SightNodePort* tmp = nullptr;
                        if (port->isTitleBarPort()) {
                            // connect next title bar
                            tmp = node->getOppositeTitleBarPort(port->kind);
                        } else {
                            // match the type
                            tmp = node->getOppositePortByType(port->kind, port->getType());
                        }

                        if (tmp) {
                            // create connection
                            auto connectionId = graph->createConnection(port->getId(), tmp->getId());
                            if (connectionId > 0) {
                                recordUndo(UndoRecordType::Create, connectionId);
                            }
                        }
                    }
                }
            }
        } else {
            if (ImGui::BeginMenu(name.c_str())) {
                for (auto& item : children) {
                    item.showContextMenu();
                }

                ImGui::EndMenu();
            }
        }
    }

    int uiAddNode(SightNode* node, bool freeMemory) {

        currentGraph()->addNode(*node);
        recordUndo(UndoRecordType::Create, node->getNodeId(), convert(node->position));
        if (freeMemory) {
            delete node;
        }
        return CODE_OK;
    }

    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection> const& connections, ImVec2 startPos, bool freeMemory, bool selectNode) {
        getRuntimeId(StartOrStop::Start);
        if (!nodes.empty()) {
            // position
            // find left-most node.
            SightNode* leftNode = nullptr;
            for (auto& item : nodes) {
                if (leftNode) {
                    auto x = leftNode->position.x - item->position.x;
                    auto y = leftNode->position.y - item->position.y;

                    if (x > 0) {
                        leftNode = item;
                    }
                } else {
                    leftNode = item;
                }
            }

            if (selectNode) {
                ed::ClearSelection();
            }

            // add
            auto leftNodePos = leftNode->position;
            setNodePos(leftNode, startPos);
            for (auto& item : nodes) {
                if (item != leftNode) {
                    // fix pos
                    auto pos = startPos + convert(item->position - leftNodePos);
                    setNodePos(item, pos);
                }
                auto nodeId = item->getNodeId();
                ed::SetNodePosition(nodeId, getNodePos(item));
                uiAddNode(item, freeMemory);
                if (selectNode) {
                    ed::SelectNode(nodeId, true);
                }
            }
            if (freeMemory) {
                nodes.clear();
            }
        }
        if (!connections.empty()) {
            for (const auto& item : connections) {
                // graph->addConnection(item);
                // graph->createConnection(item.left, item.right, item.connectionId, item.priority);
                uiAddConnection(item.left, item.right, item.connectionId, item.priority);
            }
        }
        getRuntimeId(StartOrStop::Stop);
        return CODE_OK;
    }

    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection*> const& connections, ImVec2 startPos, bool freeMemory, bool selectNode) {
        std::vector<SightNodeConnection> c;
        if (!connections.empty()) {
            c.reserve(connections.size());
            for( const auto& item: connections){
                c.push_back(*item);
            }
        }
        return uiAddMultipleNodes(nodes, c, startPos, freeMemory, selectNode);
    }

    int uiAddConnection(uint left, uint right, uint id, int priority) {
        auto graph = currentGraph();
        auto connectionId = graph->createConnection(left, right, id, priority);
        if (connectionId > 0) {
            auto connection = graph->findConnection(id);
            onConnect(connection);
            recordUndo(UndoRecordType::Create, connectionId);
        }
        return connectionId;
    }

    void uiChangeGraph(const char* path) {
        auto g = currentGraph();

        auto openGraphFunc = [path]() {
            char tmp[NAME_BUF_SIZE]{ 0 };
            auto t = currentProject()->openGraph(path, false, tmp);
            if (t) {
                // open succeed.
                changeNodeEditorGraph(tmp);
            }
        };
        if (g == nullptr || !g->editing) {
            openGraphFunc();
            return;
        }

        std::string stringPath(path);
        openSaveModal("Save File?", "Current Graph do not save, do you want save file?", [stringPath, openGraphFunc, g](SaveOperationResult r) {
            if (r == SaveOperationResult::Cancel) {
                return;
            } else if (r == SaveOperationResult::Save) {
                g->save();
            }
            openGraphFunc();
        });
    }

    bool isNodeEditorReady() {
        return g_ContextStatus->context;
    }
}