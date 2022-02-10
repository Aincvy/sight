//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#include "sight_util.h"

#include <algorithm>
#include "sstream"

#include <cstring>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/stat.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

namespace sight {

    std::string emptyString("");


    ProcessUsageInfo currentProcessUsageInfo() {
        ProcessUsageInfo usageInfo;
#ifdef __APPLE__
        // from https://stackoverflow.com/a/1911863/11226492
        struct task_basic_info t_info;
        mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

        if (KERN_SUCCESS == task_info(mach_task_self(),
                                      TASK_BASIC_INFO, (task_info_t)&t_info,
                                      &t_info_count)) {
            usageInfo.virtualMemBytes = t_info.virtual_size;
            usageInfo.residentMemBytes = t_info.resident_size;
        }
#endif
        return usageInfo;
    }

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

    std::string getLastAfter(std::string_view source, std::string_view separator) {
        if (separator.empty()) {
            return std::string{source};
        }
        auto pos = source.rfind(separator);
        if (pos == std::string_view::npos) {
            return std::string{ source };
        }

        return std::string{ source.substr(pos + separator.length()) };
    }

    std::string removeExcessSpaces(std::string_view str, int indentSpaceCount) {
        std::stringstream ss;
        
        int normalCount = 0;
        int indentLevel = 0;
        for (int i = 0; i < str.length(); i++) {
            char c = str[i];
            if (c == '\n') {
                // line  
                if (normalCount > 0) {
                    ss << c;
                } 
                normalCount = 0;
            } else if (isspace(c)) {
                // ss << c;
            }
            else {
                if (c == '}' && indentLevel > 0) {
                    indentLevel--;
                }

                if (normalCount <= 0) {
                    // new line 
                    ss << std::string(indentLevel * indentSpaceCount, ' ');
                }
                normalCount++;
                ss << c;

                if (c == '{') {
                    indentLevel++;
                }
            }
        }
        return ss.str();
    }

}