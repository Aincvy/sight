//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#include "sight_util.h"

namespace sight {

    bool endsWith(const std::string &fullString, const std::string &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

}