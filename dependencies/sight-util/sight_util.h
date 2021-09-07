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

    /**
     * All trim function are from https://stackoverflow.com/a/217605/11226492
     * @param s
     */


    inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
    }

    inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    inline std::string ltrimCopy(std::string s) {
        ltrim(s);
        return s;
    }

    inline std::string rtrimCopy(std::string s) {
        rtrim(s);
        return s;
    }

    inline std::string trimCopy(std::string s) {
        trim(s);
        return s;
    }

}