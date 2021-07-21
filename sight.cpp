//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include <stdio.h>
#include <stdlib.h>

#include "sight.h"
#include "sight_js.h"

namespace sight {

    void exitSight(int v) {

        destroyJsEngine();
        exit(v);
    }

}