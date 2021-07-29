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

    struct SightNodeConnection;

    /**
     * Port, or you can call it pin.
     */
    struct SightNodePort {
        std::string portName;
        int id;
        NodePortType kind;
        // connections
        std::vector<SightNodeConnection*> connections;

        /**
         * Set kind from int.
         * This is for js nodes.
         * @param intKind
         */
        void setKind(int intKind);

        /**
         * Does this port has 1 or more connections ?
         * @return
         */
        bool isConnect() const;

        /**
         *
         * @return
         */
        int connectionsSize() const;
    };

    /**
     * a connection.
     */
    struct SightNodeConnection {
        int connectionId;
        struct SightNodePort* left;
        struct SightNodePort* right;

        // others

        /**
         * Remove from left and right port.
         */
        void removeRefs();
    };

    /**
     * -
     */
    struct SightNodePortConnection {


        struct SightNodePort* self = nullptr;
        struct SightNodePort* target = nullptr;
        struct SightNodeConnection* connection = nullptr;
    };

    /**
     * a real node.
     */
    struct SightNode {
        std::string nodeName;
        int nodeId;
        // this node's template node.
        SightNode* templateNode = nullptr;

        std::vector<SightNodePort> inputPorts;
        std::vector<SightNodePort> outputPorts;

        /**
         *
         * @param port
         */
        void addPort(SightNodePort & port);

        /**
         * instantiate a node by this node.
         * if this node is a template, then instantiate one.
         * else it will instantiate one by this node's template.
         * @return a new node, you should `delete` the object when you do not need it.
         */
        SightNode* instantiate();

        /**
         * clone this node
         * you should `delete` this function's return value.
         * @return
         */
        SightNode* clone() const;

    private:

        void copyFrom(const SightNode *node);
    };

    /**
     * js node
     */
    struct SightJsNode : SightNode{

        void callFunction(const char *name);
    };

    /**
     * todo colors
     */
    struct SightNodeStyles {

    };

    /**
     * A graph contains many nodes.
     */
    struct SightNodeGraph{
        // real nodes
        std::vector<SightNode> nodes;
        // real connections.
        std::vector<SightNodeConnection> connections;

        /**
         *
         * @param node
         */
        void addNode(const SightNode &node);

        /**
         * Dispose graph.
         */
        void dispose();

        /**
         * Save graph data to file.
         * @param path
         * @param set       if true, then set this->filepath to path
         * @return
         */
        int saveToFile(const char *path = nullptr, bool set = false);

        void setFilePath(const char* path);

        const char* getFilePath() const;

    private:

        // save and read path.
        std::string filepath;

    };

    /**
     * sight  node editor status
     */
    struct NodeEditorStatus {

        SightNodeGraph* graph = nullptr;
        ed::EditorContext* context = nullptr;
        // node template


        /**
         * Create a graph.
         * @param path
         * @return
         */
        int createGraph(const char*path);
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

    int showNodeEditorGraph(const UIStatus & uiStatus);

    void initTestData();

    int nextNodeOrPortId();

    /**
     * add and delete node memory.  (use keyword delete)
     * @param node
     * @return
     */
    int addNode(SightNode *node);

}