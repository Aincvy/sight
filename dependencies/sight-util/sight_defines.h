//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/6.
//

#pragma once
#include <string>

// used for name string
#define FILENAME_BUF_SIZE 256
#define NAME_BUF_SIZE 64
#define LITTLE_NAME_BUF_SIZE 32
#define TINY_NAME_BUF_SIZE 16

#define LITTLE_ARRAY_SIZE 128
#define MEDIUM_ARRAY_SIZE 486
#define BIG_ARRAY_SIZE 1024

#define START_NODE_ID 3000

#define TYPE_PROCESS "Process"

#define SET_INT_VALUE(p, v) if (p)  { *(p) = v; }

#define CALL_NODE_FUNC(p, ...)                 \
    nodeFunc((p)->inputPorts, ##__VA_ARGS__);  \
    nodeFunc((p)->outputPorts, ##__VA_ARGS__); \
    nodeFunc((p)->fields, ##__VA_ARGS__)


#define IS_V8_STRING(localVal) (localVal)->IsString() || (localVal)->IsStringObject()
#define IS_V8_NUMBER(localVal) (localVal)->IsNumber() || (localVal)->IsNumberObject()


#define CHECK_CODE(func, i) if (( (i) = (func)) != CODE_OK){ \
    return i;                                                      \
}

#define SET_CODE(code, i) if(code) *code = i;

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