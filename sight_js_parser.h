//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#pragma once

#include "sight_defines.h"
#include "string"
#include "vector"
#include "sight_memory.h"
#include "any"

namespace sight {

    enum class DefLanguageType {
        JavaScript,
        Java,
        CSharp,
        Cpp,
        C,
    };

    constexpr const char* DefLanguageTypeNames[] = { "JavaScript", "Java", "CSharp", "Cpp", "C" };

    struct DefLanguage {
        int type = 0;           // JavaScript
        int version = 0;        // 0 = any version.


        DefLanguage() = default;
        DefLanguage(int type, int version);

        DefLanguageType getType() const;
        void setType(DefLanguageType type);
        const char* getTypeName() const;

        /**
         * @brief Does this compatible with rhs ? 
         * 
         * @param rhs 
         * @return true 
         * @return false 
         */
        bool isCompatible(DefLanguage const& rhs) const;

        bool operator==(DefLanguage const& rhs) const;

        bool operator<(DefLanguage const& rhs) const;
    };

    int initParser();

    void freeParser();

    /**
     *
     * @param source
     */
    std::string parseSource(std::string_view source, DefLanguage const& targetLang = {0,0}, int* status = nullptr);

}