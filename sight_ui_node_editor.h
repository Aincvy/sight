//
// Created by Aincvy(aincvy@gmail.com) on 2021/12/19.
//
#pragma once

#include "imgui.h"
#include "sight_ui.h"
#include "sight_nodes.h"
#include <string_view>

#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace sight {

    ImVec2 getNodePos(SightNode* node);
    ImVec2 getNodePos(SightNode const& node);
    void setNodePos(SightNode* node, ImVec2 pos);
    void setNodePos(SightNode& node, ImVec2 pos);

    ImVec2 convert(Vector2 v);
    Vector2 convert(ImVec2 v);

    /**
     * init
     * @return
     */
    int initNodeEditor(bool nodeStatusFlag = true);

    /**
     * destroy and free
     * @param full full destory ?
     * @return
     *
     */
    int destroyNodeEditor(bool full = false, bool nodeStatusFlag = false);

    int changeNodeEditorGraph(std::string_view pathWithoutExt);

    int showNodeEditorGraph(UIStatus const& uiStatus);

    /**
     * @brief 
     * 
     * @param port 
     * @param width 
     * @param type     TypeInfo's id
     * @param fromGraph   true: for drawing graph, false: not
     */
    void showNodePortValue(SightNodePort* port, bool fromGraph = false, int width = 120, int type = -1);

    void nodeEditorFrameBegin();

    void nodeEditorFrameEnd();

    /**
     * @brief 
     * 
     * @return true  a graph is opened.
     * @return false 
     */
    bool isNodeEditorReady();


    /**
     * add and delete node memory.  (use keyword delete)
     * @param node
     * @return CODE
     */
    int uiAddNode(SightNode* node, bool freeMemory = true);

    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection> const& connections, ImVec2 startPos, bool freeMemory = true, bool selectNode = true);
    int uiAddMultipleNodes(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection*> const& connections, ImVec2 startPos, bool freeMemory = true, bool selectNode = true);

    /**
     * @brief 
     * 
     * @param left 
     * @param right 
     * @param id 
     * @param priority 
     * @return int  connection id, or negative number
     */
    int uiAddConnection(uint left, uint right, uint id = 0, int priority = 10);

    /**
     * @brief 
     * This function record an undo-operation.
     * @param id 
     * @return int 
     */
    int uiDelNode(uint id);

    /**
     * @brief 
     * This function record an undo-operation.
     * @param id 
     * @return int 
     */
    int uiDelConnection(uint id);

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    void uiChangeGraph(const char* path);
    
}