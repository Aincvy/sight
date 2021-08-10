//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/6.
//

#pragma once

#include "sight_defines.h"

#include <string>

namespace sight {

    struct Address {
        char part[LITTLE_NAME_BUF_SIZE] = {0};

        // next part
        struct Address* next = nullptr;



        /**
         * convert part string to int
         * @return If success, the value. Otherwise fallback.
         */
        int asInt(int fallback = 0) const;

        /**
         * convert part string to float
         * @return
         */
        float asFloat(float fallback = 0) const;

        /**
         * convert part string to double
         * @return
         */
        double asDouble(double fallback = 0) const;

        /**
         * convert part string to bool
         * @return
         */
        bool asBool(bool fallback = false) const;

        /**
         * the last item
         * @return
         */
        Address* last() ;

        int length();

    };

    /**
     * It will be free all parts.
     * DO NOT CALL this function on a local var.
     * @param first
     */
    void freeAddress(struct Address*& first);

    /**
     * After use address, you need free it.
     * @param fullAddress
     * @return the first part.
     */
    Address* resolveAddress(const std::string &fullAddress);

}