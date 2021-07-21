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

namespace sight {

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
        TSNode number_node = ts_node_named_child(array_node, 0);

        printf("number_node_type: %s\n", ts_node_type(array_node));

        // Print the syntax tree as an S-expression.
        char *string = ts_node_string(root_node);
        printf("Syntax tree: %s\n", string);

        // Free all of the heap-allocated memory.
        free(string);
        ts_tree_delete(tree);
        ts_parser_delete(parser);

        return 0;
    }

}