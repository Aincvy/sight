//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//
#pragma once

#include <future>

#include "sight_defines.h"

namespace sight {

    /**
     *
     */
    struct CommandArgs {
        // string arg.
        const char * argString = nullptr;
        int argStringLength = 0;
        bool needFree = false;

        int argInt = 0;
        void* data = nullptr;
        // if data pointer to an array, this is the length.
        size_t dataLength = 0;

        // for simple thread sync.
        std::promise<int>* promise = nullptr;

        /**
         * dispose command
         */
        void dispose();
    };

    /**
     * exit program.
     * do clean work except ui and node_editor
     */
    void exitSight(int v = 0);

    /**
     * you need free this function return value.
     * @param from
     * @return
     */
    void* copyObject(void * from, size_t size);


}