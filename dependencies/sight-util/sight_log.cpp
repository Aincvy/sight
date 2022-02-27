//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/26.
//
#include "stdio.h"

#include "sight_defines.h"
#include "sight_log.h"
#include "absl/strings/substitute.h"

static std::function<void(sight::LogLevel, std::string_view msg)> logWriter = nullptr;

namespace sight {

#ifdef SIGHT_DEBUG
    bool logSourceLocation = true;
#endif

    bool registerLogWriter(std::function<void(LogLevel, std::string_view msg)> func) {
        if (func) {
            if (logWriter) {
                return false;
            }

            logWriter = func;
        } else {
            logWriter = nullptr;
        }

        return true;
    }

    bool unregisterLogWriter() {
        logWriter = nullptr;
        return true;
    }

    void callLogWriter(LogLevel l, const char* msg) {
        if (logWriter) {
            logWriter(l, msg);
        }
    }
    
}
