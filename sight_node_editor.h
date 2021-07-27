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

    /**
     *
     */
    enum NodePortType {
        Input = 100,
        Output,
        // both input and output
        Both,
        // do not have a port, only field.  on this way, it should has a textbox.
        Field,
    };

    // pin
    struct SightNodePort {
        std::string portName;
        int id;
        NodePortType kind;
        // for `kind` member, this one has higher priority.
        int intKind;

        /**
         * fix status,  you should call it before use node port.
         */
        void updateStatus();
    };

    struct SightNode {
        std::string nodeName;
        int nodeId;

        std::vector<SightNodePort> inputPorts;
        std::vector<SightNodePort> outputPorts;


        void addPort(SightNodePort & port);
    };

    /**
     * js node
     */
    struct SightJsNode : SightNode{

        void callFunction(const char *name);
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

    int nextNodeOrPortId();

}