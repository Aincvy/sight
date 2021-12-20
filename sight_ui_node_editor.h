//
// Created by Aincvy(aincvy@gmail.com) on 2021/12/19.
//
#pragma once

#include "sight_ui.h"
#include "sight_nodes.h"
#include <string_view>

#include "imgui_node_editor.h"

namespace ed = ax::NodeEditor;

namespace sight {

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
}