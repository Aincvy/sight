#pragma once

#include <string_view>
#include <sys/types.h>
#include <vector>

#include "imgui.h"

#include "sight_node.h"
#include "sight_ui.h"


#include "yaml-cpp/yaml.h"

namespace sight {

    /**
     * @brief When undo is recorded, the reason
     * 
     */
    enum class UndoRecordType {
        Create,
        Delete,
        Update,

        /**
         * @brief 
         * anyThingId = markedNodeId
         * anyThingId2 = nodeId
         * 
         */
        Replace,

        DetachNode,
    };

    struct UndoCommand {

        uint id = 0;
        UndoRecordType recordType;

        /**
         * @brief something's position (node)
         * 
         */
        ImVec2 position;

        /**
         * @brief node, connection, port id 
         * 
         */
        uint anyThingId = 0;

        /**
         * @brief second node connection, port id 
         * 
         */
        uint anyThingId2 = 0;


        SightNodeConnection* connectionData = nullptr;
        SightNode* nodeData = nullptr;
        SightNodeValue portValueData;
        // SightNode* nodeData2 = nullptr;

        UndoCommand() = default;
        UndoCommand(uint id, UndoRecordType recordType);
        UndoCommand(uint id, UndoRecordType recordType, uint anyThingId);
        ~UndoCommand() = default;

        void undo();
        void redo();

        void freeNodeData();

        void init();
        
    };

    enum class StartOrStop {
        Normal,
        Start,
        Stop,
    };

    int getRuntimeId(StartOrStop type = StartOrStop::Normal);

    int initUndo();
    int recordUndo(UndoRecordType recordType, uint anyThingId);
    int recordUndo(UndoRecordType recordType, uint anyThingId, ImVec2 pos);
    // int recordUndo(UndoRecordType recordType, uint anyThingId, ImVec2 pos, SightAnyThingWrapper anyThing);

    /**
     * @brief 
     * 
     * @return UndoCommand*  nullptr if no undo command.
     */
    UndoCommand* lastUndoCommand();


    bool isRedoEnable();
    void redo();

    bool isUndoEnable();
    void undo();

    ///////////////////////////////////////////////////////////////////////////
    //      END OF  UNDO, START COPY_TEXT
    ///////////////////////////////////////////////////////////////////////////

    enum class CopyTextType {
        None,
        Node,
        Connection,
        Multiple,
        Component,

    };

    struct CopyText{
        CopyTextType type = CopyTextType::None;
        YAML::Node data;

        int loadAsNode(SightNode*& node, SightNodeGraph* graph) const;
        int laodAsConnection(SightNodeConnection*& connection);

        /**
         * @brief 
         * 
         * @param nodes load success node, you need free the memory. use `delete` 
         * @return int : first not CODE_OK
         */
        int loadAsMultiple(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections, SightNodeGraph* graph) const;

        static void addHeader(YAML::Emitter& out, CopyTextType type);
        static const char* copyTextTypeString(CopyTextType type);
        static CopyTextType copyTextTypeString(std::string_view str);

        static std::string from(SightNode const& node);
        static std::string fromComponent(SightNode const& node);
        static std::string from(SightNodeConnection const& connection);
        static std::string from(std::vector<SightNode*> const& nodes, std::vector<SightNodeConnection> const& connections);

        static CopyText parse(std::string_view str);

        /**
         * @brief copy node text as component to clip board
         * 
         * @param node 
         */
        static void copyComponent(SightNode const& node);

        // node
        static void copyNode(SightNode const& node);
    };

    ///////////////////////////////////////////////////////////////////////////
    //      END OF COPY_TEXT
    ///////////////////////////////////////////////////////////////////////////
}