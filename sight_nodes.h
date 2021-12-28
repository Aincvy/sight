//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>
#include <map>

#include "imgui.h"
#include "imgui_node_editor.h"

#include "sight_defines.h"
#include "sight_project.h"
#include "sight_ui.h"
#include "sight_memory.h"

#include "v8.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

using Isolate = v8::Isolate;

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
    struct SightJsNode;
    struct SightJsNodePort;
    struct SightNodePortHandle;

    struct SightNodeValue{
        union {
            int i;
            float f;
            double d;
            bool b;
            char string[NAME_BUF_SIZE] = { 0 };
            float vector2[2];
            float vector3[3];
            float vector4[4];

            struct {
                char* pointer;
                size_t size;
                size_t bufferSize;
            } largeString;     // largeString
        } u;
        
        void setType(uint type);

        // if this union is largeString, then you can call below functions.
        // if not, DO NOT call those functions.

        /**
         * @brief 
         * 
         * @param needSize 
         * @return true  has modified buffer 
         * @return false has not has modified buffer
         */
        bool stringCheck(size_t needSize);
        void stringCopy(std::string const& str);

        SightNodeValue(uint type);
        SightNodeValue() = default;
        /**
         * @brief Construct a new Sight Node Value object
         * Use `copy` function.
         * @param rhs 
         */
        SightNodeValue(SightNodeValue const& rhs);
        /**
         * @brief 
         * Use `copy` function.
         * @param rhs 
         * @return SightNodeValue& 
         */
        SightNodeValue& operator=(SightNodeValue const& rhs);

        ~SightNodeValue();

    private:
        uint type;

        void stringInit();
        void stringFree();
    };

    struct SightNodePortOptions {
        bool showValue = false;
        // if false, this node port will be hidden.
        bool show = true;
        std::string errorMsg;
        bool readonly = false;
        // render as a combo box 
        std::vector<std::string> alternatives;
    };


    struct SightBaseNodePort {
        std::string portName;
        // input/output ...
        NodePortType kind;
        SightNodePortOptions options;

        uint type;

        SightNodeValue value;

        SightBaseNodePort() = default;
        SightBaseNodePort(uint type);

        /**
         * Set kind from int.
         * This is for js nodes.
         * @param intKind
         */
        void setKind(int intKind);

        virtual uint getType() const;

        const char* getPortName() const;
    };

    /**
     * Port, or you can call it pin.
     */
    struct SightNodePort : public SightBaseNodePort {
        uint id;

        // IntTypeValues ... use getType(), this maybe a fake type. 
        // uint type;
        
        // last value.
        SightNodeValue oldValue;
        
        // connections
        std::vector<SightNodeConnection*> connections;
        SightNode* node = nullptr;
        const SightJsNodePort* templateNodePort = nullptr;

        SightNodePort() = default;
        ~SightNodePort() = default;
        SightNodePort(NodePortType kind, uint type, const SightJsNodePort* templateNodePort = nullptr);

        /**
         * Does this port has 1 or more connections ?
         * @return
         */
        bool isConnect() const;

        bool isTitleBarPort() const;

        /**
         *
         * @return
         */
        int connectionsSize() const;

        /**
         * IntTypeValues
         * @return
         */
        uint getType() const override;

        /**
         *
         * @return
         */
        const char* getDefaultValue() const;

        uint getId() const;

        /**
         * @brief 
         * 
         * @return int count, -1 if graph not found.
         */
        int clearLinks();

        /**
         * @brief sort connectino by connection's priority
         * 
         */
        void sortConnections();

        SightNodeGraph* getGraph();
        const SightNodeGraph* getGraph() const;

        operator SightNodePortHandle() const;
    };

    /**
     * a connection.
     */
    struct SightNodeConnection {
        uint connectionId;
        uint left;
        uint right;
        // left port color  
        uint leftColor = IM_COL32_WHITE;

        // priority for generate code.
        // number bigger, priority higher.
        int priority = 10;

        /**
         * Remove from left and right port.
         */
        void removeRefs(SightNodeGraph *graph = nullptr);

        uint leftPortId() const;
        uint rightPortId() const;

        SightNodePort* findLeftPort() const;
        SightNodePort* findRightPort() const;
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

        operator bool() const;
        
    };

    enum class SightNodeProcessType {
        Enter = 100,
        Exit,

        Normal = 200,
    };

    struct SightJsNodeOptions{
        SightNodeProcessType processFlag = SightNodeProcessType::Normal;
        uint titleBarPortType = IntTypeProcess;
    };

    struct SightBaseNode {

    };

    /**
     * a real node.
     */
    struct SightNode : public ResetAble, SightBaseNode {
        std::string nodeName;
        uint nodeId;
        // this node's template node.  Real template node's `templateNode` must be nullptr.
        const SightJsNode* templateNode = nullptr;
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

        SightNodePort* getOppositeTitleBarPort(NodePortType type) const;

        /**
         * @brief Get the Opposite Port By Type object
         * 
         * @param kind 
         * @param type 
         * @return SightNodePort* 
         */
        SightNodePort* getOppositePortByType(NodePortType kind, uint type);

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
         * @brief be similar with findConnectionByProcess()
         * 
         * @return SightNodePort* 
         */
        SightNodePort* findPortByProcess();

        /**
         *
         */
        void updateChainPortPointer();

        /**
         * add chain ports, if not exist.
         */
        void tryAddChainPorts(uint type = IntTypeProcess);

        /**
         * @brief sort all port's connections.
         * 
         */
        void sortConnections();

        /**
         * @brief Reset node for next time used.
         * This function will be release some memory.
         */
        void reset() override;

        /**
         * @brief 
         * 
         * @param name 
         * @param order 
         * @return SightNodePortHandle 
         */
        SightNodePortHandle findPort(const char* name, int orderSize = 0, int order[] = nullptr);

        uint getNodeId() const;

        const char* templateAddress() const;

    protected:
        enum class CopyFromType {
            Clone,
            Instantiate,
        };

        /**
         *
         * @param node
         * @param isTemplate   if true, node will as a template node.
         */
        void copyFrom(const SightNode* node, CopyFromType copyFromType = CopyFromType::Clone);
    };

    enum class JsEventType {
        Default,
        AutoComplete,
        // onConnect or onDisconnect
        Connect,
        ValueChange,
    };

    struct ScriptFunctionWrapper {
        using Function = v8::Persistent<v8::Function, v8::CopyablePersistentTraits<v8::Function>>;

        Function function;
        std::string sourceCode;

        ScriptFunctionWrapper() = default;
        ~ScriptFunctionWrapper() = default;
        ScriptFunctionWrapper(Function const& f);
        ScriptFunctionWrapper(std::string const& code);

        void compile(Isolate* isolate);

        void operator()(Isolate* isolate, SightNode* node) const;
        void operator()(Isolate* isolate, SightNodePort* thisNodePort, JsEventType eventType = JsEventType::Default, void* pointer = nullptr) const;
        v8::MaybeLocal<v8::Value> operator()(Isolate* isolate, SightNode* node, v8::Local<v8::Value> arg1, v8::Local<v8::Value> arg2) const;

        operator bool() const;

        private:

        /**
         * @brief 
         * 
         * @return true  function is ready 
         * @return false function not ready
         */
        bool checkFunction(Isolate* isolate) const;
    };

    /**
     * @brief 
     * 
     */
    struct SightJsNodePort : public SightBaseNodePort {
        

        // events, called by ui thread.
        ScriptFunctionWrapper onValueChange;

        ScriptFunctionWrapper onAutoComplete;
        ScriptFunctionWrapper onConnect;
        ScriptFunctionWrapper onDisconnect;
        ScriptFunctionWrapper onClick;

        SightJsNodePort() = default;
        ~SightJsNodePort() = default;
        SightJsNodePort(std::string const& name, NodePortType kind);

        SightNodePort instantiate() const;
    };

    struct SightNodeFixedStyle {
        constexpr static int iconSize = 20;
        constexpr static int iconTitlePadding = 3;
        constexpr static int nameCharsPadding = 1;
        constexpr static int fullAlpha = 255;

        constexpr static int commonTypeLength = 120;
        constexpr static int numberTypeLength = 100;
        constexpr static int vector3TypeLength = 200;
        constexpr static int charTypeLength = 20;

    };

    struct SightNodeStyle{
        struct PortTypeStyle{
            int maxCharSize = 0; 
            int inputWidth = 0;

            float totalWidth = 0;
        };

        // this node's max width
        float width = 0;

        PortTypeStyle fieldStype;
        PortTypeStyle inputStype;
        PortTypeStyle outputStype;

        bool largeStringEffect = false;
        bool initialized = false;
    };

    /**
     * js node  / template node
     */
    struct SightJsNode {

        std::string nodeName;
        std::string fullTemplateAddress;

        std::vector<SightJsNodePort*> inputPorts;
        std::vector<SightJsNodePort*> outputPorts;
        // no input/output ports.
        std::vector<SightJsNodePort*> fields;

        std::vector<SightJsNodePort> originalInputPorts;
        std::vector<SightJsNodePort> originalOutputPorts;
        // no input/output ports.
        std::vector<SightJsNodePort> originalFields;

        absl::flat_hash_set<std::string> bothPortList;

        SightJsNodeOptions options;

        // called by js thread
        ScriptFunctionWrapper::Function generateCodeWork;
        ScriptFunctionWrapper::Function onReverseActive;

        // events ,  called by ui thread.
        ScriptFunctionWrapper onInstantiate;
        ScriptFunctionWrapper onDestroyed;
        ScriptFunctionWrapper onReload;
        ScriptFunctionWrapper onMsg;

        SightNodeStyle nodeStyle;

        SightJsNode();
        ~SightJsNode() = default;

        /**
         *
         * @param port
         */
        void addPort(const SightJsNodePort& port);

        void compact();

        void compileFunctions(Isolate* isolate);

        /**
         * instantiate a node by this node.
         * if this node is a template, then instantiate one.
         * else it will instantiate one by this node's template.
         * @return a new node, you should `delete` the object when you do not need it.
         */
        SightNode* instantiate(bool generateId = true, bool callEvent = true) const;
        
        void resetTo(SightJsNode const* node);
        void resetTo(SightJsNode const& node);

        void reset(); 

        void updateStyle();
        bool isStyleInitialized() const;

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
        SightJsNode* templateNode = nullptr;
        //
        std::vector<SightNodeTemplateAddress> children;

        SightNodeTemplateAddress();
        ~SightNodeTemplateAddress();
        explicit SightNodeTemplateAddress(std::string name);
        SightNodeTemplateAddress(std::string name, SightJsNode* templateNode);

        /**
         * If has children, then show them, otherwise, show self.
         */
        void showContextMenu();

        /**
         * @brief Free external memory.
         * The destructor do not call this function.
         * Please call this function only if you know what you are doing.
         */
        void dispose();

        void setNodeMemoryFromArray();

        private:
            bool nodeMemoryFromArray = false;
    };

    struct SightNodeGraphExternalData{
        using V8ObjectType = v8::Persistent<v8::Object, v8::CopyablePersistentTraits<v8::Object>>;

        // std::map<std::string, V8ObjectType> tinyDataMap;
        absl::flat_hash_map<std::string, V8ObjectType> tinyDataMap;
        
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
         * -1: one of left,right is invalid. -2: they are same. -3: same kind. -4 left only can accept 1 connections
         */
        int createConnection(uint leftPortId, uint rightPortId, uint connectionId = 0, int priority = 10);

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
        int delConnection(int id, bool removeRefs = true);

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
        SightNodePortHandle findPortHandle(uint id);

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
        int load(const char *path);

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

        SightNodeGraphExternalData& getExternalData();

        void callNodeEventsAfterLoad();

        /**
         * @brief call all node's sortConnections() function.
         * 
         */
        void sortConnections();

    private:

        // save and read path.
        std::string filepath;
        bool broken = false;

        // real nodes
        SightArray<SightNode> nodes;
        // real connections.
        SightArray<SightNodeConnection> connections{BIG_ARRAY_SIZE};
        // key: node/port/connection id, value: the pointer of the instance.
        std::map<uint, SightAnyThingWrapper> idMap;
        // for ...
        const static SightAnyThingWrapper invalidAnyThingWrapper;

        SightNodeGraphExternalData externalData;


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

        // node template | for render
        std::vector<SightNodeTemplateAddress> templateAddressList;

        SightArray<SightJsNode> templateNodeArray{ LITTLE_ARRAY_SIZE * 2 };
        SightArray<SightJsNodePort> templateNodePortArray{ MEDIUM_ARRAY_SIZE };

        NodeEditorStatus();
        ~NodeEditorStatus();

        /**
         * Create a graph.
         * @param path
         * @return
         */
        int createGraph(const char* path);

        /**
         * Try to load graph first, if failed, then create one.
         * Recommend use function `changeGraph`.
         * @param path
         * @return CODE_OK, CODE_FAIL
         */
        int loadOrCreateGraph(const char* path);

        /**
         *
         * @param path
         * @return
         */
        SightJsNode* findTemplateNode(const char* path);

    private:
    };

    /**
     * @brief 
     * 
     * @return int  CODE_FAIL if already init.
     */
    int initNodeStatus();

    int destoryNodeStatus();

    void onNodePortValueChange(SightNodePort* port);


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

    bool delTemplateNode(std::string_view fullName);

    /**
     *
     * @return
     */
    SightNodeGraph* getCurrentGraph();

    NodeEditorStatus* getCurrentNodeStatus();

    /**
     *
     */
    void disposeGraph();

    /**
     * change graph to another one.  
     * !!DO NOT USE THIS FUNCTION DIRECTLY, USE `Project` CLASS !!
     * @param pathWithoutExt   like './simple'  without dot.
     */
    void changeGraph(const char* pathWithoutExt);

    /**
     *
     * @param node
     * @return
     */
    const SightJsNode* findTemplateNode(const SightNode *node);
    SightJsNode* findTemplateNode(const char* path);

    /**
     * @brief 
     * 
     * @return true  a graph is opened.
     * @return false 
     */
    bool isNodeEditorReady();

    const char* getNodePortTypeName(NodePortType kind);

    inline bool isNodePortShowValue(SightNodePort const& port){
        uint type = port.getType();
        return port.options.showValue && type != IntTypeProcess && type != IntTypeObject;
    }

}