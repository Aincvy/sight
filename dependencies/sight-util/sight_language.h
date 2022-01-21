//
// Created by Aincvy(aincvy@gmail.com) on 2021/8/3.
//

#pragma once

/**
 * Used for common keys, e.g. names, short text.
 */
static const unsigned int MOST_KEYS_BUF_SIZE = 32;
static const unsigned int SHORT_KEYS_BUF_SIZE = 16;


namespace sight {


    struct LanguageWindowNames {
        char createEntity[MOST_KEYS_BUF_SIZE] = "CreateEntity";
        char hierarchy[MOST_KEYS_BUF_SIZE] = "Hierarchy";
        char inspector[MOST_KEYS_BUF_SIZE] = "Inspector";
        char project[SHORT_KEYS_BUF_SIZE] = "Project";
        char about[SHORT_KEYS_BUF_SIZE] = "About";
        char projectSettings[MOST_KEYS_BUF_SIZE] = "ProjectSettings";
    };

    struct LanguageCommonKeys {
        char className[SHORT_KEYS_BUF_SIZE] = "name";
        char fullName[SHORT_KEYS_BUF_SIZE] = "fullName";
        char address[SHORT_KEYS_BUF_SIZE] = "address";
        char preview[SHORT_KEYS_BUF_SIZE] = "Preview";
        char ok[SHORT_KEYS_BUF_SIZE] = "OK";
        char cancel[SHORT_KEYS_BUF_SIZE] = "Cancel";
        char drop[SHORT_KEYS_BUF_SIZE] = "Drop";
        char fieldName[SHORT_KEYS_BUF_SIZE] = "name";
        char fieldType[SHORT_KEYS_BUF_SIZE] = "type";
        char fieldDefaultValue[SHORT_KEYS_BUF_SIZE] = "value";
        char readonly[SHORT_KEYS_BUF_SIZE] = "readonly";
    };

    struct LanguageMenuKeys {
        // char file[SHORT_KEYS_BUF_SIZE] = "File";
        char edit[SHORT_KEYS_BUF_SIZE] = "Edit";
        // char view[SHORT_KEYS_BUF_SIZE] = "View";
        char project[SHORT_KEYS_BUF_SIZE] = "Project";
        char custom[SHORT_KEYS_BUF_SIZE] = "Custom";
        char help[SHORT_KEYS_BUF_SIZE] = "Help";

        char _new[SHORT_KEYS_BUF_SIZE] = "New";
        char file[SHORT_KEYS_BUF_SIZE] = "File";
        char graph[SHORT_KEYS_BUF_SIZE] = "Graph";
        char graphByPath[SHORT_KEYS_BUF_SIZE] = "GraphByPath";
        char entity[SHORT_KEYS_BUF_SIZE] = "Entity";
        char open[SHORT_KEYS_BUF_SIZE] = "Open";
        char save[SHORT_KEYS_BUF_SIZE] = "Save";
        char openProject[SHORT_KEYS_BUF_SIZE] = "Open Project...";
        char saveAll[SHORT_KEYS_BUF_SIZE] = "Save All";
        char options[SHORT_KEYS_BUF_SIZE] = "Options";
        char exit[SHORT_KEYS_BUF_SIZE] = "Exit";

        char undo[SHORT_KEYS_BUF_SIZE] = "Undo";
        char redo[SHORT_KEYS_BUF_SIZE] = "Redo";

        char view[SHORT_KEYS_BUF_SIZE] = "View";
        char layout[SHORT_KEYS_BUF_SIZE] = "Layout";
        char reset[SHORT_KEYS_BUF_SIZE] = "Reset";
        char windows[SHORT_KEYS_BUF_SIZE] = "Windows";

        char build[SHORT_KEYS_BUF_SIZE] = "Build";
        char rebuild[SHORT_KEYS_BUF_SIZE] = "Rebuild";
        char clean[SHORT_KEYS_BUF_SIZE] = "Clean";
        char parseGraph[SHORT_KEYS_BUF_SIZE] = "Parse Graph";
        char projectSaveConfig[SHORT_KEYS_BUF_SIZE] = "Save Config";
        char reload[SHORT_KEYS_BUF_SIZE] = "Reload";
        char settings[SHORT_KEYS_BUF_SIZE] = "Settings";
        
    };

        /**
     * The ui text.
     */
        struct LanguageKeys {
        struct LanguageWindowNames windowNames;
        struct LanguageCommonKeys commonKeys;
        struct LanguageMenuKeys menuKeys;
    };


    /**
     * Not impl, this will be return a new instance always.
     * @param path
     * @return
     */
    LanguageKeys* loadLanguage(const char* path);

}