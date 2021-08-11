//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#pragma once

#include "string"

namespace sight {

    /**
     *
     * @param fullString
     * @param ending
     * @from https://stackoverflow.com/a/874160/11226492
     * @return
     */
    bool endsWith(std::string const& fullString, std::string const& ending);

}