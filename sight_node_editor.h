//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#include "imgui.h"
#include "imgui_node_editor.h"

#include "sight_ui.h"
#include "sight_memory.h"

#include "v8.h"

namespace ed = ax::NodeEditor;

namespace sight {

    /**
     *
     */
    enum class NodePortType {
        Input = 100,
        Output,
        // both input and output
        Both,
        // do not have a port, only field.  on this way, it should has a input box.
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
        struct {
            char * pointer;
            size_t size;
        } largeString;     // todo largeString
    };

    struct SightNodePortOptions {
        bool showValue = false;
    };

    /**
     * Port, or you can call it pin.
     */
    struct SightNodePort {
        std::string portName;
        uint id;
        // input/output ...
        NodePortType kind;

        // IntTypeValues ...
        uint type;
        SightNodeValue value;
        SightNodePortOptions options;

        // connections
        std::vector<SightNodeConnection*> connections;
        SightNode* node = nullptr;

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
         * IntTypeValues
         * @return
         */
        uint getType() const;

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
        uint connectionId;
        uint left;
        uint right;


        /**
         * Remove from left and right port.
         */
        void removeRefs(SightNodeGraph *graph = nullptr);

        uint leftPortId() const;
        uint rightPortId() const;
    };

    /**
     * Unstable struct, you shouldn't hold this. Just get it, use it, and drop it.
     */
    struct SightNodePortConnection {
        struct SightNodePort* self = nullptr;
        struct SightNodePort* target = nullptr;
        struct SightNodeConnection* connection = nullptr;

        bool bad() const;

        SightNodePortConnection() = default;
        ~SightNodePortConnection() = default;
        SightNodePortConnection(SightNodeGraph* graph, SightNodeConnection* connection, SightNode* node);
        SightNodePortConnection(SightNodePort *self, SightNodePort *target, SightNodeConnection *connection);
    };

    struct SightNodePortHandle{
        SightNode *node = nullptr;
        uint portId = 0;

        /**
         * You shouldn't hold this function's result.
         * @return
         */
        SightNodePort* get() const;

        SightNodePort* operator->() const;
        SightNodePort& operator*() const;
        
    };

    enum class SightNodeProcessType {
        Enter = 100,
        Exit,

        Normal = 200,
    };

    struct SightJsNodeOptions{
        SightNodeProcessType processFlag = SightNodeProcessType::Normal;


    };

    /**
     * a real node.
     */
    struct SightNode {
        std::string nodeName;
        uint nodeId;
        // this node's template node.  Real template node's `templateNode` must be nullptr.
        SightNode const* templateNode = nullptr;
        SightNodeGraph* graph = nullptr;

        // chainInPort and chainOutPort are point to inputPorts and outputPorts
        // do not need free them. `updateChainPortPointer` will update value.
        SightNodePort* chainInPort = nullptr;
        SightNodePort* chainOutPort = nullptr;

        std::vector<SightNodePort> inputPorts;
        std::vector<SightNodePort> outputPorts;
        // no input/output ports.
        std::vector<SightNodePort> fields;

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

        /**
         * Find the next active connection.
         * If there is one and only one valid process connection, return it, otherwise nullptr.
         * Here is the priority.
         * 1. The `chainOut` port.
         * 2. Any valid connection.
         * @return
         */
        SightNodePortConnection findConnectionByProcess();

        /**
         *
         */
        void updateChainPortPointer();

        /**
         * add chain ports, if not exist.
         */
        void tryAddChainPorts();

    protected:

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
    struct SightJsNode : public SightNode{

        std::string fullTemplateAddress;

        v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> generateCodeWork;
        v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>> onReverseActive;
        SightJsNodeOptions options;

        SightJsNode();
        ~SightJsNode() = default;

        void callFunction(const char *name);

        SightJsNode & operator=(SightNode const & sightNode);

    };

    enum class SightAnyThingType {
        Node,
        Port,
        Connection,

        // used for return value, it means nothing was find.
        Invalid,
    };

    union SightAnyThingData{
        SightNode* node = nullptr;
        SightNodeConnection* connection;
        SightNodePortHandle portHandle;

        SightAnyThingData();
        SightAnyThingData(SightNodeConnection* connection);
        SightAnyThingData(SightNode* node);
        SightAnyThingData(SightNode* node, uint portId);
    };

    struct SightAnyThingWrapper{
        SightAnyThingType type = SightAnyThingType::Node;
        SightAnyThingData data;

        SightAnyThingWrapper() = default;
        SightAnyThingWrapper(SightAnyThingType type, SightNodeConnection* connection);
        SightAnyThingWrapper(SightAnyThingType type, SightNode* node);
        SightAnyThingWrapper(SightAnyThingType type, SightNode* node, uint portId);

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
        //
        SightJsNode templateNode;
        //
        std::vector<SightNodeTemplateAddress> children;

        SightNodeTemplateAddress();
        ~SightNodeTemplateAddress();
        explicit SightNodeTemplateAddress(std::string name);
        SightNodeTemplateAddress(std::string name, const SightJsNode& templateNode);
        SightNodeTemplateAddress(std::string name, const SightNode& templateNode);

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
        bool editing = false;

        SightNodeGraph();
        ~SightNodeGraph();

        /**
         *
         * @param leftPortId
         * @param rightPortId
         * @return If create success, the connection's id.
         * -1: one of left,right is invalid. -2: they are same. -3: same kind.
         */
        uint createConnection(uint leftPortId, uint rightPortId, uint connectionId = 0);

        /**
         *
         * @param connection
         */
        void addConnection(const SightNodeConnection& connection, SightNodePort* left = nullptr, SightNodePort* right = nullptr);

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
        SightNode* findNode(uint id);

        /**
         *
         * @param processType
         * @param status   0 = success, 1 multiple enter nodes. 2 no type nodes.
         * @return
         */
        SightNode* findNode(SightNodeProcessType processType, int* status = nullptr);

        /**
         *
         * @param id
         * @return The element of vector, you should not free the result. If not find, nullptr.
         */
        SightNodeConnection* findConnection(int id);

        SightNodePort* findPort(uint id);

        /**
         *
         * @param id
         * @return You should not keep the return value, just use it's asXXX function.
         */
        const SightAnyThingWrapper& findSightAnyThing(uint id) ;

        /**
         * You should call this function at init step.
         * after call this function, this->filepath will be `path`.
         * @return 0 success. Positive number is warning, and negative number is error.
         * 1: templateNode not found.
         *
         * -1: file not found.
         * -2: bad file.
         */
        int load(const char *path, bool isLoadEntity = false);

        int save();

        /**
         * Save graph data to file.
         * @param path
         * @param set       if true, then set this->filepath to path
         * @param saveAnyway  if true, broken flag will be omitted.
         * @return  0 success, 1 graph is broken.
         */
        int saveToFile(const char *path = nullptr, bool set = false, bool saveAnyway = false);


        void setFilePath(const char* path);
        const char* getFilePath() const;

        SightArray <SightNode> & getNodes();
        const SightArray <SightNode> & getNodes() const;
        const SightArray <SightNodeConnection> & getConnections() const;

        bool isBroken() const;

    private:

        // save and read path.
        std::string filepath;
        bool broken;

        // real nodes
        SightArray<SightNode> nodes;
        // real connections.
        SightArray<SightNodeConnection> connections{BIG_ARRAY_SIZE};
        // key: node/port/connection id, value: the pointer of the instance.
        std::map<uint, SightAnyThingWrapper> idMap;
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
        // file path buf.
        char contextConfigFileBuf[NAME_BUF_SIZE * 2] = {0};
        // node template | for render
        std::vector<SightNodeTemplateAddress> templateAddressList;

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
        SightJsNode* findTemplateNode(const char* path);

    private:

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

    void nodeEditorFrameBegin();

    void nodeEditorFrameEnd();

    int showNodeEditorGraph(UIStatus const& uiStatus);

    /**
     * @brief 
     * 
     * @param port 
     * @param width 
     * @param type     TypeInfo's id
     */
    void showNodePortValue(SightNodePort* port, int width = 160, int type = -1);

    uint nextNodeOrPortId();

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
     * !!DO NOT USE THIS FUNCTION DIRECTLY, USE `Project` CLASS !!
     * @param pathWithoutExt   like './simple'  without dot.
     * @param loadEntityGraph  is pathWithoutExt a entity graph?  may deprecated.
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

    /**
     *
     * @param node
     * @return
     */
    SightJsNode* findTemplateNode(const SightNode *node);
    SightJsNode* findTemplateNode(const char* path);

        /**
     * @brief 
     * 
     * @return true  a graph is opened.
     * @return false 
     */
        bool isNodeEditorReady();
}