#pragma once

#include "sight_event_bus.h"
#include "sight_node.h"
#include "sight_memory.h"

#include <string>
#include <vector>


namespace sight {

    enum class HierarchyItemType {
        None,
        /**
         * @brief do not have id, name will not be changed.
         * 
         */
        Parent = 1,

        Node = 100,
    };
    
    struct HierarchyItem :ResetAble {
        std::string name;
        std::string text;
        uint anyThingId = 0;
        HierarchyItem* parent;
        std::vector<HierarchyItem*> children;
        HierarchyItemType type;

        union {
            SightNode* node;
        } dataPointer;

        HierarchyItem* findItem(const std::string& name);
        HierarchyItem* findItem(uint id);

        void show();
        void showChildren();

        void reset() override;

        void applyTextFromNode();
    };

    struct Hierarchy {
        SightArray<HierarchyItem> items;

        HierarchyItem* root;
        // uint lastDoubleClickedNodeId = 0;
        SightNode* lastDoubleClickedNode = nullptr;


        Hierarchy();
        ~Hierarchy();

        void rebuildCache();

        void addNode(SightNode* node);
        void delNode(SightNode const* node);

        HierarchyItem* findOrCreate(const std::string& name, HierarchyItem* parent);
        HierarchyItem* findOrCreate(uint id, HierarchyItem* parent);

        HierarchyItem* createItem(uint id, const std::string& name, HierarchyItem* parent);

        bool removeItem(HierarchyItem* child, HierarchyItem* parent);

        void initEvents();

        void freeEvents();

        void clearItems();

    private:
        std::vector<EventDisposer> eventHandlerList;
    };


    void showHierarchyWindow();

    void initHierarchy();

    void disposeHierarchy();

    Hierarchy* currentHierarchy();

}
