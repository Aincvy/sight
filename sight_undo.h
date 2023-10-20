#pragma once

#include <string_view>
#include <sys/types.h>
#include <vector>

#include "imgui.h"

#include "sight_nodes.h"
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


        SightNodeConnection connectionData;
        SightNode nodeData;
        SightNodeValue portValueData;

        UndoCommand() = default;
        UndoCommand(uint id, UndoRecordType recordType);
        UndoCommand(uint id, UndoRecordType recordType, uint anyThingId);
        ~UndoCommand() = default;

        void undo();
        void redo();

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
    int recordUndo(UndoRecordType recordType, uint anyThingId, ImVec2 pos, SightAnyThingWrapper anyThing);

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

    };

    struct CopyText{
        CopyTextType type = CopyTextType::None;
        YAML::Node data;

        int loadAsNode(SightNode*& node) const;
        int laodAsConnection(SightNodeConnection*& connection);

        /**
         * @brief 
         * 
         * @param nodes load success node, you need free the memory. use `delete` 
         * @return int : first not CODE_OK
         */
        int loadAsMultiple(std::vector<SightNode*>& nodes, std::vector<SightNodeConnection>& connections) const;

        static void addHeader(YAML::Emitter& out, CopyTextType type);
        static const char* copyTextTypeString(CopyTextType type);
        static CopyTextType copyTextTypeString(std::string_view str);

        static std::string from(SightNode const& node);
        static std::string from(SightNodeConnection const& connection);
        static std::string from(std::vector<SightNode*> const& nodes, std::vector<SightNodeConnection> const& connections);

        static CopyText parse(std::string_view str);
    };

    ///////////////////////////////////////////////////////////////////////////
    //      END OF COPY_TEXT
    ///////////////////////////////////////////////////////////////////////////
}