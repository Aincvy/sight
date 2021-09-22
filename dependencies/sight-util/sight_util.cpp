//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#include "sight_util.h"

#include <algorithm>
#include "sstream"

#include <cstring>
#include <sys/stat.h>

namespace sight {

    std::string emptyString("");


    bool endsWith(const std::string &fullString, const std::string &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    std::string removeComment(const std::string &code) {
        std::stringstream ss;

        bool lineComment = false, blockComment = false, slash = false, star = false;
        int i = -1;
        for (auto c : code){
            // check if we are starting comment
            if (c == '/') {
                if (slash) {
                    lineComment = true;
                    i = -1;
                }
            } else if (c == '*') {
                if (slash) {
                    blockComment = true;
                    i = -1;
                }
            } else if (c == '\n') {
                lineComment = false;
            }

            if (!lineComment && !blockComment) {
                // first  '/'
                if (c == '/') {
                    i = static_cast<int>(c);
                } else {
                    if (i > 0) {
                        ss << static_cast<char>(i);
                        i = -1;
                    }
                    ss << c;
                }
            }

            // check the comment is over
            if (c == '/' && star) {
                blockComment = false;
            }

            slash = star = false;
            if (c == '/') {
                slash = true;
            } else if (c == '*') {
                star = true;
            }
        }

        return ss.str();
    }

    bool isFileHidden(const char* path) {
        // I don't know how to impl this ... 
        if (std::strcmp(path, ".DS_Store") == 0) {
            return true;
        }
        
        return false;
    }

}