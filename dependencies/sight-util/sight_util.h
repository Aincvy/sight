//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#pragma once

#include "string"
#include <algorithm>

namespace sight {

    struct ProcessUsageInfo{
        size_t virtualMemBytes = 0;
        size_t residentMemBytes = 0;
    };

    ProcessUsageInfo currentProcessUsageInfo();

    /**
     *
     * @param fullString
     * @param ending
     * @from https://stackoverflow.com/a/874160/11226492
     * @return
     */
    bool endsWith(std::string const& fullString, std::string const& ending);

    /**
     *
     * @param fullString
     * @param substring
     * @return
     */
    inline bool startsWith(std::string const&fullString, std::string const& substring){
        return fullString.size() >= substring.size() && fullString.find(substring) == 0;
    }

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

    /**
     *
     * @param code
     * @return
     */
    std::string removeComment(std::string const& code);

    /**
     * https://stackoverflow.com/a/313990/11226492
     * @param data
     */
    inline void lowerCase(std::string & data){
        std::transform(data.begin(), data.end(), data.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    inline void upperCase(std::string & data){
        std::transform(data.begin(), data.end(), data.begin(),
                       [](unsigned char c){ return std::toupper(c); });
    }

    /**
     * @brief 
     * 
     * @param path 
     * @return true if file is hidden
     * @return false 
     */
    bool isFileHidden(const char* path);

    inline std::string strJoin(const char* s1, const char* s2, const char* joiner = "##") {
        std::string str(s1);
        str += joiner;
        str += s2;
        return str;
    }

    inline std::string strJoin(std::string s1, std::string const& s2, std::string joiner = "##"){
        s1 += joiner;
        s1 += s2;
        return s1;
    }
    
    inline std::string removeExt(std::string const& str){
        return std::string(str, 0, str.rfind('.'));
    }

    inline std::size_t countLineInString(std::string const& str){
        auto pos = str.find_last_of('\n');
        if (pos == std::string::npos) {
            return str.empty() ? 0 : 1;   
        }
        auto size = std::count_if(str.begin(), str.end(), [](char c) { return c == '\n'; });
        return size + (pos == str.size() - 1 ? 0 : 1);
    }
    
}