//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/6.
//

#pragma once

#ifdef SIGHT_RELEASE
#define dbg(...)
#else
#include "dbg.h"
#endif

// used for name string
#define NAME_BUF_SIZE 256

#define LITTLE_NAME_BUF_SIZE 64

namespace sight {

    /**
     *
     */
    extern const char* addressPrefix;

    /**
     *
     */
    extern const int addressPrefixLen;


}