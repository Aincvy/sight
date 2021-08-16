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

#define LITTLE_ARRAY_SIZE 128
#define MEDIUM_ARRAY_SIZE 486
#define BIG_ARRAY_SIZE 1024

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