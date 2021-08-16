//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#include "sight_js_parser.h"

#include <string.h>
#include <tree_sitter/api.h>

#include <utility>

#include "absl/strings/numbers.h"
#include "absl/container/flat_hash_map.h"

#include "functional"

extern "C" {
    // dependencies/tree-sitter-javascript/src/parser.c
    TSLanguage *tree_sitter_javascript(void);
}

TSParser* g_parser = nullptr;

namespace sight {

    const Token GeneratedCode::invalidToken = Token("[](invalid%token)");

    namespace {

        /**
         * Used for tree-sitter node parse.
         */
        absl::flat_hash_map<std::string,std::function<GeneratedElement*(GeneratedCode &,TSNode &)>> nodeHandlerMap;

        GeneratedElement* generate(GeneratedCode & generatedCode, TSNode & node){
            auto nodeType = ts_node_type(node);
            dbg(nodeType);
            auto iter = nodeHandlerMap.find(nodeType);
            if (iter != nodeHandlerMap.end()) {
                return iter->second(generatedCode, node);
            }

            return nullptr;
        }

        void parseNode(GeneratedCode & generatedCode, TSNode & node){
            if (ts_node_is_null(node)) {
                return;
            }


            generate(generatedCode, node);

            auto childCount = ts_node_child_count(node);
            for (int i = 0; i < childCount; ++i) {
                TSNode temp = ts_node_named_child(node, i);
                parseNode( generatedCode, temp);
            }
        }

        void initCaseMap(){
            nodeHandlerMap["number"] = [](GeneratedCode &generatedCode,TSNode &node){
                auto token = generatedCode.getToken(&node);
                if (token == GeneratedCode::invalidToken) {
                    // may need to do something.
                }

                return generatedCode.addElement(IntLiteral(token.asInt()));
            };

            nodeHandlerMap["identifier"] = [](GeneratedCode &generatedCode,TSNode &node) {
                std::any a = node;
                return generatedCode.addElement(generatedCode.getToken(a));
            };

            nodeHandlerMap["binary_expression"] = [](GeneratedCode &generatedCode,TSNode &node){
                dbg("in binary_expression callback");
//                auto token = generatedCode.getToken(&node);
//                if (token == GeneratedCode::invalidToken) {
//                     // may need to do something.
//                }

                dbg(ts_node_child_count(node));
                // 3 children.
                std::any a = ts_node_named_child(node, 0);
                std::any b = ts_node_named_child(node, 1);
                std::any c = ts_node_named_child(node, 2);
                auto left = generatedCode.getToken(a);
                auto symbol = generatedCode.getToken(b);
                auto right = generatedCode.getToken(c);

                return generatedCode.addElement(BinaryExpr(left, symbol, right));
            };

            nodeHandlerMap["assignment_expression"] = [](GeneratedCode &generatedCode,TSNode &node){
                dbg(ts_node_child_count(node));
                auto a = ts_node_named_child(node, 0);
                auto b = ts_node_named_child(node, 1);

                auto left = generate(generatedCode, a);
                auto right = generate(generatedCode, b);
                return generatedCode.addElement(AssignValueStmt(left, right));
            };

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

        initCaseMap();
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

    int Token::asInt(int fallback) const {
        int i = 0;
        if (absl::SimpleAtoi(this->word, &i)) {
            return i;
        }

        return fallback;
    }

    float Token::asFloat(float fallback) const {
        float f = 0;
        if (absl::SimpleAtof(this->word, &f)) {
            return f;
        }
        return fallback;
    }

    double Token::asDouble(double fallback) const {
        double d = 0;
        if (absl::SimpleAtod(this->word, &d)) {
            return d;
        }
        return fallback;
    }

    bool Token::asBool(bool fallback) const {
        bool b = false;
        if (absl::SimpleAtob(this->word, &b)) {
            return b;
        }
        return fallback;
    }

    bool Token::operator==(const Token &rhs) const {
        return strcmp(this->word, rhs.word) == 0;
    }

    GeneratedElementType Token::getElementType() const{
        return GeneratedElementType::Token;
    }

    GeneratedCode::GeneratedCode(const char *source) : source(source) {
        this->sourceLength = strlen(source);
    }

    Token GeneratedCode::getToken(size_t startPos, size_t endPos) const{
        if (startPos >= sourceLength || endPos >= sourceLength || endPos - startPos >= NAME_BUF_SIZE) {
            return invalidToken;
        }
        return Token(this->source, startPos, endPos);
    }

    Token GeneratedCode::getToken(void *nodePointer) const{
        auto node = (TSNode*) nodePointer;
        return Token(this->source, ts_node_start_byte(*node), ts_node_end_byte(*node));
    }

    std::string GeneratedCode::getString(size_t startPos, size_t endPos) const {
        return std::string(this->source, startPos, endPos - startPos);
    }

    void GeneratedCode::addFunction(const char *name, const char *returnType) {
        auto p = functionArray.add();
        p->name = name;
        p->returnType = returnType;
        this->currentFunction = p;
    }

    GeneratedElement *GeneratedCode::addElement(const GeneratedElement &element) {
        GeneratedElement* result = nullptr;
        switch (element.getElementType()) {
            case GeneratedElementType::Token:
            {
                auto const & b = (Token const &)element;
                result = tokenArray.add(b);
                break;
            }
            case GeneratedElementType::IntLiteral:
            {
                auto const & b = (IntLiteral const &)element;
                result = intLiteralArray.add(b);
                break;
            }
            case GeneratedElementType::StringLiteral:
                break;
            case GeneratedElementType::BoolLiteral:
                break;
            case GeneratedElementType::CharLiteral:
                break;
            case GeneratedElementType::GeneratedBlock:
                break;
            case GeneratedElementType::FieldOrParameter:
                break;
            case GeneratedElementType::FunctionDeclaration:
            {
                auto const & f = (FunctionDeclaration const &)element;
                result = functionArray.add(f);
                break;
            }
            case GeneratedElementType::BinaryExpr:
            {
                auto const & b = (BinaryExpr const &)element;
                result = binaryExprArray.add(b);
                break;
            }
        }

        return result;
    }

    Token GeneratedCode::getToken(std::any &tsNode) const {
        auto node = std::any_cast<TSNode&>(tsNode);
        return Token(this->source, ts_node_start_byte(node), ts_node_end_byte(node));
    }

    IntLiteral::IntLiteral(int value) : Literal(value) {

    }

    GeneratedElementType IntLiteral::getElementType() const {
        return GeneratedElementType::IntLiteral;
    }

    IntLiteral::~IntLiteral() {

    }

    BoolLiteral::BoolLiteral(bool value) : Literal(value) {

    }

    StringLiteral::StringLiteral(std::string value) : Literal(std::move(value)) {

    }

    CharLiteral::CharLiteral(char value) : Literal(value) {

    }


    FunctionDeclaration::FunctionDeclaration(const char*name, const char*returnType)
        : name(name), returnType(returnType) {

    }

    GeneratedElementType FunctionDeclaration::getElementType() const{
        return GeneratedElementType::FunctionDeclaration;
    }

    GeneratedElement::GeneratedElement(int fromNode) : fromNode(fromNode) {

    }

    void GeneratedElement::reset() {
        this->fromNode = 0;
    }

    GeneratedElementType GeneratedBlock::getElementType() const{
        return GeneratedElementType::GeneratedBlock;
    }

    GeneratedElementType FieldOrParameter::getElementType() const {
        return GeneratedElementType::FieldOrParameter;
    }

    GeneratedElementType BinaryExpr::getElementType() const {
        return GeneratedElementType::BinaryExpr;
    }

    BinaryExpr::BinaryExpr(Token left, Token symbol, Token right) : left(std::move(left)), symbol(std::move(symbol)),
                                                                                         right(std::move(right)) {}

    bool GeneratedCodeVisitor::visit(GeneratedElement *element) {
        if (element == nullptr) {
            return false;
        }

        dbg(element->getElementType());
        return true;
    }

    GeneratedCodeVisitor::GeneratedCodeVisitor(GeneratedCode* generatedCode) : generatedCode(generatedCode) {

    }


    GeneratedElementType AssignValueStmt::getElementType() const {
        return GeneratedElementType::AssignValueStmt;
    }

    AssignValueStmt::AssignValueStmt(GeneratedElement *left, GeneratedElement *right) : left(left), right(right) {}


}