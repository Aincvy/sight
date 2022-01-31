//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <sys/termios.h>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <atomic>
#include <algorithm>
#include <fstream>

#include "sight_nodes.h"
#include "sight_defines.h"
#include "sight.h"
#include "sight_defines.h"
#include "dbg.h"
#include "sight_js.h"
#include "sight_address.h"
#include "sight_util.h"
#include "sight_project.h"
#include "sight_ui.h"

#include "v8pp/call_v8.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/class.hpp"


// node editor status
static sight::NodeEditorStatus* g_NodeEditorStatus;

#define CURRENT_GRAPH g_NodeEditorStatus->graph

namespace sight {

    // define SightNodeGraph::invalidAnyThingWrapper
    const SightAnyThingWrapper SightNodeGraph::invalidAnyThingWrapper = {
            SightAnyThingType::Invalid,
            static_cast<SightNode*>(nullptr),
    };

    namespace {
        // private members and functions

    }

    void onNodePortValueChange(SightNodePort* port) {
        auto node = port->node;
        node->graph->editing = true;
        if (!port->templateNodePort) {
            return;
        }

        auto isolate = currentUIStatus()->isolate;
        auto templateNodePort = port->templateNodePort;

        templateNodePort->onValueChange(isolate, port, JsEventType::ValueChange, (void*) & port->oldValue);
        // if (port->getType() == IntTypeLargeString) {
        //     port->oldValue.stringFree();
        // }
        port->oldValue = port->value;
    }

    uint nextNodeOrPortId() {
        return currentProject()->nextNodeOrPortId();
    }

    SightNodeGraph *currentGraph() {
        return CURRENT_GRAPH;
    }

    NodeEditorStatus* getCurrentNodeStatus() {
        return g_NodeEditorStatus;
    }

    void disposeGraph() {
        if (CURRENT_GRAPH) {
            delete CURRENT_GRAPH;
            CURRENT_GRAPH = nullptr;
        }
    }

    void changeGraph(const char *pathWithoutExt) {
        dbg(pathWithoutExt);
        disposeGraph();

        char buf[NAME_BUF_SIZE];
        sprintf(buf, "%s.yaml", pathWithoutExt);
        g_NodeEditorStatus->loadOrCreateGraph(buf);

        if (CURRENT_GRAPH) {
            CURRENT_GRAPH->callNodeEventsAfterLoad();
        }
        dbg("change over!");
        
    }

    SightNodePort::SightNodePort(NodePortType kind, uint type, const SightJsNodePort* templateNodePort)
        : templateNodePort(templateNodePort), oldValue(type){
        this->kind = kind;
        this->type = type;
        // this->value = {type};
    }

    bool SightNodePort::isConnect() const {
        return !connections.empty();
    }

    bool SightNodePort::isTitleBarPort() const {
        return portName.empty();
    }

    int SightNodePort::connectionsSize() const {
        return static_cast<int>(connections.size());
    }

    uint SightNodePort::getId() const {
        return id;
    }

    int SightNodePort::clearLinks() {
        auto g = getGraph();
        if (!g) {
            return -1;
        }
        if (connections.empty()) {
            return 0;
        }

        std::vector<int> ids;
        std::transform(connections.begin(), connections.end(), std::back_inserter(ids), [](SightNodeConnection* c) {
            return c->connectionId;
        });
        dbg(ids);
        uint count = 0;
        for (const auto& item : ids) {
            if (g->delConnection(item) == CODE_OK) {
                count++;
            }
        }

        return count;
    }

    void SightNodePort::sortConnections() {
        if (this->connections.size() <= 1) {
            return;
        }

        std::sort(connections.begin(), connections.end(), [](SightNodeConnection* c1, SightNodeConnection* c2) {
            if (c1->priority < c2->priority) {
                return false;
            }
            return true;
        });

    }

    const char *SightNodePort::getDefaultValue() const {
        return this->value.u.string;
    }

    uint SightNodePort::getType() const {
        if (templateNodePort) {
            return templateNodePort->type;
        }
        return this->type;
    }


    void SightNode::addPort(const SightNodePort &port) {
        if (port.kind == NodePortType::Input) {
            this->inputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->outputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            this->inputPorts.push_back(port);

            // need change id
            this->outputPorts.push_back(port);
            this->outputPorts.back().id = nextNodeOrPortId();
        } else if (port.kind == NodePortType::Field) {
            this->fields.push_back(port);
        }

    }

    SightNodePort* SightNode::getOppositeTitleBarPort(NodePortType type) const {
        if (type == NodePortType::Input) {
            return chainOutPort;
        } else if (type == NodePortType::Output) {
            return chainInPort;
        }
        return nullptr;
    }

    SightNodePort* SightNode::getOppositePortByType(NodePortType kind, uint type) {
        auto searchFunc = [type](std::vector<SightNodePort> & list) -> SightNodePort* {
            SightNodePort* p = nullptr;
            for( auto& item: list){
                // todo type check 
                if (item.getType() == type) {
                    p = &item;
                    break;
                }
            }
            return p;
        };

        if (kind == NodePortType::Input) {
            return searchFunc(this->outputPorts);
        } else if (kind == NodePortType::Output) {
            return searchFunc(this->inputPorts);
        }
        return nullptr;
    }

    SightNode *SightNode::clone(bool generateId) const{
        auto p = new SightNode();
        p->copyFrom(this, CopyFromType::Clone);
        
        if (generateId) {
            auto nodeFunc = [](std::vector<SightNodePort> & list){
                for(auto& item: list){
                    item.id = nextNodeOrPortId();
                }
            };

            p->nodeId = nextNodeOrPortId();
            CALL_NODE_FUNC(p);
        }

        return p;
    }

    void SightNode::copyFrom(const SightNode* node, CopyFromType copyFromType) {
        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        this->position = node->position;
        this->templateNode = node->templateNode;

        auto copyFunc = [copyFromType, this](std::vector<SightNodePort> const& src, std::vector<SightNodePort>& dst) {
            for (const auto &item : src) {
                dst.push_back(item);
                dst.back().node = this;

                if (copyFromType == CopyFromType::Instantiate) {
                    // init something
                    // auto& back = dst.back();
                    // if (back.getType() == IntTypeLargeString) {
                    //     back.value.stringInit();
                    // }
                }
            }
        };

        copyFunc(node->inputPorts, this->inputPorts);
        copyFunc(node->outputPorts, this->outputPorts);
        copyFunc(node->fields, this->fields);

        // updateChainPortPointer();
    }

    SightNodePortConnection SightNode::findConnectionByProcess() {
        SightNodeConnection* connection = nullptr;

        for (const auto &outputPort : outputPorts) {
            if (!outputPort.isConnect()) {
                continue;
            }

            if (outputPort.getType() == IntTypeProcess) {
                //
                connection = outputPort.connections.front();
                break;
            } else {
                if (connection) {
                    // has one.
                    connection = nullptr;
                    break;
                } else {
                    connection = outputPort.connections.front();
                }
            }
        }

        if (connection) {
            return SightNodePortConnection(graph, connection, this);
        }
        return {};
    }

    SightNodePort* SightNode::findPortByProcess() {
        if (chainOutPort->type == IntTypeProcess && chainOutPort->isConnect()) {
            return chainOutPort;
        }

        SightNodePort* p = nullptr;
        for(auto& item: this->outputPorts){
            if (item.isTitleBarPort() || !item.isConnect()) {
                continue;
            }

            if (p) {
                p = nullptr;
                break;
            } else {
                p = &item;
            }
        }

        return p;
    }

    void SightNode::updateChainPortPointer() {
        for (auto &item : inputPorts) {
            if (item.portName.empty()) {
                // let empty name port to chain port.
                chainInPort = &item;
                break;
            }
        }

        for (auto &item : outputPorts) {
            if (item.portName.empty()) {
                // let empty name port to chain port.
                chainOutPort = &item;
                break;
            }
        }

    }

    void SightNode::tryAddChainPorts(uint type) {
        auto nodeFunc = [type](std::vector<SightNodePort> & list, NodePortType kind){
            bool find = false;
            for (const auto &item : list) {
                if (item.portName.empty()) {
                    find = true;
                    break;
                }
            }

            if (!find) {
                list.push_back({
                    kind,
                    type,
                });
            }
        };

        nodeFunc(this->inputPorts, NodePortType::Input);
        nodeFunc(this->outputPorts, NodePortType::Output);

        updateChainPortPointer();
    }

    void SightNode::sortConnections() {
        for(auto& item: this->inputPorts){
            item.sortConnections();
        }

        for(auto& item: this->outputPorts){
            item.sortConnections();
        }
    }

    void SightNode::reset() {
        dbg("node reset");
        // call events
        if (templateNode) {
            auto t = dynamic_cast<const SightJsNode*>(templateNode);
            if (t) {
                t->onDestroyed(currentUIStatus()->isolate, this);
            }
        }

    }

    SightNodePortHandle SightNode::findPort(const char* name, int orderSize, int order[]) {
        static constexpr int defaultOrder[] = { 
            static_cast<int>(NodePortType::Field), 
            static_cast<int>(NodePortType::Input), 
            static_cast<int>(NodePortType::Output) 
        };

        if (orderSize <= 0) {
            // use normal methods
            orderSize = 3;
            order = (int*)defaultOrder;
        }

        SightNodePortHandle portHandle = {};
        auto func = [&portHandle, name, this](std::vector<SightNodePort> const& list) {
            for (const auto& item : list) {
                if (item.portName == name) {
                    portHandle.node = this;
                    portHandle.portId = item.id;
                    return true;
                }
            }

            return false;
        };

        for( int i = 0; i < orderSize; i++){
            switch (static_cast<NodePortType>(order[i])) {
            case NodePortType::Input:
                if (func(this->inputPorts)) {
                    goto result;
                }
                break;
            case NodePortType::Output:
                if (func(this->outputPorts)) {
                    goto result;
                }
                break;
            case NodePortType::Field:
                if (func(this->fields)) {
                    goto result;
                }
                break;
            default:
                break;
            }
        }

        result:
        return portHandle;
    }

    uint SightNode::getNodeId() const {
        return this->nodeId;
    }

    const char* SightNode::templateAddress() const {
        if (templateNode) {
            return templateNode->fullTemplateAddress.c_str();
        }
        return nullptr;
    }

    void SightNode::callEventOnInstantiate() {
        templateNode->callEventOnInstantiate(this);
    }

    SightJsNode::SightJsNode() {

    }

    void SightJsNode::addPort(const SightJsNodePort& port) {
        if (port.kind == NodePortType::Input) {
            this->originalInputPorts.push_back(port);
        } else if (port.kind == NodePortType::Output) {
            this->originalOutputPorts.push_back(port);
        } else if (port.kind == NodePortType::Both) {
            auto copy = port;
            copy.kind = NodePortType::Input;
            this->originalInputPorts.push_back(copy);
            copy.kind = NodePortType::Output;
            this->originalOutputPorts.push_back(copy);
            this->bothPortList.insert(port.portName);
        } else if (port.kind == NodePortType::Field) {
            this->originalFields.push_back(port);
        }
    }


    void SightJsNode::compact() {
        auto & array = g_NodeEditorStatus->templateNodePortArray;

        for( const auto& item: this->originalFields){
            auto p = array.add(item);
            this->fields.push_back(p);
        }
        this->originalFields.clear();

        for (const auto& item : this->originalInputPorts) {
            auto p = array.add(item);
            this->inputPorts.push_back(p);
        }
        this->originalInputPorts.clear();

        for (const auto& item : this->originalOutputPorts) {
            auto p = array.add(item);
            this->outputPorts.push_back(p);
        }
        this->originalOutputPorts.clear();
    }

    void SightJsNode::compileFunctions(Isolate* isolate) {

    }

    SightNode* SightJsNode::instantiate(bool generateId) const {
        dbg(this->nodeName);
        auto p = new SightNode();
        
        auto portCopyFunc = [](std::vector<SightJsNodePort*> const& src, std::vector<SightNodePort> & dst){
            for( const auto& item: src){
                auto copy = item->instantiate();
                copy.templateNodePort = item;
                dst.push_back(copy);
                dst.back().oldValue = dst.back().value;
            }
        };
        portCopyFunc(this->inputPorts, p->inputPorts);
        portCopyFunc(this->outputPorts, p->outputPorts);
        portCopyFunc(this->fields, p->fields);
        p->nodeName = this->nodeName;
        p->templateNode = this;
        p->tryAddChainPorts(this->options.titleBarPortType);
        p->graph = CURRENT_GRAPH;

        if (generateId) {
            // generate id
            auto nodeFunc = [](std::vector<SightNodePort>& list) {
                for (auto& item : list) {
                    item.id = nextNodeOrPortId();

                    // check type
                    auto t = item.getType();
                    if (!isBuiltInType(t)) {
                        bool isFind = false;
                        auto typeInfo = currentProject()->getTypeInfo(t, &isFind);
                        if (isFind) {
                            typeInfo.initValue(item.value);
                        }
                    }
                }
            };

            p->nodeId = nextNodeOrPortId();
            CALL_NODE_FUNC(p);
        }

        return p;
    }

    void SightJsNode::resetTo(SightJsNode const* node) {
        resetTo(*node);
    }

    void SightJsNode::resetTo(SightJsNode const& node) {
        this->nodeName = node.nodeName;
        this->fullTemplateAddress = node.fullTemplateAddress;

        // 
        auto copyFunc = [](std::vector<SightJsNodePort> &dst, std::vector<SightJsNodePort> const& src) {
            dst.clear();
            dst.assign(src.begin(), src.end());
        };
        copyFunc(originalFields, node.originalFields);
        copyFunc(originalInputPorts, node.originalInputPorts);
        copyFunc(originalOutputPorts, node.originalOutputPorts);

        //
        auto copyPointerFunc = [](std::vector<SightJsNodePort*>& dst, std::vector<SightJsNodePort*> const& src) {
            int i = 0;
            auto& templateNodePortArray = g_NodeEditorStatus->templateNodePortArray;
            for( ; i < src.size(); i++){
                if (i >= dst.size()) {
                    // add
                    auto t = templateNodePortArray.add(*src[i]);
                    dst.push_back(t);
                } else {
                    // copy
                    *dst[i] = *src[i];
                }
            }
        };
        copyPointerFunc(this->fields, node.fields);
        copyPointerFunc(this->inputPorts, node.inputPorts);
        copyPointerFunc(this->outputPorts, node.outputPorts);

        this->options = node.options;
        this->generateCodeWork = node.generateCodeWork;
        this->onReverseActive = node.onReverseActive;
        this->onInstantiate = node.onInstantiate;
        this->onDestroyed = node.onDestroyed;
        this->onReload = node.onReload;
        this->onMsg = node.onMsg;
    }

    void SightJsNode::reset() {
        this->nodeName.clear();
        this->fullTemplateAddress.clear();
        
        // todo
    }

    void SightJsNode::updateStyle() {
        auto& style = this->nodeStyle;
        // if (this->nodeName == "VarDeclare") {
        //     dbg(1);
        // }

        auto nodeFunc = [](std::vector<SightJsNodePort*> const& p, SightNodeStyle::PortTypeStyle& typeStyle, bool isField){
            typeStyle.inputWidth = 0;

            for( const auto& itemPointer: p){
                const auto& item = *itemPointer;

                auto charSize = item.portName.size();
                if (charSize > typeStyle.maxCharSize) {
                    typeStyle.maxCharSize = charSize;
                }

                if (isField || item.options.showValue) {
                    int tmpWidth = 0;
                    switch (item.type) {
                    case IntTypeVector3:
                    case IntTypeVector4:
                    case IntTypeColor:
                        tmpWidth = SightNodeFixedStyle::vector3TypeLength;
                        break;
                    case IntTypeInt:
                    case IntTypeFloat:
                    case IntTypeLong:
                    case IntTypeDouble:
                        tmpWidth = SightNodeFixedStyle::numberTypeLength;
                        break;
                    case IntTypeProcess:
                    case IntTypeObject:
                        tmpWidth = 0;
                        break;
                    default:
                        tmpWidth = SightNodeFixedStyle::commonTypeLength;
                        break;
                    }
                    if (tmpWidth > typeStyle.inputWidth) {
                        typeStyle.inputWidth = tmpWidth;
                    }
                }
            }

            if (typeStyle.maxCharSize > 0) {
                std::string tmp(typeStyle.maxCharSize + SightNodeFixedStyle::nameCharsPadding, 'A');
                auto tmpSize = ImGui::CalcTextSize(tmp.c_str());
                auto imguiStyle = ImGui::GetStyle();

                // typeStyle.totalWidth = typeStyle.inputWidth + imguiStyle.ItemSpacing.x + tmpSize.x + imguiStyle.ItemInnerSpacing.x * 2;
                typeStyle.totalWidth = typeStyle.inputWidth + tmpSize.x;
                if (!isField) {
                    typeStyle.totalWidth += SightNodeFixedStyle::iconSize + imguiStyle.ItemSpacing.x;
                } else {

                }
            } else {
                typeStyle.totalWidth = typeStyle.inputWidth;
            }
        };

        
        nodeFunc(this->fields, style.fieldStype, true);
        nodeFunc(this->inputPorts, style.inputStype, false);
        nodeFunc(this->outputPorts, style.outputStype, false);

        auto inputOutputTotalWidth = style.inputStype.totalWidth + style.outputStype.totalWidth;
        style.width = std::max(inputOutputTotalWidth, style.fieldStype.totalWidth);
        if (style.width <= 0) {
            // do not have any fields or ports
            style.width = ImGui::CalcTextSize(this->nodeName.c_str()).x;
            style.width += (SightNodeFixedStyle::iconSize + SightNodeFixedStyle::iconTitlePadding) * 2 + SightNodeFixedStyle::iconTitlePadding * 2;
        }
        style.initialized = true;
    }

    bool SightJsNode::isStyleInitialized() const {
        return nodeStyle.initialized;
    }

    void SightJsNode::callEventOnInstantiate(SightNode* p) const {
        onInstantiate(currentUIStatus()->isolate, p);
    }

    void SightNodeConnection::removeRefs(SightNodeGraph *graph) {
        if (!graph) {
            graph = CURRENT_GRAPH;
        }
        auto leftPort = graph->findPort(leftPortId());
        auto rightPort = graph->findPort(rightPortId());

        if (leftPort) {
            auto & c = leftPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
        if (rightPort) {
            auto & c = rightPort->connections;
            c.erase(std::remove(c.begin(), c.end(), this), c.end());
        }
    }

    uint SightNodeConnection::leftPortId() const {
        return this->left;
    }

    uint SightNodeConnection::rightPortId() const {
        return this->right;
    }

    SightNodePort* SightNodeConnection::findLeftPort() const {
        return left > 0 ? CURRENT_GRAPH->findPort(left) : nullptr;
    }

    SightNodePort* SightNodeConnection::findRightPort() const {
        return right > 0 ? CURRENT_GRAPH->findPort(right) : nullptr;
    }

    int SightNodeGraph::saveToFile(const char *path, bool set,bool saveAnyway) {
        if (set) {
            this->setFilePath(path);
        }
        if (!saveAnyway && isBroken()) {
            return CODE_FAIL;
        }

        // ctor yaml object
        YAML::Emitter out;
        out << YAML::BeginMap;
        // file header
        out << YAML::Key << whoAmI << YAML::Value << "sight-graph";
        // nodes
        out << YAML::Key << "nodes";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &node : nodes) {
            out << YAML::Key << node.nodeId;
            out << YAML::Value << node;
            // out << node;
        }
        out << YAML::EndMap;

        // connections
        out << YAML::Key << "connections";
        out << YAML::Value << YAML::BeginMap;
        for (const auto &connection : connections) {
            out << YAML::Key << connection.connectionId << YAML::Value << connection;
        }
        out << YAML::EndMap;

        // other info

        out << YAML::EndMap;       // end of 1st begin map

        // save file
        std::ofstream outToFile(path, std::ios::out | std::ios::trunc);
        outToFile << out.c_str() << std::endl;
        outToFile.close();
        dbg("save over");
        return CODE_OK;
    }

    int SightNodeGraph::load(const char *path) {
        this->filepath = path;

        std::ifstream fin(path);
        if (!fin.is_open()) {
            return -1;
        }

        int status = CODE_OK;
        try {
            auto root = YAML::Load(fin);

            // nodes
            auto nodeRoot = root["nodes"];
            dbg(nodeRoot.size());

            int loadNodeStatus = CODE_OK;
            SightNode* sightNode = nullptr;
            for (const auto &nodeKeyPair : nodeRoot) {
                auto yamlNode = nodeKeyPair.second;
                loadNodeStatus = loadNodeData(yamlNode, sightNode);
                if (loadNodeStatus != CODE_OK) {
                    if (status == CODE_OK) {
                        status = loadNodeStatus;
                    }

                    broken = true;
                } 

                if (sightNode) {
                    sightNode->graph = this;
                    addNode(*sightNode);
                    delete sightNode;
                    sightNode = nullptr;
                }
            }

            // connections
            auto connectionRoot = root["connections"];
            SightNodeConnection tmpConnection;
            for (const auto &item : connectionRoot) {
                auto connectionId = item.first.as<int>();
                item.second >> tmpConnection;
                createConnection(tmpConnection.left, tmpConnection.right, connectionId, tmpConnection.priority);
            }

            this->editing = false;
            dbg("load ok");
            return status;
        }catch (const YAML::BadConversion & e){
            fprintf(stderr, "read file %s error!\n", path);
            fprintf(stderr, "%s\n", e.what());
            this->reset();
            return CODE_FILE_ERROR;    // bad file
        } 

        // return 0;
    }

    void SightNodeGraph::dispose() {

    }

    const char *SightNodeGraph::getFilePath() const {
        return this->filepath.c_str();
    }

    void SightNodeGraph::setFilePath(const char* path) {
        this->filepath = path;
    }

    void SightNodeGraph::addNode(const SightNode &node) {
        // check id repeat? 

        auto p = this->nodes.add(node);
        p->updateChainPortPointer();
        idMap[node.nodeId] = {
                SightAnyThingType::Node,
                p
        };

        auto nodeFunc = [p, this](std::vector<SightNodePort> & list){
            for (auto &item : list) {
                item.node = p;
                idMap[item.id] = {
                        SightAnyThingType::Port,
                        p,
                        item.id
                };
            }
        };
        CALL_NODE_FUNC(p);
        p->callEventOnInstantiate();

        this->editing = true;
    }

    SightNode *SightNodeGraph::findNode(uint id) {
        return findSightAnyThing(id).asNode();
    }

    SightNode* SightNodeGraph::findNode(SightNodeProcessType processType, int* status /* = nullptr */){
        int count = 0;

        SightNode* p = nullptr;
        for (auto &n : nodes){
            if (findTemplateNode(&n)->options.processFlag == processType){
                count++;
                if (!p) {
                    p = &n;
                }
            }
        }

        if (count == 0) {
            SET_INT_VALUE(status, 2);
            return nullptr;
        } else if (count == 1) {
            SET_INT_VALUE(status, 0);
            return p;
        } else {
            SET_INT_VALUE(status, 1);
            return p;
        }

//        return nullptr;
    }

    SightNodeConnection *SightNodeGraph::findConnection(int id) {
        return findSightAnyThing(id).asConnection();
    }

    int SightNodeGraph::createConnection(uint leftPortId, uint rightPortId, uint connectionId, int priority) {
        if (leftPortId == rightPortId) {
            return -2;
        }

        auto left = findPort(leftPortId);
        auto right = findPort(rightPortId);
        if (!left || !right) {
            return -1;
        }
        if (left->kind == right->kind) {
            return -3;
        }

        // if (left->getType() == IntTypeProcess && left->isConnect()) {
        //     return -4;
        // }

        auto id = connectionId > 0 ? connectionId : nextNodeOrPortId();
        auto thing = findSightAnyThing(id);
        if (thing.type != SightAnyThingType::Invalid) {
            return -5;
        }

        // find color ;
        ImU32 color = IM_COL32_WHITE;
        auto [typeInfo, find] = currentProject()->findTypeInfo(left->getType());
        if (find && typeInfo.style) {
            color = typeInfo.style->color;
        }        

        SightNodeConnection connection = {
            id,
            leftPortId,
            rightPortId,
            color,
            priority,
        };
        addConnection(connection, left, right);
        return id;
    }

    void SightNodeGraph::addConnection(const SightNodeConnection &connection, SightNodePort* left, SightNodePort* right) {
        auto p = this->connections.add(connection);
        idMap[connection.connectionId] = {
                SightAnyThingType::Connection,
                p
        };

        if (!left) {
            left = findPort(connection.leftPortId());
        }
        if (!right) {
            right = findPort(connection.rightPortId());
        }
        assert(left);
        assert(right);

        left->connections.push_back(p);
        left->sortConnections();

        right->connections.push_back(p);
        right->sortConnections();
    }

    const SightArray <SightNode> & SightNodeGraph::getNodes() const {
        return this->nodes;
    }

    SightNodeGraphExternalData& SightNodeGraph::getExternalData() {
        return externalData;
    }

    void SightNodeGraph::callNodeEventsAfterLoad() {
        // call event one by one.
        auto isolate = currentUIStatus()->isolate;
        auto nodeFunc = [isolate](std::vector<SightNodePort> & list){
            for(auto& item: list){
                if (item.templateNodePort) {
                    auto templateNodePort = item.templateNodePort;
                    templateNodePort->onValueChange(isolate, &item);
                }
            }
        };

        for(auto& item: this->nodes){
            auto p = &item;
            if (item.templateNode) {
                item.templateNode->onReload(isolate, p);
            }

            // port events
            // CALL_NODE_FUNC(p);
        }
    }

    void SightNodeGraph::sortConnections() {
        for(auto& item: this->nodes){
            item.sortConnections();
        }
    }

    int SightNodeGraph::verifyId(bool errorStop, std::function<void(SightNode*)> onNodeError, std::function<void(SightNodePort*)> onPortError,
                                 std::function<void(SightNodeConnection*)> onConnectionError) {
        using set = absl::flat_hash_set<uint>;
        set nodeIdSet;
        set portIdSet;
        set connectionIdSet;

        int status = CODE_OK;
        const int maxNodeOrPortId = currentProject()->maxNodeOrPortId();

        auto isIdInvalid = [&nodeIdSet, &portIdSet, &connectionIdSet, maxNodeOrPortId](uint id) {
            return nodeIdSet.contains(id) || portIdSet.contains(id) || connectionIdSet.contains(id) || id > maxNodeOrPortId;
        };

        auto nodeFunc = [&onPortError, &isIdInvalid, &portIdSet, &status](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                auto id = item.id;
                if (isIdInvalid(id)) {
                    status = CODE_FAIL;
                    if (onPortError) {
                        onPortError(&item);
                    }
                }

                portIdSet.emplace(id);
            }
        };
        
        for(auto& item: nodes){
            auto id = item.nodeId;
            if (isIdInvalid(id)) {
                status = CODE_FAIL;
                // call 
                if (onNodeError) {
                    onNodeError(&item);
                }
                if (errorStop) {
                    break;
                }
            }
            nodeIdSet.emplace(id);

            // check port
            CALL_NODE_FUNC(&item);
            if (status != CODE_OK && errorStop) {
                // return status;
                break;
            }
        }

        if (status == CODE_OK || !errorStop) {
            // check connections
            for (auto& item : connections) {
                auto id = item.connectionId;
                if (isIdInvalid(id)) {
                    status = CODE_FAIL;
                    // call
                    if (onConnectionError) {
                        onConnectionError(&item);
                    }
                    if (errorStop) {
                        break;
                    }
                }

                connectionIdSet.emplace(id);
            }
        }

        return status;
    }

    void SightNodeGraph::checkAndFixIdError() {
        bool rebuildIdMap = false;
        auto nodeFunc = [&rebuildIdMap, this](SightNode* node) {
            auto n = findNode(node->nodeId);
            if (n == node) {
                rebuildIdMap = true;
            }
            node->nodeId = nextNodeOrPortId();
        };
        auto portFunc = [&rebuildIdMap, this](SightNodePort* port) {
            auto p = findPort(port->id);
            if (p == port) {
                rebuildIdMap = true;
            }

            port->id = nextNodeOrPortId();
        };
        auto connectionFunc = [&rebuildIdMap, this](SightNodeConnection* connection) {
            auto c = findConnection(connection->connectionId);
            if (c == connection) {
                rebuildIdMap = true;
            }

            connection->connectionId = nextNodeOrPortId();
        };

        verifyId(false, nodeFunc, portFunc, connectionFunc);

        if (rebuildIdMap) {
            dbg("Need rebuild id map, but not impl!");
        }
    }

    void SightNodeGraph::markDirty() {
        editing = true;
    }

    bool SightNodeGraph::isDirty() const {
        return editing;
    }

    const SightArray<SightNodeConnection>& SightNodeGraph::getConnections() const {
        return this->connections;
    }

    SightNodePort* SightNodeGraph::findPort(uint id) {
        return findSightAnyThing(id).asPort();
    }

    SightNodePortHandle SightNodeGraph::findPortHandle(uint id) {
        auto t = findSightAnyThing(id);
        if (t.data.portHandle) {
            return t.data.portHandle;
        }
        return {};
    }

    const SightAnyThingWrapper& SightNodeGraph::findSightAnyThing(uint id) {
        std::map<uint, SightAnyThingWrapper>::iterator iter;
        if ((iter = idMap.find(id)) == idMap.end()) {
            return invalidAnyThingWrapper;
        }
        return iter->second;
    }

    int SightNodeGraph::delNode(int id, SightNode* copyTo) {
        auto result = std::find_if(nodes.begin(), nodes.end(), [id](const SightNode& n){ return n.nodeId == id; });
        if (result == nodes.end()) {
            return CODE_FAIL;
        }

        // check connections
        auto hasConnections = [](std::vector<SightNodePort>& list){
            for(auto& item: list){
                if (item.isConnect()) {
                    return true;
                }
            }
            return false;
        };
        if (hasConnections(result->fields) || hasConnections(result->inputPorts) || hasConnections(result->outputPorts)) {
            return CODE_NODE_HAS_CONNECTIONS;
        }

        idMap.erase(id);
        if (copyTo) {
            *copyTo = *result;
        }
        nodes.erase(result);
        this->editing = true;
        return CODE_OK;
    }

    int SightNodeGraph::delConnection(int id, bool removeRefs,SightNodeConnection* copyTo) {
        auto result = std::find_if(connections.begin(), connections.end(),
                                   [id](const SightNodeConnection &c) { return c.connectionId == id; });
        if (result == connections.end()) {
            return CODE_FAIL;
        }

        // deal refs.
        if (removeRefs) {
            result->removeRefs();
        }
        idMap.erase(id);

        if (copyTo) {
            *copyTo = *result;
        }
        connections.erase(result);
        dbg(id);
        return CODE_OK;
    }

    int SightNodeGraph::save() {
        if (!editing) {
            return CODE_OK;
        }

        auto c = saveToFile(this->filepath.c_str(), false);
        if (c == CODE_OK) {
            this->editing = false;
        }
        return c;
    }

    SightNodeGraph::SightNodeGraph() {

    }

    SightNodeGraph::~SightNodeGraph() {
        this->dispose();
    }

    void SightNodeGraph::reset() {
        this->nodes.clear();
        this->connections.clear();
        this->idMap.clear();
    }

    SightArray<SightNode> &SightNodeGraph::getNodes() {
        return this->nodes;
    }

    bool SightNodeGraph::isBroken() const {
        return this->broken;
    }

    SightNode *SightAnyThingWrapper::asNode() const {
        if (type != SightAnyThingType::Node) {
            return nullptr;
        }
        return data.node;
    }

    SightNodeConnection* SightAnyThingWrapper::asConnection() const{
        if (type != SightAnyThingType::Connection) {
            return nullptr;
        }
        return data.connection;
    }

    SightNodePort *SightAnyThingWrapper::asPort() const{
        if (type != SightAnyThingType::Port) {
            return nullptr;
        }
        return data.portHandle.get();
    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNodeConnection* connection)
        :type(type), data(connection)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node)
        :type(type), data(node)
    {

    }

    SightAnyThingWrapper::SightAnyThingWrapper(SightAnyThingType type, SightNode *node, uint portId)
        :type(type), data(node, portId)
    {

    }

    void SightNodeTemplateAddress::dispose() {
        if (templateNode) {
            if (nodeMemoryFromArray) {
                g_NodeEditorStatus->templateNodeArray.remove(templateNode);
            } else {
                delete templateNode;
            }
            
            templateNode = nullptr;
        }
    }

    void SightNodeTemplateAddress::setNodeMemoryFromArray() {
        this->nodeMemoryFromArray = true;
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress() {

    }


    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name, SightJsNode* templateNode)
        : name(std::move(name))
        , templateNode(templateNode) {
    }

    SightNodeTemplateAddress::~SightNodeTemplateAddress() {
//        if (templateNode) {
//            delete templateNode;
//            templateNode = nullptr;
//        }
    }

    SightNodeTemplateAddress::SightNodeTemplateAddress(std::string name) : name(std::move(name)) {

    }

    SightNodePort* SightNodePortHandle::get() const {
        if (!node) {
            return nullptr;
        }

        for (auto &item : node->inputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        for (auto &item : node->outputPorts) {
            if (item.id == portId) {
                return &item;
            }
        }

        for(auto& item: node->fields){
            if (item.id == portId) {
                return &item;
            }
        }

        return nullptr;
    }

    SightNodePort *SightNodePortHandle::operator->() const {
        return get();
    }

    SightNodePort &SightNodePortHandle::operator*() const {
        return *get();
    }

    SightNodePortHandle::operator bool() const {
        return this->node && this->portId > 0;
    }

    int addTemplateNode(const SightNodeTemplateAddress &templateAddress) {
        dbg(templateAddress.name);
        if (templateAddress.name.empty()) {
            return -1;
        }

        SightJsNode* templateNode = templateAddress.templateNode;
        templateNode->fullTemplateAddress = templateAddress.name;
        auto address = resolveAddress(templateAddress.name);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        while (pointer) {
            bool findElement = false;
            if (!pointer->next) {
                // no next, this is the last element, it should be the node's name.
                // check name exist
                auto findResult = std::find_if(list->begin(), list->end(), [pointer](SightNodeTemplateAddress const& address) {
                    return address.name == pointer->part;
                });
                // templateNode->updateStyle();
                if (findResult == list->end()) {
                    // not found
                    templateNode->compact();
                    auto tmpPointer = g_NodeEditorStatus->templateNodeArray.add(*templateNode);
                    tmpPointer->compileFunctions(currentUIStatus()->isolate);
                    auto tmpAddress = SightNodeTemplateAddress(pointer->part, tmpPointer);
                    tmpAddress.setNodeMemoryFromArray();
                    list->push_back(tmpAddress);
                } else {
                    // find
                    std::string tmpMsg = "replace template node: ";
                    tmpMsg += pointer->part;
                    dbg(tmpMsg);
                    auto willCopy = *templateNode;

                    // src1: pointers, src2:
                    auto copyFunc = [](std::vector<SightJsNodePort*>& dst, std::vector<SightJsNodePort*> const& src1, std::vector<SightJsNodePort> const& src2) {
                        if (!dst.empty()) {
                            dbg("Oops! dst not empty, clear!");
                            dst.clear();
                        }
                        // copy pointers
                        dst.assign(src1.begin(), src1.end());

                        // copy data
                        auto& templateNodePortArray = g_NodeEditorStatus->templateNodePortArray;
                        for (int i = 0; i < src2.size(); i++) {
                            if (i >= dst.size()) {
                                // add
                                auto t = templateNodePortArray.add(src2[i]);
                                dst.push_back(t);
                            } else {
                                // copy
                                *dst[i] = src2[i];
                            }
                        }
                    };

                    // copy and compact
                    // fields, inputPorts, outputPorts
                    copyFunc(willCopy.fields, findResult->templateNode->fields, willCopy.originalFields);
                    copyFunc(willCopy.inputPorts, findResult->templateNode->inputPorts, willCopy.originalInputPorts);
                    copyFunc(willCopy.outputPorts, findResult->templateNode->outputPorts, willCopy.originalOutputPorts);

                    *findResult->templateNode = willCopy;
                }
                break;
            }

            for(auto iter = list->begin(); iter != list->end(); iter++){
                if (iter->name == pointer->part) {
                    findElement = true;
                    list = & iter->children;
                    break;
                }
            }

            if (!findElement) {
                list->push_back(SightNodeTemplateAddress(pointer->part));
                list = & list->back().children;
            }

            pointer = pointer->next;
        }

        freeAddress(address);
        return CODE_OK;
    }

    bool delTemplateNode(std::string_view fullName) {
        auto address = resolveAddress(std::string{ fullName });
        auto pointer = address;
        auto* list = &g_NodeEditorStatus->templateAddressList;
        bool delFlag = false;

        while (pointer) {
            if (!pointer->next) {
                // this is the name
                auto findResult = std::find_if(list->begin(), list->end(), [pointer](SightNodeTemplateAddress const& address) {
                    return address.name == pointer->part;
                });

                if (findResult != list->end()) {
                    list->erase(findResult);
                    delFlag = true;
                }
            } else {
                bool findElement = false;
                for (auto iter = list->begin(); iter != list->end(); iter++) {
                    if (iter->name == pointer->part) {
                        findElement = true;
                        list = &iter->children;
                        break;
                    }
                }

                if (!findElement) {
                    list->push_back(SightNodeTemplateAddress(pointer->part));
                    list = &list->back().children;
                }
            }

            pointer = pointer->next;
        }
        freeAddress(address);
        return delFlag;
    }

    const SightJsNode* findTemplateNode(const SightNode *node) {
        return node->templateNode;
    }

    SightJsNode* findTemplateNode(const char* path) {
        return g_NodeEditorStatus->findTemplateNode(path);
    }

    const char* getNodePortTypeName(NodePortType kind) {
        switch (kind) {
        case NodePortType::Input:
            return "Input";
        case NodePortType::Output:
            return "Output";
        case NodePortType::Both:
            return "Both";
        case NodePortType::Field:
            return "Field";
        }
        return nullptr;
    }

    YAML::Emitter& operator<< (YAML::Emitter& out, const Vector2& v) {
        out << YAML::Flow << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
        return out;
    }

    YAML::Emitter& operator<< (YAML::Emitter& out, const SightNode& node) {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << node.nodeName;
        out << YAML::Key << "id" << YAML::Value << node.nodeId;
        out << YAML::Key << "position" << YAML::Value << node.position;

        if (node.templateNode) {
            auto address = findTemplateNode(&node);
            if (address) {
                out << YAML::Key << "template" << YAML::Value << address->fullTemplateAddress;
            }
        }

        auto writeFloatArray = [&out](float* p, int size) {
            out << YAML::BeginSeq;
            for (int i = 0; i < size; i++) {
                out << p[i];
            }
            out << YAML::EndSeq;
        };

        auto portWork = [&out, &writeFloatArray](SightNodePort const& item) {
            out << YAML::Key << item.id;
            out << YAML::Value << YAML::BeginMap;
            out << YAML::Key << "name" << YAML::Value << item.portName;
            out << YAML::Key << "type" << YAML::Value << item.type;

            auto type = item.getType();
            out << YAML::Key << "value" << YAML::Value;
            switch (type) {
            case IntTypeFloat:
                out << item.value.u.f;
                break;
            case IntTypeDouble:
                out << item.value.u.d;
                break;
            case IntTypeInt:
            case IntTypeLong:
                out << item.value.u.i;
                break;
            case IntTypeString:
                out << item.value.u.string;
                break;
            case IntTypeLargeString:
                out << item.value.u.largeString.pointer;
                break;
            case IntTypeBool:
                out << item.value.u.b;
                break;
            case IntTypeVector3:
                writeFloatArray((float*)item.value.u.vector3, 3);
                break;
            case IntTypeChar:
                out << item.value.u.string[0];
                break;
            case IntTypeVector4:
            case IntTypeColor:
                writeFloatArray((float*)item.value.u.vector4, 4);
                break;
            default:
            {
                bool serializeGood = false;
                if (!isBuiltInType(type)) {
                    //
                    auto [typeInfo, find] = currentProject()->findTypeInfo(type);
                    if (find) {
                        switch (typeInfo.render.kind) {
                        case TypeInfoRenderKind::ComboBox:
                            out << item.value.u.i;
                            serializeGood = true;
                            break;
                        case TypeInfoRenderKind::Default:
                            break;
                        }
                    }
                }
                if (!serializeGood) {
                    out << YAML::Null;
                }
                break;
            }
            }

            out << YAML::EndMap;
        };

        // input and output
        out << YAML::Key << "inputs";
        out << YAML::Value << YAML::BeginMap;
        for (const auto& item : node.inputPorts) {
            portWork(item);
        }
        out << YAML::EndMap;

        out << YAML::Key << "outputs";
        out << YAML::Value << YAML::BeginMap;
        for (const auto& item : node.outputPorts) {
            portWork(item);
        }
        out << YAML::EndMap;

        out << YAML::Key << "fields";
        out << YAML::Value << YAML::BeginMap;
        for (const auto& item : node.fields) {
            portWork(item);
        }
        out << YAML::EndMap;   // end of fields

        //
        out << YAML::EndMap;   // end of node data.

        return out;
    }

    YAML::Emitter& operator<< (YAML::Emitter& out, const SightNodeConnection& connection) {
        out << YAML::BeginMap;
        // out << YAML::Key << "id" << YAML::Value << connection.connectionId;
        out << YAML::Key << "left" << YAML::Value << connection.leftPortId();
        out << YAML::Key << "right" << YAML::Value << connection.rightPortId();
        out << YAML::Key << "priority" << YAML::Value << connection.priority;
        out << YAML::EndMap;
        return out;
    }

    bool readFloatArray(float* p, int size, YAML::Node const& valueNode) {
        if (valueNode.IsSequence()) {
            for (int i = 0; i < size; i++) {
                auto tmp = valueNode[i];
                if (tmp.IsDefined()) {
                    p[i] = tmp.as<float>();
                }
            }
            return true;
        }

        return false;
    }

    bool operator>>(YAML::Node const& node, Vector2& v) {
        return readFloatArray((float*)&v, 2, node);
    }

    bool operator>>(YAML::Node const&node, SightNodeConnection &connection) {
        // auto nodeId = node["id"];
        // if (nodeId.IsDefined()) {
        //     connection.connectionId = nodeId.as<int>(0);
        // }
        connection.left = node["left"].as<int>(0);
        connection.right = node["right"].as<int>(0);
        auto priorityNode = node["priority"];
        if (priorityNode.IsDefined()) {
            connection.priority = priorityNode.as<int>(0);
        }
        return true;
    }

    int loadPortInfo(YAML::detail::iterator_value const& item, NodePortType nodePortType, std::vector<SightNodePort>& list, bool useOldId) {
        auto id = item.first.as<uint>(0);
        auto values = item.second;
        auto portName = values["name"].as<std::string>();
        auto typeNode = values["type"];
        uint type = 0;
        if (typeNode.IsDefined()) {
            type = typeNode.as<uint>();
        }

        SightNodePort* pointer = nullptr;
        for (auto& item : list) {
            if (item.portName == portName) {
                pointer = &item;
                break;
            }
        }
        if (!pointer) {
            dbg(portName, "not found");
            return CODE_FAIL;
        }

        auto& port = *pointer;
        port.portName = portName;
        if (useOldId) {
            port.id = id;
        }
        port.kind = nodePortType;
        port.type = type;

        auto valueNode = values["value"];
        type = port.getType();     // update to real type.
        if (valueNode.IsDefined()) {
            switch (type) {
            case IntTypeFloat:
                port.value.u.f = valueNode.as<float>();
                break;
            case IntTypeDouble:
                port.value.u.d = valueNode.as<double>();
                break;
            case IntTypeInt:
            case IntTypeLong:
                port.value.u.i = valueNode.as<int>();
                break;
            case IntTypeString:
                sprintf(port.value.u.string, "%s", valueNode.as<std::string>().c_str());
                break;
            case IntTypeLargeString:
            {
                std::string tmpString = valueNode.as<std::string>();
                port.value.stringCheck(tmpString.size());
                sprintf(port.value.u.largeString.pointer, "%s", tmpString.c_str());
                break;
            }
            case IntTypeBool:
                port.value.u.b = valueNode.as<bool>();
                break;
            case IntTypeProcess:
            case IntTypeButton:
            case IntTypeObject:
                // dbg("IntTypeProcess" , portName);
                break;
            case IntTypeChar:
                if (valueNode.IsDefined() && !valueNode.IsNull()) {
                    port.value.u.string[0] = valueNode.as<char>();
                }
                break;
            case IntTypeVector3:
                readFloatArray(port.value.u.vector3, 3, valueNode);
                break;
            case IntTypeVector4:
            case IntTypeColor:
                readFloatArray(port.value.u.vector4, 4, valueNode);
                break;
            default:
            {
                if (!isBuiltInType(type)) {
                    //
                    auto [typeInfo, find] = currentProject()->findTypeInfo(type);
                    if (find) {
                        switch (typeInfo.render.kind) {
                        case TypeInfoRenderKind::ComboBox:
                            port.value.u.i = valueNode.as<int>();
                            break;
                        case TypeInfoRenderKind::Default:
                        default:
                            dbg("no-render-function", type, getTypeName(type));
                            break;
                        }
                    }
                } else {
                    dbg("type error, unHandled", type, getTypeName(type));
                }
            } break;
            }
        }

        port.oldValue = port.value;
        return CODE_OK;
    }

    int loadNodeData(YAML::Node const& yamlNode, SightNode*& node, bool useOldId) {
        auto yamlTemplateNode = yamlNode["template"];
        if (!yamlTemplateNode.IsDefined()) {
            return CODE_FAIL;
        }
        auto templateNodeAddress = yamlTemplateNode.as<std::string>("");
        if (templateNodeAddress.empty()) {
            return CODE_NO_TEMPLATE_ADDRESS;
        }
        auto templateNode = g_NodeEditorStatus->findTemplateNode(templateNodeAddress.c_str());
        if (!templateNode) {
            dbg(templateNodeAddress, " template not found.");
            return CODE_TEMPLATE_ADDRESS_INVALID;
        }

        int status = CODE_OK;
        auto nodePointer = useOldId ? templateNode->instantiate(false) : templateNode->instantiate(true);
        auto& sightNode = *nodePointer;
        if (useOldId) {
            sightNode.nodeId = yamlNode["id"].as<int>();
        }
        sightNode.nodeName = yamlNode["name"].as<std::string>();

        auto position = yamlNode["position"];
        if (position.IsDefined()) {
            position >> sightNode.position;
        }

        auto inputs = yamlNode["inputs"];
        for (const auto& input : inputs) {
            if (loadPortInfo(input, NodePortType::Input, sightNode.inputPorts, useOldId) != CODE_OK) {
                status = CODE_GRAPH_BROKEN;   
            }
        }

        auto outputs = yamlNode["outputs"];
        for (const auto& output : outputs) {
            if (loadPortInfo(output, NodePortType::Output, sightNode.outputPorts, useOldId) != CODE_OK) {
                status = CODE_GRAPH_BROKEN;
            }
        }

        auto fields = yamlNode["fields"];
        for (const auto& field : fields) {
            if (loadPortInfo(field, NodePortType::Field, sightNode.fields, useOldId) != CODE_OK) {
                status = CODE_GRAPH_BROKEN;
            }
        }

        node = nodePointer;
        return status;
    }

    int regenerateId(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections, bool genConId) {
        auto nodeFunc = [&connections](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                auto oldId = item.id;
                item.id = nextNodeOrPortId();

                for (auto& c : connections) {
                    if (c.left == oldId) {
                        c.left = item.id;
                    } else if (c.right == oldId) {
                        c.right = item.id;
                    }
                }
            }
        };
    
        for (auto& item : nodes) {
            item->nodeId = nextNodeOrPortId();

            CALL_NODE_FUNC(item);
        }

        if (genConId) {
            for(auto& item: connections){
                item.connectionId = nextNodeOrPortId();
            }
        }

        return CODE_OK;
    }

    SightAnyThingData::SightAnyThingData() {
        this->node = nullptr;
    }

    SightAnyThingData::SightAnyThingData(SightNodeConnection *connection) {
        this->connection = connection;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node) {
        this->node = node;
    }

    SightAnyThingData::SightAnyThingData(SightNode *node, uint portId) {
        this->portHandle = { node, portId };
    }

    bool SightNodePortConnection::bad() const {
        return !self || !target || !connection;
    }

    SightNodePortConnection::SightNodePortConnection(SightNodeGraph* graph,SightNodeConnection *connection, SightNode* node) {
        auto leftPointer = graph->findPort(connection->left);
        auto rightPointer = graph->findPort(connection->right);
        this->connection = connection;

        if (leftPointer->node == node) {
            self = leftPointer;
            target= rightPointer;

        } else if (rightPointer->node == node) {
            self = rightPointer;
            target = leftPointer;
        }
    }

    SightNodePortConnection::SightNodePortConnection(SightNodePort *self, SightNodePort *target,
                                                     SightNodeConnection *connection) : self(self), target(target),
                                                                                        connection(connection) {}

    void SightNodeValue::setType(uint type) {
        if (this->type == type) {
            return;
        }

        if (this->type == IntTypeLargeString) {
            stringFree();
        }

        this->type = type;
        if (this->type == IntTypeLargeString) {
            stringInit();
        }
    }

    void SightNodeValue::stringInit() {
        auto & largeString = u.largeString;
        // largeString.bufferSize = NAME_BUF_SIZE * 2;
        largeString.bufferSize = 10;
        largeString.size = 0;
        largeString.pointer = (char*)calloc(1, largeString.bufferSize);

        // dbg(largeString.bufferSize, largeString.pointer, largeString.size);
    }

    bool SightNodeValue::stringCheck(size_t needSize) {
        auto& largeString = u.largeString;
        if (largeString.bufferSize > needSize) {
            return false;
        }
        stringFree();
        // static constexpr size_t tmpSize = NAME_BUF_SIZE * 2;
        static constexpr size_t tmpSize = 10;

        largeString.bufferSize = needSize < tmpSize ? tmpSize : static_cast<size_t>(needSize * 1.5f);
        largeString.size = 0;
        largeString.pointer = (char*)calloc(1, largeString.bufferSize);
        return true;
    }

    void SightNodeValue::stringFree() {
        auto& largeString = u.largeString;
        if (largeString.pointer) {
            free(largeString.pointer);
            largeString.pointer = nullptr;
            largeString.size = largeString.bufferSize = 0;
        }
    }

    void SightNodeValue::stringCopy(std::string const& str) {
        stringCheck(str.size());

        auto& largeString = u.largeString;
        sprintf(largeString.pointer, "%s", str.c_str());
        largeString.size = str.size();
    }

    SightNodeValue::SightNodeValue(uint type)
    {
        setType(type);
    }

    // SightNodeValue SightNodeValue::copy(int type) const {
    //     SightNodeValue tmp;
        
    //     if (type == IntTypeLargeString) {
    //         // large string
    //         tmp.stringCopy(this->u.largeString.pointer);
    //     } else {
    //         memcpy(tmp.u.string, this->u.string, std::size(this->u.string));
    //     }
    //     return tmp;
    // }

    SightNodeValue::SightNodeValue(SightNodeValue const& rhs)
    {
        this->type = rhs.type;
        if (type == IntTypeLargeString) {
            this->stringCopy(rhs.u.largeString.pointer);
        } else {
            this->u = rhs.u;
        }
    }

    SightNodeValue& SightNodeValue::operator=(SightNodeValue const& rhs) {
        if (this != &rhs) {
            this->type = rhs.type;
            if (this->type == IntTypeLargeString) {
                // stringFree();
                stringCopy(rhs.u.largeString.pointer);
            } else {
                this->u = rhs.u;
            }
        }
        return *this;
    }

    SightNodeValue::~SightNodeValue()
    {
        if (this->type == IntTypeLargeString) {
            stringFree();
        }
    }

    ScriptFunctionWrapper::ScriptFunctionWrapper(Function const& f)
        : function(f) {
    }

    ScriptFunctionWrapper::ScriptFunctionWrapper(std::string const& code)
        : sourceCode(code)
    {
        
    }

    void ScriptFunctionWrapper::compile(Isolate* isolate) {
        if (!function.IsEmpty() || sourceCode.empty()) {
            return;
        }

        auto f = recompileFunction(isolate, sourceCode);
        if (!f.IsEmpty()) {
            function = Function(isolate, f);
            sourceCode.clear();
        }
    }

    void ScriptFunctionWrapper::operator()(Isolate* isolate, SightNode* node) const {
        if (!checkFunction(isolate)) {
            return;
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto recv = v8pp::class_<SightNode>::reference_external(isolate, node);

        Local<Value> args[0];
        auto result = f->Call(context, recv, 0, args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        if (result.IsEmpty()) {
            return;
        }
        
    }

    void ScriptFunctionWrapper::operator()(Isolate* isolate, SightNodePort* thisNodePort, JsEventType eventType, void* pointer) const {
        if (!checkFunction(isolate) || !thisNodePort) {
            return;
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto node = thisNodePort->node;
        auto recv = nodePortValue(isolate, { node, thisNodePort->id});

        // prepare args
        Local<Value> args[2];
        args[0] = v8pp::class_<SightNode>::reference_external(isolate, node);
        args[1] = Object::New(isolate);
        if (eventType == JsEventType::AutoComplete) {

        } else if (eventType == JsEventType::Connect) {
            args[1] = connectionValue(isolate, (SightNodeConnection *)pointer, thisNodePort);
        } else if (eventType == JsEventType::ValueChange) {
            args[1] = getPortValue(isolate, thisNodePort->getType(), *((const SightNodeValue*)pointer));
        }

        // call function
        auto mayResult = f->Call(context, recv, std::size(args), args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        // v8pp::cleanup(isolate);
        if (mayResult.IsEmpty()) {
            return;
        }

        // after call function
        if (eventType == JsEventType::AutoComplete) {
            // 
            auto result = mayResult.ToLocalChecked();
            auto & options = thisNodePort->options;
            options.alternatives.clear();
            if (!result->IsNullOrUndefined()) {
                if (result->IsArray()) {
                    auto list = v8pp::from_v8<std::vector<std::string>>(isolate, result);
                    options.alternatives.assign(list.begin(), list.end());
                } else if (IS_V8_STRING(result)) {
                    options.alternatives.push_back(v8pp::from_v8<std::string>(isolate, result));
                }
            }
        }
    }

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::operator()(Isolate* isolate, SightNode* node, v8::Local<v8::Value> arg1, v8::Local<v8::Value> arg2) const {
        if (!checkFunction(isolate)) {
            return {};
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto recv = v8pp::class_<SightNode>::reference_external(isolate, node);

        Local<Value> args[2];
        args[0] = arg1;
        args[1] = arg2;
        auto result = f->Call(context, recv, std::size(args), args);
        v8pp::class_<SightNode>::unreference_external(isolate, node);

        return result;
    }

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::operator()(Isolate* isolate, SightEntity* entity) const {
        auto obj = v8pp::class_<SightEntity>::reference_external(isolate, entity);
        auto result = v8pp::call_v8(isolate, function.Get(isolate), v8::Object::New(isolate), obj);
        v8pp::class_<SightEntity>::unreference_external(isolate, entity);
        return result;
    }

    ScriptFunctionWrapper::operator bool() const {
        return !function.IsEmpty() || !sourceCode.empty();
    }

    bool ScriptFunctionWrapper::checkFunction(Isolate* isolate) const {
        if (function.IsEmpty()) {
            // dbg(sourceCode);
            if (!sourceCode.empty()) {
                const_cast<ScriptFunctionWrapper*>(this)->compile(isolate);
            } else {
                return false;
            }

            return !function.IsEmpty();
        }

        return true;
    }

    SightBaseNodePort::SightBaseNodePort(uint type) : type(type), value(type)
    {
        
    }

    void SightBaseNodePort::setKind(int intKind) {
        this->kind = NodePortType(intKind);
    }

    uint SightBaseNodePort::getType() const {
        return this->type;
    }

    const char* SightBaseNodePort::getPortName() const {
        return portName.c_str();
    }

    SightNodeGraph* SightNodePort::getGraph() {
        if (this->node) {
            return this->node->graph;
        }
        return nullptr;
    }

    const SightNodeGraph* SightNodePort::getGraph() const {
        return const_cast<SightNodePort*>(this)->getGraph();
    }

    SightNodePort::operator SightNodePortHandle() const {
        return {this->node, this->id};
    }

    SightJsNodePort::SightJsNodePort(std::string const& name, NodePortType kind)
    {
        this->portName = name;
        this->kind = kind;
    }

    SightNodePort SightJsNodePort::instantiate() const {
        SightNodePort tmp = { this->kind, this->type, this};
        tmp.value = this->value;
        tmp.portName = this->portName;
        tmp.options = this->options;
        return tmp;
    }

    int NodeEditorStatus::createGraph(const char* path) {
        if (graph) {
            return -1;     // has one.
        }

        graph = new SightNodeGraph();
        graph->saveToFile(path, true);
        return 0;
    }

    int NodeEditorStatus::loadOrCreateGraph(const char* path) {
        if (graph) {
            return CODE_FAIL;
        }

        graph = new SightNodeGraph();
        if (graph->load(path) != CODE_OK) {
            // if (!graph->isBroken()) {
            //     // create one.
            //     graph->save();
            // } else {

            // }

            // create one.
            graph->save();
        }

        return CODE_OK;
    }

    SightJsNode* NodeEditorStatus::findTemplateNode(const char* path) {
        auto address = resolveAddress(path);
        auto pointer = address;
        // compare and put.
        auto* list = &g_NodeEditorStatus->templateAddressList;

        bool findElement = false;
        SightJsNode* result = nullptr;
        while (pointer) {
            for (auto iter = list->begin(); iter != list->end(); iter++) {
                if (iter->name == pointer->part) {
                    if (!pointer->next) {
                        // the node
                        result = iter->templateNode;
                    }

                    findElement = true;
                    list = &iter->children;
                    break;
                }
            }
            if (!findElement || result) {
                break;
            }

            pointer = pointer->next;
        }


        freeAddress(address);
        return result;
    }

    NodeEditorStatus::NodeEditorStatus() {
    }

    NodeEditorStatus::~NodeEditorStatus() {
        // todo free memory.
    }

    Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs) {
        return {lhs.x - rhs.x, lhs.y - rhs.y};
    }

    Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs) {
        return {lhs.x + rhs.x, lhs.y + rhs.y};
    }

    int initNodeStatus() {
        if (g_NodeEditorStatus) {
            return CODE_FAIL;
        }

        g_NodeEditorStatus = new NodeEditorStatus();
        return CODE_OK;
    }

    int destoryNodeStatus() {
        if (!g_NodeEditorStatus) {
            return CODE_FAIL;
        }

        disposeGraph();

        delete g_NodeEditorStatus;
        g_NodeEditorStatus = nullptr;
        return CODE_OK;
    }

    CommonOperation::CommonOperation(std::string name, std::string description, ScriptFunctionWrapper::Function const& f)
        : name(std::move(name)),
          description(std::move(description)),
          function(f)
    {
        
    }


}