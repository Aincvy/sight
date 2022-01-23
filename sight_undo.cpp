#include "sight_undo.h"

#include "imgui_node_editor.h"
#include "sight.h"
#include "sight_nodes.h"
#include "sight_ui_node_editor.h"

#include <atomic>
#include <cassert>
#include <vector>

#define UNDO_LIST_COUNT 60

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

    int getRuntimeId() {
        static std::atomic_int runtimeId = 1000;
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

        auto & c = redoList.back();
        c.redo();
        redoList.pop_back();
        undoList.push_back(c);
    }

    bool isUndoEnable() {
        return !undoList.empty();
    }

    void undo() {
        if (undoList.empty()) {
            return;
        }

        auto & c = undoList.back();
        c.undo();
        redoList.push_back(c);
        undoList.pop_back();
    }

}