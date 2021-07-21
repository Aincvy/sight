//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_project.h"

namespace sight {

    int Project::build() {
        return 0;
    }

    int Project::clean() {
        return 0;
    }

    int Project::rebuild() {
        clean();
        return build();
    }
}