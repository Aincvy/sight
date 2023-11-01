//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/20.
//
#include <cassert>
#include <corecrt_wstdio.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <ios>
#include <iterator>
#include <set>
#include <stdio.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <yaml-cpp/emittermanip.h>

#include "sight_js_parser.h"
#include "sight_node.h"
#include "sight_node_graph.h"
#include "sight_defines.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_js.h"
#include "sight_address.h"
#include "sight_util.h"
#include "sight_project.h"
#include "sight_ui.h"
#include "sight_log.h"

#include "v8-json.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"
#include "v8.h"
#include "v8pp/call_v8.hpp"
#include "v8pp/convert.hpp"
#include "v8pp/class.hpp"

#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"
#include "v8pp/object.hpp"

#include "crude_json.h"

#ifdef NOT_WIN32
#    include <sys/termios.h>
#endif


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

    NodeEditorStatus* currentNodeStatus() {
        return g_NodeEditorStatus;
    }

    void disposeGraph() {
        if (CURRENT_GRAPH) {
            delete CURRENT_GRAPH;
            CURRENT_GRAPH = nullptr;
        }
    }

    void changeGraph(const char* pathWithoutExt, v8::Isolate* isolate) {
        logDebug(pathWithoutExt);
        disposeGraph();

        char buf[FILENAME_BUF_SIZE];
        sprintf(buf, "%s.yaml", pathWithoutExt);
        g_NodeEditorStatus->loadOrCreateGraph(buf);

        if (CURRENT_GRAPH) {
            CURRENT_GRAPH->callNodeEventsAfterLoad(isolate);
        }
        logDebug("change over!");
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
            return c1->priority > c2->priority;
        });

    }

    SightNode* SightNodePort::getNode() {
        return this->node;
    }

    uint SightNodePort::getNodeId() const {
        return node->getNodeId();
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
        } else {
            logError("unHandle port kind: $0", (int)port.kind);
        }

    }

    int SightNode::addNewPort(std::string_view name, NodePortType kind, uint type, const SightJsNodePort* templateNodePort, uint parent) {
        assert(kind != NodePortType::Both);
        if (kind == NodePortType::Input) {
            for(auto const& item: this->inputPorts){
                if(item.portName == name){
                    return CODE_PORT_NAME_REPEAT;
                }
            }
        } else if (kind == NodePortType::Output) {
            for(auto const& item: this->outputPorts){
                if(item.portName == name){
                    return CODE_PORT_NAME_REPEAT;
                }
            }
        } else if (kind == NodePortType::Field) {
            for(auto const& item: this->fields){
                if(item.portName == name){
                    return CODE_PORT_NAME_REPEAT;
                }
            }
        } else {
            assert(false);
        }
        
        SightNodePort port(kind, type, templateNodePort);
        port.portName = name;
        logDebug("add new port: $0", port.portName);
        port.node = this;
        port.id = nextNodeOrPortId();
        port.parent = parent;
        port.ownOptions.dynamicPort = true;

        addPort(port);
        if(graph){
            graph->addPortId(port);
            graph->markDirty();
        }

        return CODE_OK;
    }

    int SightNode::addNewPort(std::string_view name, SightNodePort& from) {
        return addNewPort(name, from.kind, from.type, from.templateNodePort, from.getId());
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

    SightNode *SightNode::clone(bool generateId /* = true */) const{
        auto p = new SightNode();
        p->graph = this->graph;
        p->copyFrom(this, CopyFromType::Clone, generateId);

        return p;
    }

    void SightNode::copyFrom(const SightNode* node, CopyFromType copyFromType, bool generateId /* = true */) {
        assert(this->nodeId == 0);     // only empty node can copy from other node.
        if (copyFromType == CopyFromType::Duplicate && !generateId) {
            logWarning("duplicate a node, generateId must be true!");
            generateId = true;
        }

        this->nodeId = node->nodeId;
        this->nodeName = node->nodeName;
        this->position = node->position;
        this->templateNode = node->templateNode;

        if (generateId) {
            graph->markDirty();
            this->nodeId = nextNodeOrPortId();

            if (copyFromType == CopyFromType::Duplicate) {
                // ignore other types
                graph->addNodeId(this);
            }
        }

        auto copyFunc = [copyFromType, this, generateId](std::vector<SightNodePort> const& src, std::vector<SightNodePort>& dst) {
            for (const auto &item : src) {
                dst.push_back(item);
                auto& port = dst.back();
                port.node = this;
                if (generateId) {
                    port.id = nextNodeOrPortId();
                }
                
                if (copyFromType == CopyFromType::Instantiate) {
                    // init something
                    // auto& back = dst.back();
                    // if (back.getType() == IntTypeLargeString) {
                    //     back.value.stringInit();
                    // }
                } else if (copyFromType == CopyFromType::Duplicate || copyFromType == CopyFromType::Component) {
                    // 
                    port.connections.clear();
                    port.oldValue = {};
                    
                    graph->addPortId(port);
                }
            }
        };

        copyFunc(node->inputPorts, this->inputPorts);
        copyFunc(node->outputPorts, this->outputPorts);
        copyFunc(node->fields, this->fields);

        // copy components
        if (copyFromType != CopyFromType::Component && node->componentContainer) {
            this->componentContainer = graph->deepClone(node->componentContainer);
        }

        if (copyFromType == CopyFromType::Duplicate) {
            updateChainPortPointer();
            graph->markDirty();
        }
    }

    void SightNode::markAsDeleted(bool f) {

        if (f) {
            this->flags |= (uchar)SightNodeFlags::Deleted;
        } else {
            this->flags &= ~(uchar)SightNodeFlags::Deleted;
        }
    }

    void SightNode::markAsComponent(bool f) {
        if (f) {
            this->flags |= (uchar)SightNodeFlags::Component;
        } else {
            this->flags &= ~(uchar)SightNodeFlags::Component;
        }
    }

    bool SightNode::isDeleted() const {
        return (this->flags & (uchar)SightNodeFlags::Deleted) != 0;
        
    }

    bool SightNode::isComponent() const {
        return (this->flags & (uchar)SightNodeFlags::Component) != 0;        
    }

    bool SightNode::hasConnections() const {

        // check connections
        auto hasConnections = [](std::vector<SightNodePort> const& list) {
            for (auto& item : list) {
                if (item.isConnect()) {
                    return true;
                }
            }
            return false;
        };

        return hasConnections(inputPorts) || hasConnections(outputPorts);
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
        logDebug("node reset");
        // call events
        if (templateNode) {
            auto t = dynamic_cast<const SightJsNode*>(templateNode);
            if (t) {
                t->onDestroyed(currentUIStatus()->isolate, this);
            }
        }

        if (this->componentContainer) {
            graph->removeComponentContainer(this->componentContainer);
            this->componentContainer = nullptr;
        }

        auto nodeFunc = [this](std::vector<SightNodePort>& list) {
            // loop
            for (auto& item : list) {
                if (item.isConnect()) {
                    logError("node reset, but a port is connected.. $0, port: $1", this->nodeId, item.getId());
                }
            }
            list.clear();
        };

        CALL_NODE_FUNC(this);

        this->nodeId = 0;
        this->nodeName = "";
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

    bool SightNode::checkAsComponent() const {
        return templateNode->component.active;
    }

    bool SightNode::addComponent(SightJsNode* templateNode) {
        if (!templateNode || !templateNode->checkAsComponent()) {
            return false;
        }

        auto p = templateNode->instantiate(this->graph);
        addComponent(p);
        
        return true;
    }

    bool SightNode::addComponent(SightNode* p) {
        if (!p || p == this) {
            return false;
        }

        getComponentContainer()->addComponent(p);
        return true;
    }

    std::vector<uint> SightNode::findChildrenPorts(uint parentPortId) const {
        std::vector<uint> result;
        auto nodeFunc = [&result, parentPortId](std::vector<SightNodePort> const& list) {
            for(const auto& item: list) {
                if(item.parent == parentPortId) {
                    result.push_back(item.getId());
                }
            }
        };

        CALL_NODE_FUNC(this);
        return result;
    }

    SightComponentContainer* SightNode::getComponentContainer() {
        if (!componentContainer) {
            
            this->componentContainer = graph->createComponentContainer();
        }
        return componentContainer;
    }

    v8::Local<v8::Object> SightNode::toV8Object(v8::Isolate* isolate) const {
        auto object = v8::Object::New(isolate);
        auto context = isolate->GetCurrentContext();
        auto nodeFunc = [&object, isolate, &context](std::vector<SightNodePort> const& list) {
            for (const auto& item : list) {
                auto v = getPortValue(isolate, item.getType(), item.value);
                auto key = v8pp::to_v8(isolate, item.portName);

                // check contains
                auto hasResult = object->Has(context, key);
                if (hasResult.IsJust() && hasResult.FromJust()) {
                    // already exists
                    logWarning("port name already exists: $0", item.portName);
                    continue;
                }

                object->Set(context, key, v).ToChecked();
            }
        };

        // CALL_NODE_FUNC(this);

        nodeFunc(this->fields);
        nodeFunc(this->inputPorts);
        nodeFunc(this->outputPorts);

        auto key = v8pp::to_v8(isolate, "@@nodeId");
        auto hasResult = object->Has(context, key);
        // if do not contains, then add to object
        if (hasResult.IsJust() && hasResult.FromJust()) {

        } else {
            object->Set(context, key, v8pp::to_v8(isolate, this->nodeId)).ToChecked();
        }

        return object;
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

    SightNode* SightJsNode::instantiate(bool generateId) const {
        logDebug(this->nodeName);
        auto p = new SightNode();
        
        this->instantiate(p, generateId, currentGraph());
        return p;
    }

    SightNode* SightJsNode::instantiate(SightNodeGraph* graph, bool generateId) const {
        auto p = graph->getNodes().add();
        p->graph = graph;

        this->instantiate(p, generateId, graph);
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

        // node name width
        auto titleBarWidth = ImGui::CalcTextSize(this->nodeName.c_str()).x;
        titleBarWidth += (SightNodeFixedStyle::iconSize + SightNodeFixedStyle::iconTitlePadding) * 2 + SightNodeFixedStyle::iconTitlePadding * 2;

        style.width = std::max(style.width, titleBarWidth);
        style.initialized = true;
    }

    bool SightJsNode::isStyleInitialized() const {
        return nodeStyle.initialized;
    }

    void SightJsNode::callEventOnInstantiate(SightNode* p) const {
        onInstantiate(currentUIStatus()->isolate, p);
    }

    bool SightJsNode::checkAsComponent() const {
        return component.active;
    }

    void SightJsNode::instantiate(SightNode* p, bool generateId, SightNodeGraph* graph) const {
        auto portCopyFunc = [](std::vector<SightJsNodePort*> const& src, std::vector<SightNodePort>& dst) {
            for (const auto& item : src) {
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

        if (!graph) {
            graph = currentGraph();
        }
        p->graph = graph;

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
    }

    SightNodeConnection::SightNodeConnection() {
    }
    SightNodeConnection::SightNodeConnection(uint id, uint left, uint right, uint leftColor, int priority)
        : connectionId(id), left(left), right(right), leftColor(leftColor), priority(priority) {
    }

    void SightNodeConnection::removeRefs() {
        
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

    void SightNodeConnection::makeRefs() {
    
        auto leftPort = graph->findPort(leftPortId());
        auto rightPort = graph->findPort(rightPortId());

        if (leftPort) {
            auto& c = leftPort->connections;
            // check has already ?
            if (std::find(c.begin(), c.end(), this) == c.end()) {
                c.push_back(this);
                leftPort->sortConnections();
            } else {
                // already exists
                logError("connection already exists, port: $0, connection: $1", leftPortId(), connectionId);
            }
            
        }
        if (rightPort) {
            auto& c = rightPort->connections;
            // check has already ?
            if (std::find(c.begin(), c.end(), this) == c.end()) {
                c.push_back(this);
                rightPort->sortConnections();
            } else {
                // already exists
                logError("connection already exists, port: $0, connection: $1", rightPortId(), connectionId);
            }            
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

    SightComponentContainer*   SightNodeConnection::getComponentContainer() {
        if (!componentContainer) {
            componentContainer = graph->createComponentContainer();
        }
        return componentContainer;
    }

    bool SightNodeConnection::addComponent(SightJsNode* templateNode) {
        return getComponentContainer()->setGraph(this->graph)-> addComponent(templateNode);
    }

    bool SightNodeConnection::addComponent(SightNode* sightNode) {
        return getComponentContainer()->setGraph(this->graph)->addComponent(sightNode);
    }

    void SightNodeConnection::reset() {
        this->connectionId = 0;
        this->left = this->right = 0;
        this->priority = 10;
        this->generateCode = true;
        this->leftColor = IM_COL32_WHITE;

        if (this->componentContainer) {
            graph->removeComponentContainer(this->componentContainer);
            this->componentContainer = nullptr;
        }
    }

    void SightNodeConnection::markAsDeleted(bool f) {

        if (f) {
            this->flags |= (uchar)SightNodeFlags::Deleted;
        } else {
            this->flags &= ~(uchar)SightNodeFlags::Deleted;
        }
    }

    void SightNodeConnection::markAsComponent(bool f) {

        if (f) {
            this->flags |= (uchar)SightNodeFlags::Component;
        } else {
            this->flags &= ~(uchar)SightNodeFlags::Component;
        }
    }

    bool SightNodeConnection::isDeleted() const {
        return (this->flags & (uchar)SightNodeFlags::Deleted) != 0;
    }

    bool SightNodeConnection::isComponent() const {
        return (this->flags & (uchar)SightNodeFlags::Component) != 0;
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

    bool SightNodeTemplateAddress::isAnyItemCanShow(bool filterComponent, SightAnyThingType appendToType) const {
        if (filterComponent) {
            if (children.empty()) {
                if (templateNode && templateNode->checkAsComponent()) {
                    if (appendToType == SightAnyThingType::Node) {
                        return templateNode->component.allowNode;
                    } else if (appendToType == SightAnyThingType::Connection) {
                        return templateNode->component.allowConnection;
                    }
                    return false;
                }
            } else {
                for (const auto& item : children) {
                    if (item.isAnyItemCanShow(filterComponent, appendToType)) {
                        return true;
                    }
                }
            }
            return false;
        } else {
            // todo create node filter.
        }
        return true;
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

    int addTemplateNode(const SightNodeTemplateAddress& templateAddress, bool isUpdate) {
        logDebug(templateAddress.name);

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

                if (findResult == list->end()) {
                    // not found
                    templateNode->compact();
                    auto tmpPointer = g_NodeEditorStatus->templateNodeArray.add(*templateNode);
                    if (isUpdate) {   
                        tmpPointer->nodeStyle.initialized = false;
                    }
                    auto tmpAddress = SightNodeTemplateAddress(pointer->part, tmpPointer);
                    tmpAddress.setNodeMemoryFromArray();
                    list->push_back(tmpAddress);
                } else {
                    // find
                    std::string tmpMsg = "replace template node: ";
                    tmpMsg += pointer->part;
                    logDebug(tmpMsg);
                    auto willCopy = *templateNode;

                    // src1: pointers, src2:
                    auto copyFunc = [](std::vector<SightJsNodePort*>& dst, std::vector<SightJsNodePort*>& src1, std::vector<SightJsNodePort>& src2) {
                        if (!dst.empty()) {
                            logDebug("Oops! dst not empty, clear!");
                            dst.clear();
                        }
                        
                        auto& templateNodePortArray = g_NodeEditorStatus->templateNodePortArray;

                        // clear old data.
                        for(auto& item: src1){
                            templateNodePortArray.remove(item);
                        }
                        src1.clear();

                        // copy data
                        for( const auto& item: src2){
                            dst.push_back(templateNodePortArray.add(item));
                        }
                        src2.clear();

                    };

                    // copy and compact
                    // fields, inputPorts, outputPorts
                    copyFunc(willCopy.fields, findResult->templateNode->fields, willCopy.originalFields);
                    copyFunc(willCopy.inputPorts, findResult->templateNode->inputPorts, willCopy.originalInputPorts);
                    copyFunc(willCopy.outputPorts, findResult->templateNode->outputPorts, willCopy.originalOutputPorts);

                    if (isUpdate) {
                        willCopy.nodeStyle.initialized = false;
                    }
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

            if(item.parent){
                out << YAML::Key << "parent" << YAML::Value << item.parent;
            }

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

            out << YAML::Key << "options" << YAML::BeginMap;
            out << YAML::Key << "customPortName" << YAML::Value << item.ownOptions.customPortName ;
            out << YAML::Key << "dynamicPort" << YAML::Value << item.ownOptions.dynamicPort ;
            out << YAML::EndMap;

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

        // components
        if (node.componentContainer) {
            out << YAML::Key << "components" << YAML::Value << *node.componentContainer;
        }
        
        out << YAML::EndMap;   // end of node data.

        return out;
    }

    YAML::Emitter& operator<< (YAML::Emitter& out, const SightNodeConnection& connection) {
        out << YAML::BeginMap;
        out << YAML::Key << "id" << YAML::Value << connection.connectionId;
        out << YAML::Key << "left" << YAML::Value << connection.leftPortId();
        out << YAML::Key << "right" << YAML::Value << connection.rightPortId();
        out << YAML::Key << "priority" << YAML::Value << connection.priority;
        out << YAML::Key << "generateCode" << YAML::Value << connection.generateCode;

        // components
        if (connection.componentContainer) {
            out << YAML::Key << "components" << YAML::Value << *connection.componentContainer;
        }
        out << YAML::EndMap;
        return out;
    }

    YAML::Emitter& operator<<(YAML::Emitter& out, const SightComponentContainer& componentContainer) {
        out << YAML::BeginMap;
        for (const auto& item : componentContainer.components) {
            out << YAML::Key << item->nodeId;
            out << YAML::Value << *item;
        }
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
        if (node["generateCode"]) {
            connection.generateCode = node["generateCode"].as<bool>();
        }

        auto componentsNode = node["components"];
        if (componentsNode.IsDefined()) {
            auto componentsContainer = connection.getComponentContainer();
            
            componentsNode >> *componentsContainer;
        }
        return true;
    }

    bool operator>>(YAML::Node const& node, SightComponentContainer& componentContainer) {
        for (auto& item : node) {
            SightNode* tmp = nullptr;

            if (loadNodeData(item.second, tmp) == CODE_OK) {
                componentContainer.addComponent(tmp);
            }
        }

        return false;
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

        auto optionsYamlNode = values["options"];
        auto dynamicPort = false;
        std::string customPortName = "";
        if (optionsYamlNode.IsDefined())
        {
            dynamicPort = optionsYamlNode["dynamicPort"].as<bool>();
            customPortName = optionsYamlNode["customPortName"].as<std::string>();
        }

        SightNodePort* pointer = nullptr;
        if(dynamicPort) {
            // 
            list.push_back({});
            pointer = & (list.back());
        } else {
            for (auto& item : list) {
                if (item.portName == portName) {
                    pointer = &item;
                    break;
                }
            }
        }
        
        if (!pointer) {
            logDebug("$0 not found", portName);
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
                            logDebug("no-render-function: $0, $1", type, getTypeName(type));
                            break;
                        }
                    }
                } else {
                    logDebug("type error, unHandled: $0, $1", type, getTypeName(type));
                }
            } break;
            }
        }

        port.oldValue = port.value;

        if(!customPortName.empty()){
            port.ownOptions.customPortName = customPortName;
        }
        port.ownOptions.dynamicPort = dynamicPort;
        auto parentNode = values["parent"];
        if (parentNode.IsDefined())
        {
            port.parent = parentNode.as<uint>();
        }

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
            logDebug("template not found: $0", templateNodeAddress, " template not found.");
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

        auto componentsNode = yamlNode["components"];
        if (componentsNode.IsDefined()) {
            auto componentsContainer = nodePointer->getComponentContainer();
            componentsNode >> *componentsContainer;
        }

        // fix no-data-port's id.   Happened when template node add any port(s).
        auto nodeFunc = [](std::vector<SightNodePort> & list){
            for(auto& item: list){
                if (item.id <= 0) {
                    item.id = nextNodeOrPortId();   
                }
            }
        };
        CALL_NODE_FUNC(nodePointer);

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

    uint SightNodeValue::getType() const {
        return this->type;
    }

    bool SightNodeValue::setValue(std::string_view str) {

        auto vectorData = [this, str](const int size) {
            std::vector<std::string> data = absl::StrSplit(str, ",");
            if (data.size() != size) {
                return false;
            }

            auto p = u.vector3;
            for (int i = 0; i < data.size(); i++) {
                float f = 0;
                if (!absl::SimpleAtof(data[i], &f)) {
                    return false;
                }
                p[i] = f;
            }
            return true;
        };

        switch (type) {
        case IntTypeInt:
        case IntTypeLong:
            return absl::SimpleAtoi(str, &u.i);
        case IntTypeString:
            if (str.length() >= std::size(u.string)) {
                return false;
            }
            // sprintf(u.string, "%s", str.data());
            str.copy(u.string, std::size(u.string));
            return true;
        case IntTypeLargeString:
            stringCopy(std::string{ str.data() });
            return true;
        case IntTypeBool:
            return absl::SimpleAtob(str, &u.b);
        case IntTypeFloat:
            return absl::SimpleAtof(str, &u.f);
        case IntTypeDouble:
            return absl::SimpleAtod(str, &u.d);
        case IntTypeVector3:
            return vectorData(3);
        case IntTypeVector4:
        case IntTypeColor:
            return vectorData(4);
        }

        return false;
    }

    void SightNodeValue::stringInit() {
        auto & largeString = u.largeString;
        // largeString.bufferSize = NAME_BUF_SIZE * 2;
        largeString.bufferSize = 10;
        largeString.size = 0;
        largeString.pointer = (char*)calloc(1, largeString.bufferSize);

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

    const char* SightNodeValue::getString() const {
        return u.string;
    }

    const char* SightNodeValue::getLargeString() const {
        return u.largeString.pointer;    
    }

    std::string SightNodeValue::getLargeStringCopy() const {
        return std::string(u.largeString.pointer, u.largeString.size);
    }

    SightNodeValue::SightNodeValue(uint type)
    {
        setType(type);
    }

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

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::operator()(Isolate* isolate, SightNode* node) const {
        if (!checkFunction(isolate)) {
            return {};
        }
        using namespace v8;

        v8::HandleScope handleScope(isolate);
        auto f = function.Get(isolate);
        auto context = isolate->GetCurrentContext();
        auto recv = v8pp::class_<SightNode>::reference_external(isolate, node);

        auto result = f->Call(context, recv, 0, nullptr);
        v8pp::class_<SightNode>::unreference_external(isolate, node);
        return result;
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

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::operator()(Isolate* isolate, int index, const char* graphName) const {
        auto obj = v8::Object::New(isolate);
        v8pp::set_const(isolate, obj, "index", index);
        v8pp::set_const(isolate, obj, "graphName", graphName);
        return v8pp::call_v8(isolate, function.Get(isolate), v8::Object::New(isolate), obj);
    }

    ScriptFunctionWrapper::operator bool() const {
        return !function.IsEmpty() || !sourceCode.empty();
    }

    v8::MaybeLocal<v8::Value> ScriptFunctionWrapper::callByReadonly(Isolate* isolate, SightNode* node) const {
        if (!checkFunction(isolate)) {
            return {};
        }

        auto obj = node->toV8Object(isolate);
        return v8pp::call_v8(isolate, this->function.Get(isolate), obj);
    }

    bool ScriptFunctionWrapper::checkFunction(Isolate* isolate) const {
        if (function.IsEmpty()) {
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
        tmp.id = 0;    // init 
        return tmp;
    }

    int NodeEditorStatus::createGraph(const char* path) {
        if (graph) {
            return -1;     // has one.
        }

        graph = new SightNodeGraph();
        graph->markDirty();
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
            graph->setName(graph->getFileName());
            graph->markDirty();
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

    void NodeEditorStatus::updateTemplateNodeStyles() {
        for(auto& item: templateNodeArray){
            item.updateStyle();
        }
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

    SightNodePortOptions::~SightNodePortOptions()
    {
        freeVarName();
    }

    void SightNodePortOptions::initVarName() {
        if(varNameLength > 0) {
            return;
        }

        varNameLength = NAME_BUF_SIZE;
        varName = (char*)calloc(1, sizeof(char) * varNameLength);
    }

    SightNodePortOptions& SightNodePortOptions::operator=(SightNodePortOptions const& rhs) {
        if(this != &rhs) {
            this->typeList = rhs.typeList;

            if(this->typeList){
                initVarName();
                sprintf(varName, "%s", rhs.varName);
            }
        }
        return *this;
    }

    void SightNodePortOptions::freeVarName() {
        if(varNameLength > 0){
            free(varName);
            varName = nullptr;
            varNameLength = 0;
        }
    }

    bool SightComponentContainer::addComponent(SightJsNode* templateNode) {
        
        if (!templateNode || !templateNode->checkAsComponent()) {
            return false;
        }

        auto p = templateNode->instantiate(this->graph);
        addComponent(p);
        return true;
    }

    bool SightComponentContainer::addComponent(SightNode* p) {
        
        auto nodeFunc = [p](std::vector<SightNodePort>& list) {
            for (auto& item : list) {
                item.node = p;
            }
        };
        CALL_NODE_FUNC(p);
        components.push_back(p);
        p->graph = graph;
        p->markAsComponent();

        graph->registerNodeIds(p);
        graph->markDirty();
        return true;
    }

    void SightComponentContainer::reset() {
    
        // means delete
        if (!this->components.empty()) {
            //delete all elements
            for (auto& item : this->components) {
                // delete item;
                graph->getNodes().remove(item);
            }

            this->components.clear();
        }

    }

}