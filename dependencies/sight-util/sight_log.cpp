//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/26.
//
#include "stdio.h"

#include "sight_log.h"

namespace sight {

    int logDebug(const char *msg) {
        printf("%s", msg);

        return 0;
    }
}
