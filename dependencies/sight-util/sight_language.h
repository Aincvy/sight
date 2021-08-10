//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/3.
//

#pragma once

/**
 * Used for common keys, e.g. names, short text.
 */
static const unsigned int MOST_KEYS_BUF_SIZE = 50;

namespace sight {


    struct LanguageWindowNames {
        char createEntity[MOST_KEYS_BUF_SIZE] = "CreateEntity";
        char hierarchy[MOST_KEYS_BUF_SIZE] = "Hierarchy";
        char inspector[MOST_KEYS_BUF_SIZE] = "Inspector";
    };

    struct LanguageCommonKeys {
        char className[MOST_KEYS_BUF_SIZE] = "name";
        char address[MOST_KEYS_BUF_SIZE] = "address";
        char preview[MOST_KEYS_BUF_SIZE] = "Preview";
        char ok[MOST_KEYS_BUF_SIZE] = "OK";
        char cancel[MOST_KEYS_BUF_SIZE] = "Cancel";
        char fieldName[MOST_KEYS_BUF_SIZE] = "name";
        char fieldType[MOST_KEYS_BUF_SIZE] = "type";
        char fieldDefaultValue[MOST_KEYS_BUF_SIZE] = "value";

    };

    /**
     * The ui text.
     */
    struct LanguageKeys {
        struct LanguageWindowNames windowNames;
        struct LanguageCommonKeys commonKeys;
    };


    /**
     * Not impl, this will be return a new instance always.
     * @param path
     * @return
     */
    LanguageKeys* loadLanguage(const char* path);

}