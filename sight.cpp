//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include <stdio.h>
#include <stdlib.h>

#include "sight.h"
#include "sight_js.h"

#include "sight_project.h"

namespace sight {

    const std::string resourceFolder = "../resources/";

    void exitSight(int v) {

        addJsCommand(JsCommandType::Destroy);
        currentProject()->save();
        // exit(v);
    }

    void* copyObject(void* from, size_t size) {
        auto dst = calloc(1, size);
        memcpy(dst, from, size);
        return dst;
    }

    void CommandArgs::dispose() {
        if (needFree) {
            if (argString) {
                free(const_cast<char*>(argString));
                argString = nullptr;
                argStringLength = 0;
            }
            if (data) {
                free(data);
            }
        }
    }
}     // namespace sight