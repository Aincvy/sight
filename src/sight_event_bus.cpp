#include "sight_event_bus.h"
#include "sight_node.h"

namespace sight {

    EventPassage<SightNode*>* SimpleEventBus::nodeAdded() {

        static EventPassage<SightNode*> eventPassage;
        return &eventPassage;
    }

    EventPassage<SightNode const&>* SimpleEventBus::nodeRemoved() {

        static EventPassage<SightNode const&> eventPassage;
        return &eventPassage;
    }

    EventPassage<SightNodeGraph const&>* SimpleEventBus::graphDisposed() {
    
        static EventPassage<SightNodeGraph const&> eventPassage;
        return &eventPassage;    
    }

    EventPassage<SightNodeGraph*>* SimpleEventBus::graphOpened() {
        static EventPassage<SightNodeGraph*> eventPassage;
        return &eventPassage;    
    }


}