#pragma once

/**
 * @file sight_event_bus.h
 * @author Aincvy (aincvy@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-11-06
 * 
 * @copyright Copyright (c) 2023
 * 
 * A very simple event bus.
 */

#include <functional>
#include <random>

#include <absl/container/flat_hash_map.h>

#include "sight_node.h"

namespace sight {

    typedef std::function<void()> EventDisposer;

    /**
     * @brief Very simple event passage, sync.
     * 
     */
    template<typename T>
    class EventPassage {
        typedef std::function<void(T)> callback_t;

        // key random id, value: callback
        absl::flat_hash_map<int, callback_t> map;

        std::random_device rd;
        std::mt19937 gen{rd()};
        std::uniform_int_distribution<> dis{0, 99999999};

    public:
        EventDisposer addListener(callback_t callback) {
            // int eventId = generateEventId();
            int eventId = this->dis(this->gen);
            map[eventId] = callback;
            return [eventId, this]() {
                removeListener(eventId);
            };
        }

        void removeListener(int eventId) {
            map.erase(eventId);
        }

        void dispatch(T node) {
            for (const auto& pair : map) {
                pair.second(node);
            }
        }
    };

    /**
     * @brief NOT thread safe.  Call this one in UI(main) thread.
     * 
     */
    class SimpleEventBus {
        // absl::flat_hash_map<int, EventPassage*>

    public:
        static EventPassage<SightNode*>* nodeAdded();
        static EventPassage<SightNode const&>* nodeRemoved();
        static EventPassage<SightNodeGraph const&>* graphDisposed();

    };
    
}