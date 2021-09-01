//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once
#include <string>
#include <vector>
#include <map>

#include "imgui.h"
#include "imgui_node_editor.h"

#include "sight_ui.h"
#include "sight_memory.h"

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
    class SightNodeGraph;
    struct SightNode;

    union SightNodeValue{
        int i;
        float f;
        double d;
        bool b;
        char string[NAME_BUF_SIZE] = {0};
        char* largeString;
    };

    /**
     * Port, or you can call it pin.
     */
    struct SightNodePort {
        std::string portName;
        int id;
        // input/output ...
        NodePortType kind;
        // int/long/String/float ...
        // use getType() function, do not use this field directly.
        std::string type;
        SightNodeValue value;

        // connections
        std::vector<SightNodeConnection*> connections;
        SightNode* node = nullptr;
        SightNodePort const* templatePort = nullptr;

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

        /**
         * int/long/String/float ...
         * @return
         */
        std::string const& getType() const;

        /**
         *
         * @return
         */
        const char* getDefaultValue() const;

    };

    /**
     * a connection.
     */
    struct SightNodeConnection {
        int connectionId;
        struct SightNodePort* left;
        struct SightNodePort* right;

        /**
         * Add ref to left and right.
         * You should call this function only once.
         */
        void addRefs();

        /**
         * Remove from left and right port.
         */
        void removeRefs();

        int leftPortId() const;
        int rightPortId() const;

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
        // this node's template node.  Real template node's `templateNode` must be nullptr.
        SightNode const* templateNode = nullptr;
        SightNodeGraph* graph = nullptr;

        std::vector<SightNodePort> inputPorts;
        std::vector<SightNodePort> outputPorts;

        /**
         *
         * @param port
         */
        void addPort(const SightNodePort & port);

        /**
         * instantiate a node by this node.
         * if this node is a template, then instantiate one.
         * else it will instantiate one by this node's template.
         * @return a new node, you should `delete` the object when you do not need it.
         */
        SightNode* instantiate(bool generateId = true) const;

        /**
         * clone this node
         * you should `delete` this function's return value.
         * @return
         */
        SightNode* clone() const;

    private:

        /**
         *
         * @param node
         * @param isTemplate   if true, node will as a template node.
         */
        void copyFrom(const SightNode *node, bool isTemplate);
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

    enum class SightAnyThingType {
        Node,
        Port,
        Connection,

        // used for return value, it means nothing was find.
        Invalid,
    };

    struct SightAnyThingWrapper{
        SightAnyThingType type = SightAnyThingType::Node;
        void* pointer = nullptr;

        SightNode* asNode() const;
        SightNodePort* asPort() const;
        SightNodeConnection* asConnection() const;
    };

    /**
     * Used for context menu.
     */
    struct SightNodeTemplateAddress {
        // name or address
        std::string name;
        // templateNode will call delete when current object deconstructing.
        SightNode templateNode;
        //
        std::vector<SightNodeTemplateAddress> children;

        SightNodeTemplateAddress();
        ~SightNodeTemplateAddress();
        SightNodeTemplateAddress(std::string name, SightNode templateNode);

        /**
         * If has children, then show them, otherwise, show self.
         */
        void showContextMenu(const ImVec2 &openPopupPosition);
    };

    /**
     * A graph contains many nodes.
     */
    class SightNodeGraph{
    public:

        //
        std::atomic<int> nodeOrPortId = 10000;

        SightNodeGraph();
        ~SightNodeGraph();

        /**
         *
         * @param leftPortId
         * @param rightPortId
         * @return If create success, the connection's id.
         * -1: one of left,right is invalid. -2: they are same. -3: same kind.
         */
        int createConnection(int leftPortId, int rightPortId, int connectionId = -1);

        /**
         *
         * @param connection
         */
        void addConnection(const SightNodeConnection& connection);

        /**
         *
         * @param node
         */
        void addNode(const SightNode &node);

        /**
         *
         * @param id
         * @return
         */
        int delNode(int id);

        /**
         *
         * @param id
         * @return
         */
        int delConnection(int id);

        /**
         * Find a node by id
         * @param id
         * @return The element of vector, you should not free the result. If not find, nullptr.
         */
        SightNode* findNode(int id);

        /**
         *
         * @param id
         * @return The element of vector, you should not free the result. If not find, nullptr.
         */
        SightNodeConnection* findConnection(int id);

        SightNodePort* findPort(int id);

        /**
         *
         * @param id
         * @return You should not keep the return value, just use it's asXXX function.
         */
        const SightAnyThingWrapper& findSightAnyThing(int id) ;

        /**
         * You should call this function at init step.
         * after call this function, this->filepath will be `path`.
         * @return
         */
        int load(const char *path, bool isLoadEntity = false);

        int save();

        /**
         * Save graph data to file.
         * @param path
         * @param set       if true, then set this->filepath to path
         * @return
         */
        int saveToFile(const char *path = nullptr, bool set = false);


        void setFilePath(const char* path);
        const char* getFilePath() const;

        SightArray <SightNode> & getNodes();
        const SightArray <SightNode> & getNodes() const;
        const SightArray <SightNodeConnection> & getConnections() const;

    private:

        // save and read path.
        std::string filepath;

        // real nodes
//        std::vector<SightNode> nodes;
        SightArray<SightNode> nodes;
        // real connections.
//        std::vector<SightNodeConnection> connections;
        SightArray<SightNodeConnection> connections{BIG_ARRAY_SIZE};
        // key: node/port/connection id, value: the pointer of the instance.
        std::map<int, SightAnyThingWrapper> idMap;
        // for ...
        const static SightAnyThingWrapper invalidAnyThingWrapper;


        /**
         * Dispose graph.
         */
        void dispose();

        /**
         * clear data
         */
        void reset();
    };


    /**
     * sight  node editor status
     */
    struct NodeEditorStatus {

        SightNodeGraph* graph = nullptr;
        SightNodeGraph* entityGraph = nullptr;
        ed::EditorContext* context = nullptr;
        //
        char contextConfigFile[NAME_BUF_SIZE * 2] = {0};
        // node template
        std::vector<SightNodeTemplateAddress> templateAddressList;
        // key: SightNode pointer, value: full template address.
        std::map<SightNode const*, std::string> templateReverseFindMap;

        NodeEditorStatus();
        ~NodeEditorStatus();

        /**
         * Create a graph.
         * @param path
         * @return
         */
        int createGraph(const char*path);

        /**
         * Try to load graph first, if failed, then create one.
         * Recommend use function `changeGraph`.
         * @param path
         * @return
         */
        int loadOrCreateGraph(const char*path);

        /**
         *
         * @param path
         * @return
         */
        SightNode* findTemplateNode(const char* path);

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

    int nextNodeOrPortId();

    /**
     * add and delete node memory.  (use keyword delete)
     * @param node
     * @return
     */
    int addNode(SightNode *node);

    /**
     *
     * @param templateAddress  Make sure templateAddress's name's last element is it's real name.
     *                          In this place, name = address.
     * @return
     */
    int addTemplateNode(const SightNodeTemplateAddress& templateAddress);

    /**
     *
     * @return
     */
    SightNodeGraph* getCurrentGraph();

    /**
     *
     */
    void disposeGraph(bool context = true);

    /**
     * change graph to another one.
     * @param pathWithoutExt   like './simple'  without dot.
     */
    void changeGraph(const char* pathWithoutExt, bool loadEntityGraph = false);


    /**
     * Generate node from entity.
     * @param createEntityData
     * @return
     */
    SightNode* generateNode(const UICreateEntity & createEntityData, SightNode *node = nullptr);

    /**
     *
     * @param createEntityData
     * @return 0 ok.
     */
    int addEntity(const UICreateEntity & createEntityData);

    /**
     * Load all entities.
     * @return
     */
    int loadEntities();

}