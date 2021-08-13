//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_js_parser.h"

#include <string.h>
#include <tree_sitter/api.h>


extern "C" {
    // dependencies/tree-sitter-javascript/src/parser.c
    TSLanguage *tree_sitter_javascript(void);
}

TSParser* g_parser = nullptr;

namespace sight {

    const Token GeneratedCode::invalidToken = Token("invalidToken");


    namespace {

        void parseNode(GeneratedCode & generatedCode, TSNode & node){
            if (ts_node_is_null(node)) {
                return;
            }

            dbg(ts_node_type(node));

            std::string typeString = ts_node_type(node);
            if (typeString == "number") {
                auto startPos = ts_node_start_byte(node);
                auto endPos = ts_node_end_byte(node);
                auto t = generatedCode.getToken(startPos, endPos);
                dbg(t.word);
            }

            auto childCount = ts_node_child_count(node);
            for (int i = 0; i < childCount; ++i) {
                TSNode temp = ts_node_named_child(node, i);
                parseNode( generatedCode, temp);
            }
        }

    }

    int testParser() {

        // Create a parser.
        TSParser *parser = ts_parser_new();

        // Set the parser's language (JSON in this case).
        ts_parser_set_language(parser, tree_sitter_javascript());

        // Build a syntax tree based on source code stored in a string.
        const char *source_code = "function abc(a,b) { console.log('123'); let c = a + b; }";
        TSTree *tree = ts_parser_parse_string(
                parser,
                NULL,
                source_code,
                strlen(source_code)
        );

        // Get the root node of the syntax tree.
        TSNode root_node = ts_tree_root_node(tree);
        int count = ts_node_child_count(root_node);
        printf("child count: %d\n", count);

        // Get some child nodes.
        TSNode array_node = ts_node_named_child(root_node, 0);

        printf("number_node_type: %s\n", ts_node_type(array_node));

        // Print the syntax tree as an S-expression.
        char *string = ts_node_string(root_node);
        printf("Syntax tree: %s\n", string);

        // Free all of the heap-allocated memory.
        free(string);
        ts_tree_delete(tree);

        const char *source_code2 = "a = 1 + 1;";
        tree = ts_parser_parse_string(
                parser,
                NULL,
                source_code2,
                strlen(source_code2)

        );

        TSNode root_node2 = ts_tree_root_node(tree);
        count = ts_node_child_count(root_node2);
        printf("child count: %d\n", count);

        // Print the syntax tree as an S-expression.
        string = ts_node_string(root_node2);
        printf("Syntax tree: %s\n", string);

        free(string);
        ts_tree_delete(tree);
        ts_parser_delete(parser);

        return 0;
    }

    int initParser() {
        if (g_parser) {
            return 1;
        }

        g_parser = ts_parser_new();
        // Set the parser's language
        ts_parser_set_language(g_parser, tree_sitter_javascript());
        return 0;
    }

    void freeParser() {
        if (g_parser) {
            ts_parser_delete(g_parser);
            g_parser = nullptr;
        }
    }

    void parseSource(const char *source) {
        TSTree *tree = ts_parser_parse_string(
                g_parser,
                NULL,
                source,
                strlen(source)
        );

        GeneratedCode generatedCode(source);
        TSNode rootNode = ts_tree_root_node(tree);

        parseNode(generatedCode, rootNode);

        ts_tree_delete(tree);
    }

    Token::Token(const char *string) {
        if (!string) {
            return;
        }

        sprintf(this->word, "%s", string);
    }

    Token::Token(const char *source, size_t startPos, size_t endPos) {
        memcpy(this->word, source + startPos, endPos - startPos);
    }


    GeneratedCode::GeneratedCode(const char *source) : source(source) {
        this->sourceLength = strlen(source);
    }

    Token GeneratedCode::getToken(size_t startPos, size_t endPos) {
        if (startPos >= sourceLength || endPos >= sourceLength || endPos - startPos >= NAME_BUF_SIZE) {
            return invalidToken;
        }
        return Token(this->source, startPos, endPos);
    }

    Token GeneratedCode::getToken(void *nodePointer) {
        auto node = (TSNode*) nodePointer;
        return Token(this->source, ts_node_start_byte(*node), ts_node_end_byte(*node));
    }


}