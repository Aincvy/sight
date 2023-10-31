//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/10.
//

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>


namespace sight {

    struct ProcessUsageInfo{
        size_t virtualMemBytes = 0;
        size_t residentMemBytes = 0;
    };

    enum class CaseTypes {
        None,
        PascalCase,     // 大驼峰命名法   MyVariable、MyClass。
        CamelCase,      // 小驼峰命名法  myVariable、myMethod。
        SnakeCase,      //  下划线命名法 my_variable、my_method。
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

    inline std::string& appendIfNotExists(std::string& str, std::string const& end) {
        if (!endsWith(str, end)) {
            str += end;
        }
        return str;
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

    std::string changeStringToPascalCase(std::string_view str);
    std::string changeStringToCamelCase(std::string_view str);
    std::string changeStringToSnakeCase(std::string_view str);

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

    /**
     * @brief 
     * 
     * @param str 
     * @return std::string 
     */
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

    inline void assign(char* buf, std::string_view str, size_t len = -1) {
        if (len > 0) {
            snprintf(buf, len, "%s", str.data());
        } else {
            sprintf(buf, "%s", str.data());
        }
    }

    template <class T>
    inline void addIfNotExist(std::vector<T> & list, T const& s){
        auto iter = std::find(list.begin(), list.end(), s);
        if (iter == list.end()) {
           list.push_back(s); 
        }
    }

    
    inline std::string changeStringToCase(std::string_view str, CaseTypes caseType) {
        switch (caseType) {
        case CaseTypes::None:
            return std::string(str);
        case CaseTypes::PascalCase:
            return changeStringToPascalCase(str);
        case CaseTypes::CamelCase:
            return changeStringToCamelCase(str);
        case CaseTypes::SnakeCase:
           return changeStringToSnakeCase(str);
        }
        return {};
    }

    
    template<class T>
    inline bool contains(std::vector<T> vector, T e) {
        return std::find(vector.begin(), vector.end(), e) != vector.end();
    }
    
    /**
     * @brief 
     * 
     * @param path 
     * @return true if file is hidden
     * @return false 
     */
    bool isFileHidden(const char* path);

    /**
     * @brief Get the Last After separator
     * If the source do not have the separator, then return the source.
     * @param source 
     * @param separator 
     * @return std::string 
     */
    std::string getLastAfter(std::string_view source, std::string_view separator);

    /**
     * @brief 
     * 
     * @param str 
     * @return std::string 
     */
    std::string removeExcessSpaces(std::string_view str, int indentSpaceCount = 2);

#ifdef _WIN32
    std::string narrow(const std::wstring& str);

    std::wstring broaden(std::string_view str);
#endif


    bool isValidPath(const std::filesystem::path& p);
    
}