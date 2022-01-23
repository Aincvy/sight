#pragma once

#include <sys/types.h>

#include "imgui.h"

#include "sight_nodes.h"

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

    int getRuntimeId();

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


}