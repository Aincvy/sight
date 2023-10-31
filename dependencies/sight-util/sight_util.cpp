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
#    include <mach/mach.h>
#elif _WIN32
#    include <Windows.h>
#    include <psapi.h>
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
#elif _WIN32
        // Memory usage
        PROCESS_MEMORY_COUNTERS_EX memInfo;
        auto process = GetCurrentProcess();
        if (GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&memInfo, sizeof(memInfo))) {
            usageInfo.virtualMemBytes = memInfo.PrivateUsage;
            usageInfo.residentMemBytes = memInfo.WorkingSetSize;
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
    std::string changeStringToPascalCase(std::string_view str) {
        std::stringstream result;
        bool capitalizeNext = true;

        for (char c : str) {
            if (std::isalpha(c)) {
                if (capitalizeNext) {
                    result << static_cast<char>(std::toupper(c));
                    capitalizeNext = false;
                } else {
                    result << c;
                }
            } else {
                capitalizeNext = true;
            }
        }

        return result.str();
    }

    std::string changeStringToCamelCase(std::string_view str) {
        std::string pascalCaseStr = changeStringToPascalCase(str);
        if (!pascalCaseStr.empty()) {
            pascalCaseStr[0] = std::tolower(pascalCaseStr[0]);
        }
        return pascalCaseStr;
    }

    std::string changeStringToSnakeCase(std::string_view str) {
        std::stringstream result;

        for (char c : str) {
            if (std::isalpha(c)) {
                if (std::isupper(c)) {
                    result << '_';
                    result << static_cast<char>(std::tolower(c));
                } else {
                    result << c;
                }
            } else {
                result << '_';
            }
        }

        return result.str();
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
                ss << c;
                // if (normalCount > 0) {
                //     ss << c;
                // }
                // normalCount = 0;
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

    std::string narrow(const std::wstring& str) {
        std::ostringstream stm;

        // Correct code.
        const std::ctype<wchar_t>& ctfacet = use_facet<std::ctype<wchar_t>>(stm.getloc());

        for (size_t i = 0; i < str.size(); ++i)
            stm << ctfacet.narrow(str[i], 0);
        return stm.str();
    }

    std::wstring broaden(std::string_view str) {
        int size = str.size() + 1;
        std::wstring res(size, L'\0');
        mbstowcs(&res[0], str.data(), size);
        return res;
    }

    bool isValidPath(const std::filesystem::path& p) {
        if (std::filesystem::exists(p)) {
            return true;
        }

        std::filesystem::path parent_path = p.parent_path();

        if (std::filesystem::exists(parent_path)) {
            return true;
        }

        return false;
    }


}