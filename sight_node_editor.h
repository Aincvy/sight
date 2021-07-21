//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_node_editor.h"

#include "sight_ui.h"

namespace ed = ax::NodeEditor;

namespace sight {

    // pin
    struct SightNodePort {
        std::string portName;
        ed::PinId id;
        ed::PinKind kind;
    };

    struct SightNode {
        std::string nodeName;
        ed::NodeId nodeId;

        std::vector<SightNodePort> ports;
    };


    /**
     * init
     * @return
     */
    int initNodeEditor();

    /**
     * destroy and free
     * @return
     *
     */
    int destroyNodeEditor();


    int testNodeEditor(const ImGuiIO& io);


    int showNodeEditorGraph(const UIStatus & uiStatus);

    void initTestData();


}