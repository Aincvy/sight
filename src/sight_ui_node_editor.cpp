#include "sight_defines.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include "IconsMaterialDesign.h"
#include "sight_log.h"
#include "sight_util.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "imgui_canvas.h"
#include "imgui_internal.h"
#include "imgui_node_editor.h"
#include "imgui_node_editor_internal.h"

#include "sight_ui_node_editor.h"
#include "sight_external_widgets.h"
#include "sight.h"
#include "sight_address.h"
#include "sight_keybindings.h"
#include "sight_colors.h"
#include "sight_node.h"
#include "sight_node_graph.h"
#include "sight_project.h"
#include "sight_undo.h"
#include "sight_widgets.h"
#include "sight_js.h"
#include "sight_js_parser.h"
#include "sight_ui.h"
#include "sight_event_bus.h"

#include <cassert>
#include <iostream>
#include <iterator>
#include <string_view>
#include <sys/stat.h>
#include <vector>

#include "absl/strings/substitute.h"


#define BACKGROUND_CONTEXT_MENU "BackgroundContextMenu"
#define LINK_CONTEXT_MENU "LinkContextMenu"
#define PIN_CONTEXT_MENU "PinContextMenu"
#define NODE_CONTEXT_MENU "NodeContextMenu"
#define COMPONENT_CONTEXT_MENU "ComponentContextMenu"

namespace ed = ax::NodeEditor;

static struct {
    ed::EditorContext* context = nullptr;
    // file path buf.
    char contextConfigFileBuf[NAME_BUF_SIZE * 2] = { 0 };
    uint componentForWhich = 0;     // which node?

    bool contextMenuCreateNodeOpen = false;
    bool needSaveGraph = false;
    bool createComponents = false;
    sight::SaveReason lastSaveGraphReason = sight::SaveReason::Automatic;

    sight::SightNodePortHandle portForCreateNode;

    ImVec2 nextNodePosition{ 0, 0 };
    float lastSyncPositionTime = 0;

    // pointer to project's graphOutputJsonConfig
    sight::SightNodeGraphOutputJsonConfig* graphOutputJsonConfig =  nullptr;
    char outputJsonFilePathBuf[NAME_BUF_SIZE * 2] = { 0 };
    bool outputJsonFilePathError = false;
    uint lastMarkedNodeId = 0;
    uint lastClickedNodeId = 0;

    uint lastMarkedPortId = 0;
    // both means invalid port type
    sight::NodePortType lastMarkedPortType = sight::NodePortType::Both;


    bool lastMarkNodeIsManually = false;
    
    bool isNextNodePositionEmpty() {
        return nextNodePosition.x == 0 && nextNodePosition.y == 0;
    }

    void resetAfterCreateNode() {
        contextMenuCreateNodeOpen = false;
        nextNodePosition = { 0, 0 };
        portForCreateNode = {};
        createComponents = false;
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


        void tryAutoMarkNode(uint nodeId) {
            if (g_ContextStatus->lastMarkedNodeId == 0 || !g_ContextStatus->lastMarkNodeIsManually) {
                g_ContextStatus->lastMarkedNodeId = nodeId;
            }
        }

        void clearMarkNodeInfo() {
            g_ContextStatus->lastMarkedNodeId = 0;
            g_ContextStatus->lastMarkNodeIsManually = false;
        }

        void tryMarkPort(SightNodePort const& port) {
            g_ContextStatus->lastMarkedPortId = port.getId();
            g_ContextStatus->lastMarkedPortType = port.kind;
        }
        
        // node editor functions

        void checkResetCreateStatus() {
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
        void showContextMenu(uint nodeId, uint linkId, uint pinId, SightNodeGraph* graph, ImVec2 const& mousePos) {
            auto showPortDebugInfo = [](SightNodePort const& item) {
                auto portInfoMsg = absl::Substitute("$0, $1", item.getId(), item.portName);
                logDebug(portInfoMsg);
            };

            if (ImGui::BeginPopup(NODE_CONTEXT_MENU)) {
                // logDebug(nodeId);
                if (ImGui::MenuItem("DebugInfo")) {
                    // show debug info.
                    auto nodePointer = currentGraph()->findNode(nodeId);
                    // logDebug(nodePointer);
                    if (nodePointer) {
                        auto nodeFunc = [&showPortDebugInfo](std::vector<SightNodePort> const& list) {
                            if (list.empty()) {
                                return;
                            }

                            logDebug(getNodePortTypeName(list.front().kind));
                            for (const auto& item : list) {
                                showPortDebugInfo(item);
                            }
                        };

                        CALL_NODE_FUNC(nodePointer);
                    }
                } else if (ImGui::MenuItem("Copy")) {
                    auto nodePointer = currentGraph()->findNode(nodeId);
                    if (nodePointer) {
                        auto str = CopyText::from(*nodePointer);
                        ImGui::SetClipboardText(str.c_str());
                    }
                }
                if (ImGui::MenuItem("Mark")) {
                    g_ContextStatus->lastMarkedNodeId = nodeId;
                }

                auto markedNodeId = g_ContextStatus->lastMarkedNodeId;
                if (ImGui::MenuItem("Replace From", nullptr, false, markedNodeId != 0 && markedNodeId != nodeId)) {
                    if (markedNodeId == 0) {
                        logError("Replace can't be called before mark a node");
                        return;
                    }
                    if (markedNodeId == nodeId) {
                        logError("Replace can't be called on the same node");
                        return;
                    }

                    if (replaceNode(currentGraph(), nodeId, markedNodeId)) {
                        g_ContextStatus->lastMarkedNodeId = 0;
                        recordUndo(UndoRecordType::Replace, nodeId);
                        auto undo = lastUndoCommand();
                        undo->anyThingId = markedNodeId;
                        undo->anyThingId2 = nodeId;

                        g_ContextStatus->lastSyncPositionTime = 0;
                        // ed::BreakLinks(ed::NodeId(markedNodeId));
                        // ed::BreakLinks(ed::NodeId(nodeId));
                    } else {
                        logError("Replace failed");
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
                        logDebug("no-port-info");
                    }
                }
                if (ImGui::MenuItem("ShowFlow")) {
                    for (const auto& item : port->connections) {
                        ed::Flow(item->connectionId);
                        logDebug(item->connectionId);
                    }
                } else if (ImGui::MenuItem("MarkPort")) {
                    tryMarkPort(*port);
                }

                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(LINK_CONTEXT_MENU)) {
                auto connection = graph->findConnection(linkId);
                if (!connection) {
                    logError("can't find connection: $0", linkId);
                    ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                    return;
                }

                auto markedNodeId = g_ContextStatus->lastMarkedNodeId;
                if (ImGui::MenuItem("insertAtMiddle", nullptr, false, markedNodeId != 0)) {
                    if (graph->insertNodeAtConnectionMid(markedNodeId, linkId)) {
                        ed::SetNodePosition(markedNodeId, mousePos);

                        clearMarkNodeInfo();
                    }
                }
                //
                auto lastMarkedPortId = g_ContextStatus->lastMarkedPortId;
                if (lastMarkedPortId > 0) {
                    auto lastMarkedPortType = g_ContextStatus->lastMarkedPortType;
                    if (lastMarkedPortType == NodePortType::Input || lastMarkedPortType == NodePortType::Output) {
                        bool changed = false;
                        if (ImGui::MenuItem("ChangeLeftToMark")) {
                            changed = connection->changeLeft(lastMarkedPortId);
                        } else if (ImGui::MenuItem("ChangeRightToMark")) {
                            changed = connection->changeRight(lastMarkedPortId);
                        }

                        if (changed) {
                            g_ContextStatus->lastMarkedPortId = 0;
                            g_ContextStatus->lastMarkedPortType = NodePortType::Both;
                        } 
                    }
                }

                ImGui::EndPopup();
            }
            if (ImGui::BeginPopup(BACKGROUND_CONTEXT_MENU)) {
                if (g_ContextStatus->isNextNodePositionEmpty()) {
                    g_ContextStatus->nextNodePosition = ImGui::GetMousePos();
                }
                g_ContextStatus->componentForWhich = 0;
                for (auto& item : currentNodeStatus()->templateAddressList) {
                    item.showContextMenu(g_ContextStatus->createComponents);
                }

                ImGui::Separator();
                if (ImGui::MenuItem("custom operations")) {
                    logDebug("custom operations");
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
                drawList->PathBezierCubicCurveTo(
                    ImVec2(left, top),
                    ImVec2(left, top),
                    ImVec2(left, top) + ImVec2(rounding, 0));
                drawList->PathLineTo(tip_top);
                drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
                drawList->PathBezierCubicCurveTo(
                    tip_right,
                    tip_right,
                    tip_bottom + (tip_right - tip_bottom) * tip_round);
                drawList->PathLineTo(tip_bottom);
                drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
                drawList->PathBezierCubicCurveTo(
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

        int getAlpha(SightNodePort const& port) {
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
        }


        void showNodePortsWithEd(SightNode* node, SightNodeStyle const& nodeStyle) {
            auto project = currentProject();
            auto color = ImColor(0, 99, 160, 255);

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

            // outputs
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

                if (item.options.dynamic) {
                    // dynamic port
                    showDynamicRootPort(item, nodeStyle.outputStype.maxCharSize);
                } else {
                    // common port
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
            }

            ImGui::EndGroup();
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
                const_cast<SightJsNode*>(templateNode)->updateStyle();     // try to optimization, but ...
            }
            auto& nodeStyle = templateNode->nodeStyle;

            // filter

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

            showNodePortsWithEd(node, nodeStyle);

            showNodeComponents(node->componentContainer, node, node->graph, node->getNodeId(), false);
            ed::EndNode();
            if (syncPositionFrom) {
                setNodePos(node, ed::GetNodePosition(node->nodeId));
            }

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                // logDebug("clicked node: $0", node->getNodeId());
                g_ContextStatus->lastClickedNodeId = node->getNodeId();
            }

            return CODE_OK;
        }

        void handleNodeEditorKeyboard(UIStatus const& uiStatus, SightNodeGraph* graph) {
            auto mousePos = ImGui::GetMousePos();
            auto canvasMousePos = ed::ScreenToCanvas(mousePos);

            // handle keyboard
            auto keybindings = uiStatus.keybindings;
            if (keybindings->controlKey.isKeyDown()) {

                if (keybindings->duplicateNode) {
                    logDebug("duplicate node");
                    if (uiStatus.selection.selectedNodeOrLinks.size() == 1) {
                        auto tmpNode = uiStatus.selection.getSelectedNode();
                        if (tmpNode) {
                            auto node = graph->deepClone(tmpNode);
                            auto pos = ed::GetNodePosition(tmpNode->nodeId);
                            auto size = ed::GetNodeSize(tmpNode->nodeId);
                            auto nodeId = node->getNodeId();
                            ed::SetNodePosition(node->nodeId, ImVec2(pos.x, pos.y + size.y + 10));
                            recordUndo(UndoRecordType::Create, node->getNodeId(), convert(node->position));

                            // change selected
                            ed::SelectNode(nodeId);

                            tryAutoMarkNode(nodeId);
                        } else {
                            logError("cannot find node by id: $0", *uiStatus.selection.selectedNodeOrLinks.begin());
                        }
                    } else {
                        logError("not supported yet!");
                        // std::vector<SightNode*> nodes;
                        // std::vector<SightNodeConnection> connections;
                        // uiStatus.selection.getSelected(nodes, connections);

                        // if (!nodes.empty()) {
                        //     std::vector<SightNode*> targetNodes;
                        //     targetNodes.reserve(nodes.size());
                        //     for (const auto& item : nodes) {
                        //         targetNodes.push_back(item->clone(false));
                        //     }
                        //     regenerateId(targetNodes, connections, true);
                        //     ed::ClearSelection();
                        //     for (auto& item : targetNodes) {
                        //         ed::SelectNode(item->nodeId, true);
                        //     }
                        //     uiAddMultipleNodes(targetNodes, connections, openPopupPosition);
                        // }
                    }

                } else if (keybindings->paste) {
                    // try paste node
                    std::string_view str = ImGui::GetClipboardText();
                    if (!str.empty()) {
                        auto c = CopyText::parse(str);
                        if (c.type == CopyTextType::Node) {
                            SightNode* nodePointer = nullptr;
                            c.loadAsNode(nodePointer, graph);
                            if (nodePointer) {
                                // add node
                                ed::SetNodePosition(nodePointer->getNodeId(), canvasMousePos);
                                uiAddNode(nodePointer);     // this function will delete nodePointer
                                tryAutoMarkNode(nodePointer->getNodeId());
                            }
                        } else if (c.type == CopyTextType::Multiple) {
                            std::vector<SightNode*> nodes;
                            std::vector<SightNodeConnection> connections;
                            c.loadAsMultiple(nodes, connections, graph);
                            regenerateId(nodes, connections, true);
                            uiAddMultipleNodes(nodes, connections, canvasMousePos);
                        }
                    }
                } else if (keybindings->_delete) {
                    // delete
                    logDebug("delete");
                    auto& selected = uiStatus.selection.selectedNodeOrLinks;
                    if (selected.size() == 1) {
                        auto id = *selected.begin();
                        auto thing = graph->findSightAnyThing(id);
                        if (thing.type == SightAnyThingType::Node) {
                            ed::DeleteNode(id);
                        } else if (thing.type == SightAnyThingType::Connection) {
                            ed::DeleteLink(id);
                        }
                    }
                } else if (keybindings->insertNode.isKeyReleased()) {
                    auto markedNodeId = g_ContextStatus->lastMarkedNodeId;
                    if (markedNodeId <= 0) {
                        logError("need mark a node first.");
                    } else {
                        // find a connection
                        uint connectionId = ed::GetHoveredLink().Get();
                        if (connectionId <= 0) {
                            // try find from selection
                            auto const& tmp = uiStatus.selection.selectedNodeOrLinks;
                            if (tmp.size() == 1) {
                                // only works when select 1 connection
                                uint id = *tmp.begin();
                                auto anyThing = graph->findSightAnyThing(id);
                                if (anyThing.type == SightAnyThingType::Connection) {
                                    connectionId = id;
                                }
                            }
                        }

                        if (connectionId <= 0) {
                            logError("need select 1 and only 1 connection!");
                        } else {
                            if (graph->insertNodeAtConnectionMid(markedNodeId, connectionId)) {
                                ed::SetNodePosition(markedNodeId, canvasMousePos);

                                clearMarkNodeInfo();
                            }
                        }
                    }
                } else if (keybindings->markNode.isKeyDown()) {
                    uint hoveredNode = ed::GetHoveredNode().Get();
                    if (hoveredNode == 0) {
                        auto const& ids = uiStatus.selection.selectedNodeOrLinks;
                        if (ids.size() != 1) {
                            logWarning("mark node shortcut only valid when select 1 node.");
                        } else {
                            auto tmpId = *ids.begin();
                            auto a = graph->findSightAnyThing(tmpId);
                            if (a.type == SightAnyThingType::Node) {
                                hoveredNode = tmpId;
                            }
                        }
                    }

                    if (hoveredNode == 0) {
                        logWarning("mark node shortcut only valid when select 1 node.");
                    } else {
                        g_ContextStatus->lastMarkedNodeId = hoveredNode;
                        g_ContextStatus->lastMarkNodeIsManually = true;
                    }
                }
            }
            if (keybindings->esc.isKeyDown()) {
                if (g_ContextStatus->contextMenuCreateNodeOpen) {
                    g_ContextStatus->resetAfterCreateNode();
                }
            } else if (keybindings->detachNode.isKeyDown()) {
                auto clickedNode = g_ContextStatus->lastClickedNodeId;
                if (clickedNode > 0) {
                    recordUndo(UndoRecordType::DetachNode, clickedNode);
                    if (graph->detachNodeConnections(clickedNode)) {
                        tryAutoMarkNode(clickedNode);
                    }
                }
            } 
        }

        int showNodes(UIStatus const& uiStatus, SightNodeGraph* graph) {
            bool syncPositionTo = false, syncPositionFrom = false;
            auto now = ImGui::GetTime();
            if (g_ContextStatus->lastSyncPositionTime <= 0) {
                g_ContextStatus->lastSyncPositionTime = now;
                syncPositionTo = true;
            } else if (now - g_ContextStatus->lastSyncPositionTime >= 5) {
                g_ContextStatus->lastSyncPositionTime = now;

                syncPositionFrom = true;
            }

            auto sightSettings = getSightSettings();

            // ImGui::SameLine();
            // if (ImGui::Button("Parse")) {
            //     graph->asyncParse();
            // }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_SAVE)) {
                // save button
                syncPositionFrom = true;
                syncPositionTo = false;

                graph->markDirty();
                g_ContextStatus->needSaveGraph = true;
                g_ContextStatus->lastSaveGraphReason = SaveReason::User;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Auto Save", &(sightSettings->autoSave));
            if (graph->isBroken()) {
                ImGui::SameLine();
                ImGui::TextColored(uiStatus.uiColors->errorText, "Graph Broken: %s", graph->getBrokenReason().c_str());
                return CODE_FAIL;
            }
            ImGui::SameLine();

            static float zoomValue = 1;
            constexpr const float zoomStep = 0.25f;
            constexpr const float zoomMin = 0.25f;
            constexpr const float zoomMax = 3.25f;

            if (ImGui::Button(ICON_MD_ZOOM_IN)) {
                if ((zoomValue += zoomStep) > zoomMax) {
                    zoomValue = zoomMax;
                    toast("Zoom value max", "");
                }
                ed::SetCurrentZoom(zoomValue);
                trace("$0, $1", ed::GetCurrentZoom(), zoomValue);
            }
            helpMarker("Zoom In");
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_ZOOM_OUT)) {
                if ((zoomValue -= zoomStep) < zoomMin) {
                    zoomValue = zoomMin;
                    toast("Zoom value min", "");
                }
                ed::SetCurrentZoom(zoomValue);
                trace("$0, $1", ed::GetCurrentZoom(), zoomValue);
            }
            helpMarker("Zoom Out");
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_CROP_FREE)) {
                ed::SetCurrentZoom(zoomValue = 1);
                trace("Reset Zoom: $0, $1", ed::GetCurrentZoom(), zoomValue);
            }
            helpMarker("Reset Zoom");

            // SaveAsJson
            ImGui::SameLine();
            if (ImGui::Button("SaveAsJson")) {
                if (strlen(g_ContextStatus->outputJsonFilePathBuf) <= 0) {
                    auto tmpPath = graph->getSaveAsJsonPath();
                    if (!tmpPath.empty()) {
                        // write to buf
                        sprintf(g_ContextStatus->contextConfigFileBuf, "%s", tmpPath.c_str());
                    }
                }
                currentUIStatus()->windowStatus.graphOutputJsonConfigWindow =true;
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_SHIFT)) {
                logDebug("shift button");
            }

            ImGui::Separator();

            // Start interaction with editor.
            ed::Begin("My Editor", ImVec2(0.0, 0.0f));

            // nodes
            graph->loopOf([syncPositionTo, syncPositionFrom](SightNode* node) {
                showNode(node, syncPositionTo, syncPositionFrom);
            });

            // Submit Links
            graph->loopOf([](SightNodeConnection* connection) {
                ed::Link(connection->connectionId, connection->leftPortId(),
                         connection->rightPortId(), ImColor(connection->leftColor));
            });


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
                        } else {
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
                                        logDebug("left or right is invalid");
                                        break;
                                    case -2:
                                        logDebug("same port id");
                                        break;
                                    case -3:
                                        logDebug("same kind");
                                        break;
                                    case -4:
                                        logDebug("left only can accept one connection");
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
                        logDebug("new node");

                        if (portHandle) {
                            flagBackgroundContextMenu = true;
                        } else {
                            logDebug("invalid port handle");
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
                        uiDelConnection(deletedLinkId.Get());
                    }

                    // You may reject link deletion by calling:
                    // ed::RejectDeletedItem();
                }

                ed::NodeId deletedNodeId;
                while (ed::QueryDeletedNode(&deletedNodeId)) {
                    // ask for node delete.
                    if (ed::AcceptDeletedItem()) {

                        // recordUndo(UndoRecordType::Delete, 0, ed::GetNodePosition(deletedNodeId));
                        // graph->delNode(static_cast<int>(deletedNodeId.Get()), &(lastUndoCommand()->nodeData));
                        uiDelNode(deletedNodeId.Get());
                    }
                }
            }
            ed::EndDelete();     // Wrap up deletion action

            auto openPopupPosition = ImGui::GetMousePos();     // this line must be before  ed::Suspend();

            
            // right click
            static ed::NodeId contextNodeId = 0;
            static ed::LinkId contextLinkId = 0;
            static ed::PinId contextPinId = 0;
            bool needOpenContextMenu = false;

            ed::Suspend();
            if (ed::ShowNodeContextMenu(&contextNodeId)) {
                needOpenContextMenu = true;
                ImGui::OpenPopup(NODE_CONTEXT_MENU);
            } else if (ed::ShowPinContextMenu(&contextPinId)) {
                needOpenContextMenu = true;
                ImGui::OpenPopup(PIN_CONTEXT_MENU);
            } else if (ed::ShowLinkContextMenu(&contextLinkId)) {
                needOpenContextMenu = true;
                ImGui::OpenPopup(LINK_CONTEXT_MENU);
            } else if (ed::ShowBackgroundContextMenu()) {
                needOpenContextMenu = true;
                flagBackgroundContextMenu = true;
            }

            if (needOpenContextMenu) {
                g_ContextStatus->componentForWhich = 0;
            }

            if (flagBackgroundContextMenu) {
                g_ContextStatus->nextNodePosition = openPopupPosition;
                g_ContextStatus->contextMenuCreateNodeOpen = true;
                ImGui::OpenPopup(BACKGROUND_CONTEXT_MENU);
            }

            showContextMenu(static_cast<uint>(contextNodeId.Get()), static_cast<uint>(contextLinkId.Get()),
                            static_cast<uint>(contextPinId.Get()), graph, openPopupPosition);
            checkResetCreateStatus();

            ed::Resume();
            // End of interaction with editor.
            ed::End();

            if (uiStatus.needInit) {
                // ed::NavigateToContent(0);
            }

            if (syncPositionFrom && sightSettings->autoSave) {
                g_ContextStatus->needSaveGraph = true;
            }

            return CODE_OK;
        }
    }


    int showNodeEditorGraph(UIStatus const& uiStatus) {

        std::string windowTitle("Graph");
        auto graph = currentGraph();
        if (graph) {
            windowTitle += " - ";
            windowTitle += graph->getFilePath();
            windowTitle += "###Node Editor Graph";
        }

        bool flag = ImGui::Begin(windowTitle.c_str(), nullptr,
                                 ImGuiWindowFlags_NoCollapse |
                                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                     ImGuiWindowFlags_NoBringToFrontOnFocus);
        if (flag) {
            if (graph) {
                showNodes(uiStatus, graph);

                handleNodeEditorKeyboard(uiStatus, graph);

                // do some reset work
                g_ContextStatus->lastClickedNodeId = 0;
            } else {
                ImGui::Text("Please open a graph.");
            }
        }

        ImGui::End();

        if (g_ContextStatus->needSaveGraph) {
            g_ContextStatus->needSaveGraph = false;
            uiSaveGraph();
        }

        if (uiStatus.windowStatus.graphOutputJsonConfigWindow) {
            if (showOutputJsonConfigPanel(graph, g_ContextStatus->graphOutputJsonConfig)) {
                // output json
                auto pathBuf = g_ContextStatus->outputJsonFilePathBuf;
                if (strlen(pathBuf) <= 0) {
                    logError("output json need a file path!");
                } else {
                    if (pathBuf != graph->getSaveAsJsonPath()) {
                        graph->addSaveAsJsonHistory(pathBuf);
                    }

                    addJsCommand(JsCommandType::GraphToJsonData, CommandArgs::copyFrom(pathBuf));
                    currentProject()->saveConfigFile();
                }

                currentUIStatus()->windowStatus.graphOutputJsonConfigWindow = false;
            }
        }

        return CODE_OK;
    }

    void prepareNodeEditorToShow() {

        auto p = currentProject();
        g_ContextStatus->graphOutputJsonConfig = p->getGraphOutputJsonConfig();
    
    }

    ImVec2 getNodePos(SightNode* node) {
        return convert(node->position);
    }

    ImVec2 getNodePos(SightNode const& node) {
        return convert(node.position);
    }

    void setNodePos(SightNode* node, ImVec2 pos) {
        // logDebug("try set node pos: $0", node->nodeName);
        const auto& oldPos = node->position;
        if (oldPos.x == pos.x && oldPos.y == pos.y) {
            return;
        }
        auto target = convert(pos);
        node->position = target;
        node->graph->markDirty();

        // logDebug("set node pos, name: $0, pos: $1, $2", node->nodeName, target.x, target.y);
    }

    void setNodePos(SightNode& node, ImVec2 pos) {
        node.position = convert(pos);
    }

    ImVec2 convert(Vector2 v) {
        return { v.x, v.y };
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

        return CODE_OK;
    }

    void showNodePortValue(SightNodePort* port, bool fromGraph, int width, int type) {
        assert(width > 0);

        // do not handle type-list-port
        assert(!port->options.typeList);
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
            // logDebug("port value changed: $0", port->getId());
            recordUndo(UndoRecordType::Update, port->getId());
            lastUndoCommand()->portValueData = port->oldValue;

            onNodePortValueChange(port);
        };

        const float dragFloatSpeed = 0.25f;
        auto& options = port->options;
        switch (type) {
        case IntTypeFloat:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat(labelBuf, &port->value.u.f, dragFloatSpeed, 0, 0, "%.3f", flags)) {
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
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
                    callOnValueChange();
                }
            }

            break;
        }
        case IntTypeBool:
        {
            if (checkBox(labelBuf, &port->value.u.b, options.readonly)) {
                callOnValueChange();
            }
            break;
        }
        case IntTypeColor:
        {
            int flags = options.readonly ? ImGuiColorEditFlags_DefaultOptions_ | ImGuiColorEditFlags_NoInputs : 0;
            if (ImGui::ColorEdit3(labelBuf, port->value.u.vector4, flags)) {
                callOnValueChange();
            }
            break;
        }
        case IntTypeVector3:
        {
            // fixme: vector3 and vector4 has a value change bug.
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat3(labelBuf, port->value.u.vector3, dragFloatSpeed, 0, 0, "%.3f", flags)) {
                callOnValueChange();
            }
            break;
        }
        case IntTypeVector4:
        {
            int flags = options.readonly ? ImGuiSliderFlags_ReadOnly : 0;
            if (ImGui::DragFloat4(labelBuf, port->value.u.vector4, dragFloatSpeed, 0, 0, "%.3f", flags)) {
                callOnValueChange();
            }
            break;
        }
        case IntTypeChar:
        {
            int flags = options.readonly ? ImGuiInputTextFlags_ReadOnly : 0;
            ImGui::SetNextItemWidth(SightNodeFixedStyle::charTypeLength);
            if (ImGui::InputText(labelBuf, port->value.u.string, 2, 0)) {
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
                            logDebug("resize", largeString.bufferSize, data->BufSize, data->BufTextLen);
                        }
                    }
                    return 0;
                };
                if (ImGui::InputTextMultiline(labelBuf, port->value.u.largeString.pointer, port->value.u.largeString.bufferSize,
                                              ImVec2(nWidth, 150), flags, callback, &port->value)) {
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) {
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

    void SightNodeTemplateAddress::showContextMenu(bool createComponent) {
        if (children.empty()) {
            // if (createComponent && !this->templateNode->checkAsComponent()) {
            //     return;
            // }

            auto graph = currentGraph();
            auto anyThing = graph->findSightAnyThing(g_ContextStatus->componentForWhich);

            if (templateNode->checkAsComponent()) {
                const auto& c = templateNode->component;

                if (g_ContextStatus->componentForWhich == 0) {
                    // create nodes
                    if (c.onlyComponent) {
                        return;
                    }
                }

                // node, connection
                if (anyThing.type == SightAnyThingType::Node && !c.allowNode) {
                    return;
                }

                if (anyThing.type == SightAnyThingType::Connection && !c.allowConnection) {
                    return;
                }
            } else if (createComponent) {
                return;
            }

            if (ImGui::MenuItem(name.c_str())) {
                //
                if (createComponent) {
                    //
                    if (anyThing.type == SightAnyThingType::Node) {
                        auto n = anyThing.asNode();
                        n->addComponent(this->templateNode);
                    } else if (anyThing.type == SightAnyThingType::Connection) {
                        auto c = anyThing.asConnection();
                        c->addComponent(this->templateNode);
                    } else {
                        logError("$0 is not a node or connection!", g_ContextStatus->componentForWhich);
                    }

                } else {
                    // create node
                    auto node = this->templateNode->instantiate(graph);
                    auto nodeId = node->getNodeId();
                    node->position = convert(g_ContextStatus->nextNodePosition);
                    ed::SetNodePosition(nodeId, g_ContextStatus->nextNodePosition);
                    if (uiAddNode(node) == CODE_OK) {
                        node = nullptr;
                        // check port
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
            }
        } else {
            auto g = currentGraph();
            if (ImGui::BeginMenu(name.c_str())) {
                for (auto& item : children) {
                    if (item.isAnyItemCanShow(createComponent, g->findSightAnyThing(g_ContextStatus->componentForWhich).type)) {
                        item.showContextMenu(createComponent);
                    }
                }

                ImGui::EndMenu();
            }
        }
    }

    int uiAddNode(SightNode* node) {

        currentGraph()->registerNode(node);
        recordUndo(UndoRecordType::Create, node->getNodeId(), convert(node->position));
        return CODE_OK;
    }

    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection> const& connections, ImVec2 startPos, bool selectNode) {
        getRuntimeId(StartOrStop::Start);
        auto graph = currentGraph();
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
                uiAddNode(item);
                if (selectNode) {
                    ed::SelectNode(nodeId, true);
                }
            }
        }
        if (!connections.empty()) {
            for (const auto& item : connections) {
                uiAddConnection(item.left, item.right, item.connectionId, item.priority);
                auto connection = graph->findConnection(item.connectionId);
                if (connection) {
                    // componentContainer is required from graph, so just copy.
                    // however, this is not a good design, should be refactored.
                    connection->componentContainer = item.componentContainer;
                }
            }
        }
        getRuntimeId(StartOrStop::Stop);
        return CODE_OK;
    }

    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection*> const& connections, ImVec2 startPos, bool selectNode) {
        std::vector<SightNodeConnection> c;
        if (!connections.empty()) {
            c.reserve(connections.size());
            for (const auto& item : connections) {
                c.push_back(*item);
            }
        }
        return uiAddMultipleNodes(nodes, c, startPos, selectNode);
    }

    int uiAddConnection(uint left, uint right, uint id, int priority) {
        auto graph = currentGraph();
        auto connectionId = graph->createConnection(left, right, id, priority);
        graph->markDirty();
        if (connectionId > 0) {
            auto connection = graph->findConnection(id);
            onConnect(connection);
            recordUndo(UndoRecordType::Create, connectionId);
        }
        return connectionId;
    }

    int uiDelNode(uint id) {
        logDebug("try delete node: $0", id);
        auto graph = currentGraph();
        auto node = graph->findNode(id);
        if (!node) {
            return CODE_GRAPH_INVALID_ID;
        }

        // delete connection manually
        auto nodeFunc = [](std::vector<SightNodePort>& list) {
            // loop
            for (auto& port : list) {
                // delete connection
                if (!port.isConnect()) {
                    continue;
                }

                // make a copy of port.connections, and loop
                auto connections = port.connections;
                for (auto& connection : connections) {
                    uiDelConnection(connection->connectionId);
                }
                connections.clear();
            }
        };

        nodeFunc(node->inputPorts);
        nodeFunc(node->outputPorts);

        recordUndo(UndoRecordType::Delete, 0, ed::GetNodePosition(id));
        int status = currentGraph()->fakeDeleteNode(node);
        assert(status == CODE_OK);
        lastUndoCommand()->nodeData = node;
        return CODE_OK;
    }

    int uiDelConnection(uint connectionId) {
        logDebug("try delete connection: $0", connectionId);
        auto connection = currentGraph()->findConnection(connectionId);
        OnDisconnect(connection);

        recordUndo(UndoRecordType::Delete, connectionId);
        int status = currentGraph()->fakeDeleteConnection(connection);
        assert(status == CODE_OK);
        lastUndoCommand()->connectionData = connection;
        return CODE_OK;
    }

    void uiChangeGraph(std::string_view path) {
        auto oldGraph = currentGraph();

        auto openGraphFunc = [path]() {
            char tmp[FILENAME_BUF_SIZE]{ 0 };
            auto graph = currentProject()->openGraph(path, currentUIStatus()->isolate, tmp);
            if (graph) {
                //
                if (graph->verifyId() != CODE_OK) {
                    graph->markBroken(true, " Id has error!");
                } else {
                    SimpleEventBus::graphOpened()->dispatch(graph);
                }

                auto code = changeNodeEditorGraph(tmp);
                if (code == CODE_OK) {
                    nodeEditorFrameBegin();     // update node editor context

                    if (strlen(g_ContextStatus->outputJsonFilePathBuf) <= 0) {
                        auto tmpPath = graph->getSaveAsJsonPath();
                        if (!tmpPath.empty()) {
                            strcpy(g_ContextStatus->outputJsonFilePathBuf, tmpPath.c_str());
                        }

                    }
                }
            }
        };
        if (oldGraph == nullptr || !oldGraph->editing) {
            openGraphFunc();
            return;
        }

        std::string stringPath(path);
        openSaveModal("Save File?", "Current Graph do not save, do you want save file?", [stringPath, openGraphFunc, oldGraph](SaveOperationResult r) {
            if (r == SaveOperationResult::Cancel) {
                return;
            } else if (r == SaveOperationResult::Save) {
                oldGraph->save();
            }
            openGraphFunc();
        });
    }

    bool showNodePortType(NodePortType& portType) {
        // combo box
        static const char* strings[] = { "Input", "Output", "Both", "Field" };
        const int offset = static_cast<int>(NodePortType::Input);
        int value = static_cast<int>(portType) - offset;
        assert(value >= 0 && value < std::size(strings));

        bool f = false;
        if (ImGui::BeginCombo("PortType", strings[value])) {
            for (int i = 0; i < std::size(strings); i++) {
                if (ImGui::Selectable(strings[i], i == value)) {
                    value = i;
                    portType = static_cast<NodePortType>(value + offset);
                    f = true;
                }
            }
            ImGui::EndCombo();
        }
        return f;
    }

    void showPortOptions(SightBaseNodePortOptions& options) {
        ToggleButton("Show", &options.show);
        ImGui::SameLine();
        ImGui::Text("Show");
        ImGui::SameLine();
        ToggleButton("ShowValue", &options.showValue);
        ImGui::SameLine();
        ImGui::Text("ShowValue");
        ImGui::SameLine();
        ToggleButton("Readonly", &options.readonly);
        ImGui::SameLine();
        ImGui::Text("Readonly");

        // return false;
    }

    void showGraphSettings() {
        constexpr const auto fmt = "%12s: ";

        auto graph = currentGraph();
        auto& settings = graph->getSettings();

        ImGui::Text(fmt, "Name");
        ImGui::SameLine();
        if (ImGui::InputText("##graphName", settings.graphName, std::size(settings.graphName))) {
            graph->markDirty();
        }

        ImGui::Text("Language Info");
        ImGui::Text(fmt, "Type");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##type", DefLanguageTypeNames[settings.language.type])) {
            for (int i = 0; i < std::size(DefLanguageTypeNames); i++) {
                if (ImGui::Selectable(DefLanguageTypeNames[i], i == settings.language.type)) {
                    //
                    settings.language.type = i;
                    graph->markDirty();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Text(fmt, "Version");
        ImGui::SameLine();
        if (ImGui::InputInt("##version", &settings.language.version)) {
            graph->markDirty();
        }

        ImGui::Text(fmt, "OutputFile");
        ImGui::SameLine();
        if (ImGui::Button("...")) {
            // open
            int status = CODE_OK;
            auto baseDir = currentProject()->pathTargetFolder();

            std::string path = saveFileDialog(baseDir.c_str(), &status, graph->getFileName());
            if (status == CODE_OK) {
                settings.outputFilePath = path;
                logDebug(path);
                graph->markDirty();
            } else {
                if (status == CODE_USER_CANCELED) {
                    logDebug("User cancelled.");
                } else {
                    logError("open file dialog error, code: $0", status);
                }
            }
        }
        helpMarker("The file to save graph-parse-result.\nClick to select file.");
        if (!settings.outputFilePath.empty()) {
            ImGui::TextWrapped("%s", settings.outputFilePath.data());
        }

        // code template
        ImGui::Text(fmt, "CodeTemplate");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##code-template", settings.codeTemplate.c_str())) {
            auto names = getCodeTemplateNames();
            for (const auto& item : names) {
                if (ImGui::Selectable(item.c_str(), item == settings.codeTemplate)) {
                    settings.codeTemplate = item;
                    graph->markDirty();
                }
            }
            ImGui::EndCombo();
        }

        // connection code template
        ImGui::Text(fmt, "Connection");
        ImGui::Text(fmt, "  CodeTemplate");
        ImGui::SameLine();
        if (ImGui::BeginCombo("##connection-code-template", settings.connectionCodeTemplate.c_str())) {
            auto names = getConnectionCodeTemplateNames();
            for (const auto& item : names) {
                if (ImGui::Selectable(item.c_str(), item == settings.connectionCodeTemplate)) {
                    settings.connectionCodeTemplate = item;
                    graph->markDirty();
                }
            }
            ImGui::EndCombo();
        }

        // enterNode
        ImGui::Text(fmt, "EnterNode");
        ImGui::SameLine();
        if (ImGui::InputInt("##enter-node", &settings.enterNode)) {
            graph->markDirty();
        }
        if (settings.enterNode > 0) {
            // show name
            ImGui::Text(fmt, "EnterNodeName");
            ImGui::SameLine();
            auto n = graph->findNode(settings.enterNode);
            if (n) {
                ImGui::TextColored(currentUIStatus()->uiColors->nodeIdText, "%s", n->nodeName.c_str());
            } else {
                ImGui::TextColored(currentUIStatus()->uiColors->errorText, "Bad Node Id");
            }
        }
    }

    int showComponentContextMenu() {
        auto tmpId = g_ContextStatus->componentForWhich;
        auto g = currentGraph();
        SightAnyThingType type = g->findSightAnyThing(tmpId).type;
        if (ImGui::BeginPopup(COMPONENT_CONTEXT_MENU)) {
            for (auto& item : currentNodeStatus()->templateAddressList) {
                if (item.isAnyItemCanShow(true, type)) {
                    item.showContextMenu(true);
                }
            }

            ImGui::EndPopup();
        }
        return CODE_OK;
    }

    void showAddComponentButton(uint id) {
        if (ImGui::Button("Add Component...")) {
            g_ContextStatus->createComponents = true;
            g_ContextStatus->componentForWhich = id;

            // g_ContextStatus->nextNodePosition = ImGui::GetMousePos();
            // g_ContextStatus->contextMenuCreateNodeOpen = true;
            ImGui::OpenPopup(COMPONENT_CONTEXT_MENU);
        }

        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_CONTENT_PASTE)) {
            auto graph = currentGraph();

            // copy clip board
            auto ct = CopyText::parse(ImGui::GetClipboardText());
            if (ct.type != CopyTextType::Component) {
                // not a component
                openAlertModal("Alert!", "Clipboard does not have a component.");
                return;
            }

            SightNode* pointer;
            if (ct.loadAsNode(pointer, graph) == CODE_OK) {
                graph->getComponentContainer(id)->addComponent(pointer);
            } else {
                logError("load node from clip board error");
            }
        }
        
    }

    int showNodeComponents(SightComponentContainer* container, SightNode* node, SightNodeGraph* graph, uint id, bool fromInspector) {
        if (!container) {
            if (fromInspector) {
                showAddComponentButton(id);
            }
            return CODE_OK;
        }

        if (!graph && node) {
            graph = node->graph;
        }

        int delIndex = -1;
        int index = -1;
        char labelBuf[NAME_BUF_SIZE] = {'\0'};
        for (const auto& item : container->components) {
            index++;

            if (fromInspector) {
                ImGui::Separator();
                ImGui::TextColored(currentUIStatus()->uiColors->nodeIdText, "%s", item->nodeName.c_str());
                ImGui::SameLine();
                sprintf(labelBuf, ICON_MD_DELETE "##del-component-%d", index);
                if (ImGui::Button(labelBuf)) {
                    delIndex = index;
                }
                ImGui::SameLine();
                
                sprintf(labelBuf, ICON_MD_CONTENT_COPY "##copy-component-%d", index);
                if (ImGui::Button(labelBuf)) {
                    CopyText::copyComponent(*item);
                    logInfo("copy component successfully!");
                }
                showNodePorts(item);
            } else if (node) {
                ImGui::TextColored(currentUIStatus()->uiColors->nodeIdText, "%s", ICON_MD_CHECK_BOX);
                ImGui::SameLine();
                ImGui::TextColored(currentUIStatus()->uiColors->nodeIdText, "%s", item->nodeName.c_str());

                auto& nodeStyle = node->templateNode->nodeStyle;
                showNodePortsWithEd(item, nodeStyle);
            }
        }

        if (delIndex >= 0) {
            // container->components.erase(container->components.begin() + delIndex);
            container->removeComponent(delIndex);
            graph->markDirty();
        }

        if (fromInspector) {
            showAddComponentButton(id);
        }

        return CODE_OK;
    }

    void uiReloadGraph() {
        auto g = currentGraph();
        if (g) {
            std::string path = g->getFilePath();
            disposeGraph();
            uiChangeGraph(path);
        }
    }

    bool showTypeList(std::string& currentValue) {
        bool flag = false;
        if (ImGui::BeginCombo("##TypeList", currentValue.c_str())) {
            auto p = currentProject();
            for (auto const& item : p->getTypeListCache()) {
                if (ImGui::Selectable(item.c_str(), item == currentValue)) {
                    currentValue = item;
                    flag = true;
                }
            }

            ImGui::EndCombo();
        }

        return flag;
    }

    void showDynamicRootPort(SightNodePort& item, int fmtCharSize) {
        ImGui::Text("%-*s", fmtCharSize, item.portName.c_str());
        ImGui::SameLine();
        if (ImGui::Button(ICON_MD_ADD "##add-dynamic-port")) {
            if (item.ownOptions.showAddChild) {
                item.ownOptions.showAddChild = false;
            } else {
                hideAllAddChild(currentGraph());
                item.ownOptions.showAddChild = true;
                sprintf(currentUIStatus()->buffer.customPortName, "");
            }
        }
        if (item.ownOptions.showAddChild) {
            auto& buffer = currentUIStatus()->buffer;
            ImGui::SetNextItemWidth(ImGui::CalcTextSize("A").x * std::max(SightNodeFixedStyle::minNameInputLen, fmtCharSize));
            ImGui::InputText("##add-dynamic-port-child", buffer.customPortName, std::size(buffer.customPortName));
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_DONE "##add-dynamic-port-done")) {
                if (strlen(buffer.customPortName) > 0) {
                    item.node->addNewPort(buffer.customPortName, item);
                    sprintf(buffer.customPortName, "");
                } else {
                    toast("need a name", "");
                }
            }
        }
    }

    void hideAllAddChild(SightNodeGraph* graph) {
        if (!graph) {
            return;
        }

        auto nodeFunc = [](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                item.ownOptions.showAddChild = false;
            }
        };

        for (auto& node : graph->getNodes()) {
            CALL_NODE_FUNC(&node);
        }
    }

    void uiSaveGraph() {
        auto g = currentGraph();
        if (g->isDirty()) {
            currentProject()->saveConfigFile();
        }

        g->save(g_ContextStatus->lastSaveGraphReason);
        g_ContextStatus->lastSaveGraphReason = SaveReason::Automatic;     // change to auto
    }

    void trySaveCurrentGraph() {
        currentGraph()->markDirty();
        g_ContextStatus->needSaveGraph = true;
        g_ContextStatus->lastSaveGraphReason = SaveReason::User;
    }

    bool showOutputJsonConfigPanel(SightNodeGraph* graph, SightNodeGraphOutputJsonConfig* config) {
        bool result = false;
        if (ImGui::Begin("Output Json Config Panel")) {
            const float interval = 10;

            const char* longString = "includeNodeIdOnConnection";
            const float longStringWidth = ImGui::CalcTextSize(longString).x;
#define ADJUST_TEXT_AND_INPUT_TEXT(itemName, itemValue, itemValueSize)                             \
    const char* itemName##String = #itemName;                                                      \
    const float itemName##StringWidth = ImGui::CalcTextSize(itemName##String).x;                   \
    const float itemName##DifferenceInPixels = longStringWidth - itemName##StringWidth - interval; \
    ImGui::SetCursorPosX(itemName##DifferenceInPixels);                                            \
    ImGui::Text("%s", itemName##String);                                                           \
    ImGui::SameLine(longStringWidth);                                                              \
    ImGui::InputText("##" #itemName, itemValue, itemValueSize);

            if (g_ContextStatus->outputJsonFilePathError) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            }
            ADJUST_TEXT_AND_INPUT_TEXT(FilePath, g_ContextStatus->outputJsonFilePathBuf, std::size(g_ContextStatus->outputJsonFilePathBuf))
            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_FILE_OPEN)) {
                // select a save file location.
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
                    if (path.length() >= std::size(g_ContextStatus->outputJsonFilePathBuf)) {
                        logError("path too long: $0", path);
                    } else {
                        sprintf(g_ContextStatus->outputJsonFilePathBuf, "%s", path.data());
                        logDebug("select file: $0", path.data());
                    }
                }
            }
            if (g_ContextStatus->outputJsonFilePathError) {
                ImGui::PopStyleColor();
            }

            // nodeRootName
            char nodeRootName[32];
            strncpy(nodeRootName, config->nodeRootName.c_str(), sizeof(nodeRootName));
            nodeRootName[sizeof(nodeRootName) - 1] = '\0';
            ADJUST_TEXT_AND_INPUT_TEXT(nodeRootName, nodeRootName, sizeof(nodeRootName))
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                config->nodeRootName = nodeRootName;
            }

            // connectionRootName
            char connectionRootName[32];
            strncpy(connectionRootName, config->connectionRootName.c_str(), sizeof(connectionRootName));
            connectionRootName[sizeof(connectionRootName) - 1] = '\0';
            ADJUST_TEXT_AND_INPUT_TEXT(connectionRootName, connectionRootName, sizeof(connectionRootName))
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                config->connectionRootName = connectionRootName;
            }

            // includeRightConnections
            ImGui::Text("includeRightConnections");
            ImGui::SameLine();
            ImGui::Checkbox("##includeRightConnections", &config->includeRightConnections);
            ImGui::SameLine();
            ImGui::Dummy({ 20, 0 });
            ImGui::SameLine();
            // includeNodeIdOnConnectionData
            ImGui::Text("includeNodeIdOnConnectionData");
            ImGui::SameLine();
            ImGui::Checkbox("##includeNodeIdOnConnectionData", &config->includeNodeIdOnConnectionData);
            ImGui::SameLine();
            ImGui::Dummy({ 20, 0 });
            ImGui::SameLine();
            // exportData
            ImGui::Text("exportData");
            ImGui::SameLine();
            ImGui::Checkbox("##exportData", &config->exportData);
            
            // fieldNameCaseType
            ImGui::Text("fieldNameCaseType");
            ImGui::SameLine();
            const char* caseTypes[] = { "None", "PascalCase", "CamelCase", "SnakeCase" };
            int currentCaseType = static_cast<int>(config->fieldNameCaseType);
            ImGui::SetNextItemWidth(140);
            ImGui::Combo("##fieldNameCaseType", &currentCaseType, caseTypes, IM_ARRAYSIZE(caseTypes));
            config->fieldNameCaseType = static_cast<CaseTypes>(currentCaseType);

            // Buttons
            auto uiStatus = currentUIStatus();
            if (ImGui::Button(uiStatus->languageKeys->commonKeys.ok)) {
                if (strlen(g_ContextStatus->outputJsonFilePathBuf) <= 0) {
                    g_ContextStatus->outputJsonFilePathError = true;
                } else if (!isValidPath(g_ContextStatus->outputJsonFilePathBuf)) {
                    g_ContextStatus->outputJsonFilePathError = true;
                } else {
                    result = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(uiStatus->languageKeys->commonKeys.cancel)) {
                // close window
                currentUIStatus()->windowStatus.graphOutputJsonConfigWindow = false;
            }
        }
        ImGui::End();

        return result;
    }

    bool isNodeEditorReady() {
        return g_ContextStatus->context;
    }
}