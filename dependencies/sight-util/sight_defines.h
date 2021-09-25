//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/6.
//

#pragma once

#ifdef SIGHT_RELEASE
#define dbg(...)
#else
#define DBG_MACRO_NO_WARNING 1
#include "dbg.h"
#endif

// used for name string
#define NAME_BUF_SIZE 65
#define LITTLE_NAME_BUF_SIZE 33

#define LITTLE_ARRAY_SIZE 128
#define MEDIUM_ARRAY_SIZE 486
#define BIG_ARRAY_SIZE 1024

#define TYPE_PROCESS "Process"

#define SET_INT_VALUE(p, v) if (p)  { *(p) = v; }

#define CALL_NODE_FUNC(p) nodeFunc((p)->inputPorts); \
        nodeFunc((p)->outputPorts);                  \
        nodeFunc((p)->fields)


#define IS_V8_STRING(localVal) (localVal)->IsString() || (localVal)->IsStringObject()


#define CHECK_CODE(func, i) if (( (i) = (func)) != CODE_OK){ \
    return i;                                                      \
}




namespace sight {

    extern std::string emptyString;


    /**
     *
     */
    extern const char* addressPrefix;

    /**
     *
     */
    extern const int addressPrefixLen;

}