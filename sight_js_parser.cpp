//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_js_parser.h"
#include "sight.h"
#include "sight_defines.h"
#include "sight_log.h"

#include <cstring>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string.h>
#include <string>
#include <string_view>
#include <map>

#include <tree_sitter/api.h>

#include <utility>

#include "absl/strings/numbers.h"
#include "absl/container/flat_hash_map.h"

#include "functional"

#ifdef _WIN32
#    define TO_CONST_CHAT_POINTER(x) (x)._Unwrapped()
#else
#    define TO_CONST_CHAT_POINTER(x) (x)
#endif

extern "C" {
    // dependencies/tree-sitter-javascript/src/parser.c
    TSLanguage *tree_sitter_javascript(void);
}

TSParser* g_parser = nullptr;

namespace sight {

    namespace {

        struct GeneratedCode{
            bool fail = false;
            std::string failReason{};
            std::string_view source;
            // which language do you want to generate ? 
            DefLanguage const& targetLang;
        };

        struct TreeSitterFields {
            TSFieldId _operator;
            TSFieldId left;
            TSFieldId right;
            TSFieldId argument;
            TSFieldId body;
            TSFieldId kind;
        };

        TreeSitterFields jsLangFields;

        /**
         * Used for tree-sitter node parse.
         */
        using mapType = absl::flat_hash_map<std::string, std::function<void(GeneratedCode&, std::ostream&, TSNode)>>;
        // mapType nodeHandlerMap;
        // 
        std::map<DefLanguage, mapType> langNodeHandlerMap;

        void initTreeSitterFields(TSLanguage* language){

            constexpr const char* fieldOperator = "operator";
            constexpr const char* fieldLeft = "left";
            constexpr const char* fieldRight = "right";
            constexpr const char* fieldArgument = "argument";
            constexpr const char* fieldBody = "body";
            constexpr const char* fieldKind = "kind";

            jsLangFields._operator = ts_language_field_id_for_name(language, fieldOperator, strlen(fieldOperator));
            jsLangFields.left = ts_language_field_id_for_name(language, fieldLeft, strlen(fieldLeft));
            jsLangFields.right = ts_language_field_id_for_name(language, fieldRight, strlen(fieldRight));
            jsLangFields.argument = ts_language_field_id_for_name(language, fieldArgument, strlen(fieldArgument));
            jsLangFields.body = ts_language_field_id_for_name(language, fieldBody, strlen(fieldBody));
            jsLangFields.kind = ts_language_field_id_for_name(language, fieldKind, strlen(fieldKind));
        }



        void generate(GeneratedCode& code, std::ostream& out, TSNode node);


        void plainAction(GeneratedCode& code, std::ostream& out, TSNode tsNode) {
            auto source = code.source;
            auto startPos = ts_node_start_byte(tsNode);
            auto endPos = ts_node_end_byte(tsNode);
            for (auto i = startPos; i < endPos; i++) {
                out << source[i];
            }
            out << " ";   // insert 1 space.

            // out << std::endl;
            // logDebug("s: $0, e: $1, ",startPos, endPos);
        };

        void defaultAction(GeneratedCode& code, std::ostream& out, TSNode tsNode) {
            uint count = 0;
            auto source = code.source;
            if ((count = ts_node_child_count(tsNode)) <= 0) {
                // terminal node
                plainAction(code, out, tsNode);
            } else {
                // loop item of children
                for (int i = 0; i < count; i++) {
                    auto n = ts_node_child(tsNode, i);
                    generate(code, out, n);
                }
            }
        }

        void generate(GeneratedCode& code, std::ostream& out, TSNode node) {
            if (ts_node_is_null(node)) {
                return;
            }

            auto& nodeHandlerMap = langNodeHandlerMap[code.targetLang];
            auto nodeType = ts_node_type(node);
            auto iter = nodeHandlerMap.find(nodeType);
            if (iter != nodeHandlerMap.end()) {
                iter->second(code, out, node);
            } else {
                // try find *
                iter = nodeHandlerMap.find("*");
                if (iter != nodeHandlerMap.end()) {
                    iter->second(code, out, node);
                } else {
                    defaultAction(code, out, node);
                }
            }

            return;
        }

        /**
         * 
         * @param generatedCode
         * @param node
         */
        void showNodeHierarchy(TSNode & node){
            if (ts_node_is_null(node)) {
                return;
            }

            char *string = ts_node_string(node);
            logDebug(string);
            free(string);
        }

        void initCShaprHandlerMap(){
            DefLanguage lang;
            lang.setType(DefLanguageType::CSharp);
            lang.version = 0;

            auto& nodeHandlerMap = langNodeHandlerMap[lang];

            nodeHandlerMap["identifier"] = plainAction;

            nodeHandlerMap["for_in_statement"] = [](GeneratedCode& code, std::ostream& out, TSNode tsNode) {
                // logDebug(" for_in_statement");

                auto source = code.source;
                out << "foreach( var ";
                auto left = ts_node_child_by_field_id(tsNode, jsLangFields.left);
                generate(code, out, left);
                out << " in ";
                auto right = ts_node_child_by_field_id(tsNode, jsLangFields.right);
                generate(code, out, right);
                out << "){ \n";
                auto body = ts_node_child_by_field_id(tsNode, jsLangFields.body);
                generate(code, out, body);
                out << "}\n";
            };

            // variable_declaration
            nodeHandlerMap["lexical_declaration"] = [](GeneratedCode& code, std::ostream& out, TSNode tsNode) {
                // logDebug("lexical_declaration");
                
                auto kind = ts_node_child_by_field_id(tsNode, jsLangFields.kind);
                bool isConst = false;
                constexpr const auto strConst = "const";
                if (strncmp(TO_CONST_CHAT_POINTER(code.source.begin() + ts_node_start_byte(kind)), strConst, strlen(strConst)) == 0) {
                    // const
                    isConst = true;
                }

                // append source.
                if (isConst) {
                    out << "const ";
                }
                out << "var ";

                // next node
                generate(code, out, ts_node_next_named_sibling(kind));
                out << ";\n";
            };

        }

        void initJavaScriptHandlerMap(){
            DefLanguage lang;
            lang.setType(DefLanguageType::JavaScript);
            lang.version = 0;

            auto& nodeHandlerMap = langNodeHandlerMap[lang];
            nodeHandlerMap["*"] = defaultAction;

        }

        void initCaseMap(){
            // field name from: https://github.com/Aincvy/tree-sitter-javascript/blob/master/grammar.js


            initCShaprHandlerMap();
            initJavaScriptHandlerMap();
        }
        
    }

    int initParser() {
        logDebug("start initParser");
        if (g_parser) {
            return CODE_FAIL;
        }

        g_parser = ts_parser_new();
        // Set the parser's language
        auto language = tree_sitter_javascript();
        ts_parser_set_language(g_parser, language);
        initTreeSitterFields(language);

        initCaseMap();
        logDebug("end initParser");
        return CODE_OK;
    }

    void freeParser() {
        if (g_parser) {
            ts_parser_delete(g_parser);
            g_parser = nullptr;
        }
    }

    std::string parseSource(std::string_view source, DefLanguage const& targetLang, int* status) {
        logDebug(source);
        if (langNodeHandlerMap.find(targetLang) == langNodeHandlerMap.end()) {
            logError("Cannot find type: $0, version: $1, type-name: $2", targetLang.type, targetLang.version, targetLang.getTypeName());
            return {};
        }

        TSTree *tree = ts_parser_parse_string(
                g_parser,
                NULL,
                source.data(),
                source.length()
        );

        TSNode rootNode = ts_tree_root_node(tree);
        // showNodeHierarchy(rootNode);

        GeneratedCode code{
            .fail = false,
            .source = source,
            .targetLang = targetLang,
        };
        
        std::stringstream ss;
        generate(code, ss, rootNode);
        ts_tree_delete(tree);

        if (code.fail) {
            // generate failed
            logError("parse source error: $0", code.failReason);
            SET_CODE(status, CODE_FAIL);
            return {};
        }

        SET_CODE(status, CODE_OK);
        return ss.str();
    }

    DefLanguage::DefLanguage(int type, int version)
        : type(type), version(version)
    {
        
    }

    DefLanguageType DefLanguage::getType() const {
        return static_cast<DefLanguageType>(type);
    }

    void DefLanguage::setType(DefLanguageType type) {
        this->type = static_cast<int>(type);
    }

    const char* DefLanguage::getTypeName() const {
        if (type >= 0 && type < std::size(DefLanguageTypeNames)) {
            return DefLanguageTypeNames[type];
        }
        return nullptr;
    }

    bool DefLanguage::isCompatible(DefLanguage const& rhs) const {
        return this->type == rhs.type && this->version >= rhs.version;
    }

    bool DefLanguage::operator==(DefLanguage const& rhs) const {
        if (this->type != rhs.type) {
            return false;
        }

        // let 0 = any version
        if (this->version == 0 || rhs.version == 0) {
            return true;
        }

        return this->version == rhs.version;
    }

    bool DefLanguage::operator<(DefLanguage const& rhs) const {
        if (*this == rhs) {
            return false;
        }

        if (this->type < rhs.type) {
            return true;
        } else if (this->type == rhs.type) {
            return this->version < rhs.version;
        } else {
            return false;
        }
    }

}