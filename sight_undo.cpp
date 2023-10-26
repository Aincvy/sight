#include "sight_undo.h"

#include "imgui_node_editor.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_nodes.h"
#include "sight_ui_node_editor.h"

#include <atomic>
#include <cassert>
#include <string>
#include <vector>

#ifdef NOT_WIN32
#    include <sys/termios.h>
#endif

#define UNDO_LIST_COUNT 60

namespace ed = ax::NodeEditor;

namespace sight {

    static std::vector<UndoCommand> undoList;
    static std::vector<UndoCommand> redoList;

    UndoCommand::UndoCommand(uint id, UndoRecordType recordType)
        : id(id),
          recordType(recordType)
    {
        connectionData.connectionId = 0;
        nodeData.nodeId = 0;
    }

    UndoCommand::UndoCommand(uint id, UndoRecordType recordType, uint anyThingId)
        : UndoCommand(id, recordType)
    {
        this->anyThingId = anyThingId;
    }


    void UndoCommand::undo() {
        switch (recordType) {
        case UndoRecordType::Create:
        {
            auto g = currentGraph();
            if (g) {
                auto thing = g->findSightAnyThing(anyThingId);
                if (thing.type == SightAnyThingType::Node) {
                    g->delNode(anyThingId, &nodeData);
                } else if (thing.type == SightAnyThingType::Connection) {
                    g->delConnection(anyThingId, true, &connectionData);
                }
            }
            break;
        }
        case UndoRecordType::Delete: 
        {
            auto g = currentGraph();
            if (g) {
                if (connectionData.connectionId > 0) {
                    g->addConnection(connectionData);
                } else if (nodeData.nodeId > 0) {
                    g->addNode(nodeData);
                    ed::SetNodePosition(nodeData.nodeId, position);
                }
            }
            break;
        }
        case UndoRecordType::Update: 
        {
            auto g = currentGraph();
            if (g) {
                auto port = g->findPort(anyThingId);
                if (port) {
                    auto oldValue = port->oldValue;
                    port->value = portValueData;
                    onNodePortValueChange(port);
                    portValueData = oldValue;
                }
            }
            break;
        }
        }
    }

    void UndoCommand::redo() {
        switch (recordType) {
        case UndoRecordType::Create:
        {
            auto g = currentGraph();
            if (g) {
                if (nodeData.nodeId > 0) {
                    g->addNode(nodeData);
                    ed::SetNodePosition(nodeData.nodeId, position);
                } else if (connectionData.connectionId > 0) {
                    g->addConnection(connectionData);
                }
            }
            break;
        }
        case UndoRecordType::Delete:
        {
            auto g = currentGraph();
            if (g) {
                if (connectionData.connectionId > 0) {
                    g->delConnection(connectionData.connectionId);
                } else if (nodeData.nodeId > 0) {
                    g->delNode(nodeData.getNodeId());
                }
            }
            break;
        }
        case UndoRecordType::Update:
        {
            auto g = currentGraph();
            if (g) {
                auto port = g->findPort(anyThingId);
                if (port) {
                    auto oldValue = port->oldValue;
                    port->value = portValueData;
                    onNodePortValueChange(port);
                    portValueData = oldValue;
                }
            }
            break;
        }
        }
    }

    int getRuntimeId(StartOrStop type) {
        static std::atomic_int runtimeId = 1000;
        static int recordId = -1;

        if (type == StartOrStop::Start) {
            assert(recordId < 0);
            recordId = runtimeId++;
        } else if (type == StartOrStop::Stop) {
            recordId = -1;
        }

        if (recordId > 0) {
            return recordId;
        }
        return runtimeId++;
    }

    int initUndo() {
        undoList.reserve(UNDO_LIST_COUNT / 2);
        redoList.reserve(8);
        return CODE_OK;
    }

    int recordUndo(UndoRecordType recordType, uint anyThingId) {
        redoList.clear();
        undoList.emplace_back(getRuntimeId(), recordType, anyThingId);

        while (undoList.size() > UNDO_LIST_COUNT) {
            undoList.erase(undoList.begin());
        }
        return CODE_OK;
    }

    int recordUndo(UndoRecordType recordType, uint anyThingId, ImVec2 pos) {
        int t = recordUndo(recordType, anyThingId);
        assert(t == CODE_OK);
        lastUndoCommand()->position = pos;
        return CODE_OK;
    }

    int recordUndo(UndoRecordType recordType, uint anyThingId, ImVec2 pos, SightAnyThingWrapper anyThing) {
        int t = recordUndo(recordType, anyThingId);
        assert(t == CODE_OK);

        auto c = lastUndoCommand();
        c->position = pos;
        if (anyThing.type == SightAnyThingType::Connection) {
            c->connectionData = *anyThing.asConnection();
        } else if (anyThing.type == SightAnyThingType::Node) {
            c->nodeData = *anyThing.asNode();
        }
        return CODE_OK;
    }

    UndoCommand* lastUndoCommand() {
        if (!undoList.empty()) {
            return &undoList.back();
        }
        return nullptr;
    }

    bool isRedoEnable() {
        return !redoList.empty();
    }

    void redo() {
        if (redoList.empty()) {
            return;
        }

        auto redoId = redoList.back().id;
        while (!redoList.empty()) {
            auto& c = redoList.back();
            if (c.id != redoId) {
                break;
            }
            c.redo();

            undoList.push_back(c);
            redoList.pop_back();
        }
    }

    bool isUndoEnable() {
        return !undoList.empty();
    }

    void undo() {
        if (undoList.empty()) {
            return;
        }
        
        int undoId = undoList.back().id;
        while (!undoList.empty()) {
            auto& c = undoList.back();
            if (c.id != undoId) {
                break;
            }
            c.undo();

            redoList.push_back(c);
            undoList.pop_back();
        }
    }

    int CopyText::loadAsNode(SightNode*& node) const {
        return loadNodeData(this->data, node, false);
    }

    int CopyText::laodAsConnection(SightNodeConnection*& connection) {
        return CODE_NOT_IMPLEMENTED;
    }

    int CopyText::loadAsMultiple(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections) const {
        if (data.size() <= 0) {
            return CODE_FAIL;
        }
        assert(connections.empty());
        assert(nodes.empty());

        int status = CODE_OK;
        auto dataNodes = data["nodes"];
        if (dataNodes.IsDefined() && dataNodes.IsSequence()) {
            for (const auto& item : dataNodes) {
                SightNode* node = nullptr;
                int flag = loadNodeData(item, node);
                if (status == CODE_OK && flag != CODE_OK) {
                    status = flag;
                }
                if (node) {
                    nodes.push_back(node);
                }
            }
        }
        auto dataConnections = data["connections"];
        if (dataConnections.IsDefined() && dataConnections.IsSequence()) {
            for( const auto& item: dataConnections){
                SightNodeConnection c;
                if (item >> c) {
                    c.connectionId = nextNodeOrPortId();
                    connections.push_back(c);
                }
            }
        }

        return status;
    }

    void CopyText::addHeader(YAML::Emitter& out, CopyTextType type) {
        out << YAML::Key << whoAmI << YAML::Value << sightIsCopyText;
        out << YAML::Key << stringType << YAML::Value << copyTextTypeString(type);
    }

    const char* CopyText::copyTextTypeString(CopyTextType type) {
        switch (type) {
        case CopyTextType::None:
            break;
        case CopyTextType::Node:
            return "Node";
        case CopyTextType::Connection:
            return "Connection";
        case CopyTextType::Multiple:
            return "Multiple";
        }
        return "None";
    }

    CopyTextType CopyText::copyTextTypeString(std::string_view str) {
        if (str == "Node") {
            return CopyTextType::Node;
        } else if (str == "Connection") {
            return CopyTextType::Connection;
        } else if (str == "Multiple") {
            return CopyTextType::Multiple;
        }
        return CopyTextType::None;
    }

    std::string CopyText::from(SightNode const& node) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        addHeader(out, CopyTextType::Node);

        out << YAML::Key << stringData << YAML::Value << node;
        out << YAML::EndMap;
        return out.c_str();
    }

    std::string CopyText::from(SightNodeConnection const& connection) {
        return {};
    }

    std::string CopyText::from(std::vector<SightNode*> const& nodes, std::vector<SightNodeConnection> const& connections) {
        YAML::Emitter out;
        out << YAML::BeginMap;
        addHeader(out, CopyTextType::Multiple);

        out << YAML::Key << stringData << YAML::Value << YAML::BeginMap;
        // nodes
        out << YAML::Key << "nodes" << YAML::Value << YAML::BeginSeq;
        for( const auto& item: nodes){
            out << *item;
        }
        out << YAML::EndSeq;

        // connections
        out << YAML::Key << "connections" << YAML::Value << YAML::BeginSeq;
        for( const auto& item: connections){
            out << item;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;
        return out.c_str();
    }

    CopyText CopyText::parse(std::string_view str) {
        try {
            auto nodeRoot = YAML::Load(str.data());
            if (nodeRoot.size() == 0) {
                return {};
            }
            auto n = nodeRoot[whoAmI];
            if (!n.IsDefined() || n.as<std::string>() != sightIsCopyText) {
                return {};
            }

            
            auto type = copyTextTypeString(nodeRoot[stringType].as<std::string>());
            if (type == CopyTextType::None) {
                return {};
            }

            return {
                type,
                nodeRoot[stringData]
            };
        }catch(YAML::ParserException const& e){
            logDebug(e.what());
        }catch(YAML::BadSubscript const& e){
            logDebug(e.what());
        }
        
        return {};
    }

}