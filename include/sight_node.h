//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
// Include node editor ui part and logic part.

#pragma once

#include "sight_defines.h"
#include "sight_util.h"
#include "v8-isolate.h"
#include "v8-object.h"
#include "v8pp/convert.hpp"

#include <atomic>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <vector>
#include <map>


#include "yaml-cpp/yaml.h"

#include "sight.h"
#include "sight_colors.h"
#include "sight_memory.h"
#include "sight_js_parser.h"

#include "v8.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

#include "absl/strings/substitute.h"

using Isolate = v8::Isolate;

namespace sight {

    struct Vector2 {
        float x, y;
        Vector2() {
            x = y = 0.0f;
        }
        Vector2(float _x, float _y) {
            x = _x;
            y = _y;
        }

        bool isZero(){
            return x == 0 && y == 0;
        }
    };
    Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs);
    Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs);

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

    enum class SaveReason {
        User,
        Script,
        Automatic,
    };

    enum class SightNodeFlags : uchar {
        None = 0,
        Component = 1 << 0,
        Deleted = 1 << 1,
    };

    struct SightNodeConnection;
    class SightNodeGraph;
    struct SightNode;
    struct SightJsNode;
    struct SightJsNodePort;
    struct SightNodePortHandle;
    struct SightEntity;

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
        uint getType() const;

        /**
         * @brief Set the Value object from a string.
         * You must call setType(type) first.
         * @param str 
         */
        bool setValue(std::string_view str);

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

        const char* getString() const;

        const char* getLargeString() const;

        std::string getLargeStringCopy() const;
 

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

    /**
     * 
     */
    struct SightNodePortOptions {
        bool typeList = false;
        // if this is a dynamic port, showAddChild will show a input text
        bool showAddChild = false;
        // is this port a dynamic port? 
        bool dynamicPort = false;
        char* varName = nullptr;
        short varNameLength = 0;

        std::string customPortName;

        ~SightNodePortOptions();

        void initVarName(); 

        SightNodePortOptions& operator=(SightNodePortOptions const& rhs);

    private:
        void freeVarName();

    };

    struct SightBaseNodePortOptions {
        bool showValue = false;
        // if false, this node port will be hidden.
        bool show = true;
        bool readonly = false;
        std::string errorMsg;
        // render as a combo box 
        std::vector<std::string> alternatives;
        // show as a type list
        bool typeList = false;
        // allow a port list? 
        bool dynamic = false;
    };


    struct SightBaseNodePort {
        std::string portName;
        // input/output ...
        NodePortType kind;
        SightBaseNodePortOptions options;

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
        
        SightNodePortOptions ownOptions;

        // if this is a dynamic port, `parent` is which one current port fork from.
        uint parent = 0;
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

        /**
         * @brief 
         * Remove a connection from connections field. 
         * Be careful, you should try SightNodeGraph::delConnection(int id, bool removeRefs = true) or SightNodeGraph::fakeDeleteConnection(uint id) first.
         * 
         * @param id 
         * @return true 
         * @return false 
         */
        bool removeConnection(uint id);

        /**
         * @brief Add a connection to connections field.
         * 
         * @param id 
         * @return true 
         * @return false 
         */
        bool addConnection(SightNodeConnection* connection);

        SightNode* getNode();
        uint getNodeId() const;

        SightNodeGraph* getGraph();
        const SightNodeGraph* getGraph() const;

        operator SightNodePortHandle() const;
    };

    struct SightComponentContainer : ResetAble {
        std::vector<SightNode*> components;
        SightNodeGraph* graph;

        inline SightComponentContainer* setGraph(SightNodeGraph* graph) {
            this->graph = graph;
            return this;
        }

        /**
         * @brief 
         * 
         * @param node 
         * @return true 
         * @return false 
         */
        bool addComponent(SightJsNode* templateNode);

        /**
         * @brief 
         * 
         * @param p   should be a memory-managed node(e.g SightNodeGraph's node)
         * @param graph 
         * @return true 
         * @return false 
         */
        bool addComponent(SightNode* p);

        bool removeComponent(SightNode* p);
        
        bool removeComponent(size_t index);

        
        void reset() override;
    };

    /**
     * a connection.
     */
    struct SightNodeConnection : ResetAble {
        uint connectionId;
        uint left;
        // using `changeRight`  to change 
        uint right;
        // left port color  
        uint leftColor = IM_COL32_WHITE;

        // priority for generate code.
        // number bigger, priority higher.
        int priority = 10;
        // should this connection generate code ?
        bool generateCode = true;

        uchar flags = (uchar)SightNodeFlags::None;

        SightNodeGraph* graph = nullptr;
        
        SightComponentContainer* componentContainer = nullptr;

        SightNodeConnection();
        SightNodeConnection(uint id, uint left, uint right, uint leftColor, int priority);
        
        /**
         * Remove from left and right port.
         */
        void removeRefs();

        void makeRefs();

        uint leftPortId() const;
        uint rightPortId() const;

        SightNodePort* findLeftPort() const;
        SightNodePort* findRightPort() const;

        SightComponentContainer* getComponentContainer();

        bool addComponent(SightJsNode* templateNode);

        bool addComponent(SightNode* sightNode);

        // reset
        void reset() override;

        void markAsDeleted(bool f = true);

        void markAsComponent(bool f = true);

        bool isDeleted() const;

        bool isComponent() const;

        bool changeRight(uint newRight);
        bool changeLeft(uint newLeft);
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

    struct SightJsNodeOptions{
        uint titleBarPortType = IntTypeProcess;
    };


    /**
     * a real node.
     */
    struct SightNode : public ResetAble {

        enum class CopyFromType {
            Clone,
            Instantiate,

            // current node as a component.
            Component,

            // duplicate this node, this will clear port's connections.
            Duplicate,
        };

        std::string nodeName;
        uint nodeId;
        uchar flags = (uchar)SightNodeFlags::None;
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

        Vector2 position;

        // use `getComponentContainer()` to get a exists object.
        SightComponentContainer* componentContainer = nullptr;

        /**
         * this function do not record by id.
         * @param port
         */
        void addPort(const SightNodePort & port);

        /**
         * this is diff from `void addPort(const SightNodePort & port);`
         * this will be generate new id for port and record it. (Which means user can find this port by it's id.)
         */
        int addNewPort(std::string_view name, NodePortType kind, uint type, const SightJsNodePort* templateNodePort = nullptr, uint parent = 0);

        /**
         * clone from @from
         */
        int addNewPort(std::string_view name, SightNodePort& from);

        /**
         * 
         */
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
         * clone this node,  use `new` to alloc memory.
         * you should `delete` this function's return value.
         * @return
         */
        SightNode* clone(bool generateId = true) const;

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

        /**
         * @brief No dot call this function mutiple times.
         * 
         */
        void callEventOnInstantiate();

        /**
         * @brief Does this node component compatible?
         * 
         * @return true 
         * @return false 
         */
        bool checkAsComponent() const;

        /**
         * @brief 
         * 
         * @param node 
         * @return true 
         * @return false 
         */
        bool addComponent(SightJsNode* templateNode);
        
        bool addComponent(SightNode* sightNode);

        /**
         * find all children ports in this node.
         */
        std::vector<uint> findChildrenPorts(uint parentPortId) const;

        SightComponentContainer* getComponentContainer();

        /**
         * @brief make a snapshot of this object. all fields, inputs, outputs included
         * 
         * @return v8::Local<v8::Object> 
         */
        v8::Local<v8::Object> toV8Object(v8::Isolate* ) const;

        /**
         * 
         * @param node copy all data from this node.
         */
        void copyFrom(const SightNode* node, CopyFromType copyFromType = CopyFromType::Clone, bool generateId = true);

        void markAsDeleted(bool f = true);

        void markAsComponent(bool f = true);

        bool isDeleted() const;

        bool isComponent() const;

        /**
         * @brief Any port that has a connection ? 
         * 
         * @return true 
         * @return false 
         */
        bool hasConnections() const;
        
    protected:
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
        
        template<class... Args>
        std::string getString(Isolate* isolate, Args const&... args) {
            auto result = operator()(isolate, args...);
            if (result.IsEmpty()) {
                return {};
            }

            auto v = result.ToLocalChecked();
            if (IS_V8_STRING(v)) {
                return v8pp::from_v8<std::string>(isolate, v);
            }
            return {};
        }

        v8::MaybeLocal<v8::Value> operator()(Isolate* isolate, SightNode* node) const;
        void operator()(Isolate* isolate, SightNodePort* thisNodePort, JsEventType eventType = JsEventType::Default, void* pointer = nullptr) const;
        v8::MaybeLocal<v8::Value> operator()(Isolate* isolate, SightNode* node, v8::Local<v8::Value> arg1, v8::Local<v8::Value> arg2) const;
        v8::MaybeLocal<v8::Value> operator()(Isolate* isolate, SightEntity* entity) const;
        v8::MaybeLocal<v8::Value> operator()(Isolate* isolate, int index, const char* graphName) const;

        operator bool() const;

        /**
         * @brief node will turn into a v8 object, and modify of the object will NOT affect the node.
         * 
         * @param isolate 
         * @param node 
         * @return v8::MaybeLocal<v8::Value> 
         */
        v8::MaybeLocal<v8::Value> callByReadonly(Isolate* isolate, SightNode* node) const;
        
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
     * 
     * @brief component js options
     * 
     */
    struct SightJsComponent {
        bool active = false;
        bool onlyComponent = true;       // cannot as a independent node ?
        bool allowNode = true;
        bool allowConnection = false;     // allow add this component to connection?
        bool activeOnReverse = false;     // call functions on `onReverseActive` ?
        ScriptFunctionWrapper beforeGenerate;
        ScriptFunctionWrapper afterGenerate;
        ScriptFunctionWrapper appendDataToOutput;
    };

    struct CommonOperation{
        std::string name;
        std::string description;
        ScriptFunctionWrapper function;

        CommonOperation() = default;
        ~CommonOperation() = default;
        CommonOperation(std::string name, std::string description, ScriptFunctionWrapper::Function const& f);
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
        constexpr static int minNameInputLen = 16;
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
        SightJsComponent component;

        SightJsNode();
        ~SightJsNode() = default;

        /**
         *
         * @param port
         */
        void addPort(const SightJsNodePort& port);

        void compact();

        /**
         * TODO  this function should be DELETED!
         * instantiate a node by this node.
         * if this node is a template, then instantiate one.
         * else it will instantiate one by this node's template.
         * @return a new node, you should `delete` the object when you do not need it.
         */
        // SightNode* instantiate(bool generateId = true) const;

        SightNode* instantiate(SightNodeGraph* graph, bool generateId = true) const;
        
        void resetTo(SightJsNode const* node);
        void resetTo(SightJsNode const& node);

        void reset(); 

        void updateStyle();
        bool isStyleInitialized() const;

        void callEventOnInstantiate(SightNode* p) const;

        bool checkAsComponent() const;

    private:
        void instantiate(SightNode* p, bool generateId = true, SightNodeGraph* graph = nullptr) const;
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
        void showContextMenu(bool createComponent);

        /**
         * @brief 
         * 
         * @param createComponent 
         * @return true 
         * @return false 
         */
        bool isAnyItemCanShow(bool filterComponent, SightAnyThingType appendToType) const;

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

    struct SightNodeGraphSettings {
        DefLanguage language;
        int enterNode = 0;

        std::string outputFilePath;
        std::string codeTemplate;
        std::string connectionCodeTemplate;
        char graphName[LITTLE_NAME_BUF_SIZE] = { 0 };
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

        void updateTemplateNodeStyles();

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
     *
     * @param templateAddress  Make sure templateAddress's name's last element is it's real name.
     *                          In this place, name = address.
     * @return
     */
    int addTemplateNode(const SightNodeTemplateAddress& templateAddress, bool isUpdate = false);

    bool delTemplateNode(std::string_view fullName);

    /**
     *
     * @return
     */
    SightNodeGraph* currentGraph();

    NodeEditorStatus* currentNodeStatus();

    /**
     *
     */
    void disposeGraph();

    /**
     * change graph to another one.  
     * !!DO NOT USE THIS FUNCTION DIRECTLY, USE `Project` CLASS !!
     * @param pathWithoutExt   like './simple'  without dot.
     */
    void changeGraph(const char* pathWithoutExt, v8::Isolate* isolate);

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

    YAML::Emitter& operator<< (YAML::Emitter& out, const Vector2& v);
    YAML::Emitter& operator<< (YAML::Emitter& out, const SightNode& node);
    YAML::Emitter& operator<< (YAML::Emitter& out, const SightNodeConnection& connection);
    YAML::Emitter& operator<< (YAML::Emitter& out, const SightComponentContainer& componentContainer);
    bool operator>>(YAML::Node const&node, Vector2 &v);
    bool operator>>(YAML::Node const& node, SightNodeConnection& connection);
    bool operator>>(YAML::Node const& node, SightComponentContainer& componentContainer);

    /**
     * @brief 
     * 
     * @param yamlNode 
     * @param node will be managed by graph.
     * @param graph 
     * @param useOldId 
     * @return int 
     */
    int loadNodeData(YAML::Node const& yamlNode, SightNode*& node, SightNodeGraph* graph, bool useOldId = true);


    /**
     * @brief 
     * 
     * @param nodes 
     * @param connections 
     * @param genConId   generate Connection Id ? 
     * @return int 
     */
    int regenerateId(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections, bool genConId = false);

    inline bool isNodePortShowValue(SightNodePort const& port){
        uint type = port.getType();
        return port.options.showValue && type != IntTypeProcess && type != IntTypeObject;
    }

    /**
     * @brief 把 fromNode 替换成 toNode
     * 
     * @param fromNode 
     * @param toNode 
     */
    bool replaceNode(SightNodeGraph* graph, uint fromNode, uint toNode);



}