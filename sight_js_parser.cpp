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

        struct TreeSitterFields {
            TSFieldId _operator;
            TSFieldId left;
            TSFieldId right;
            TSFieldId argument;
        };

        TreeSitterFields fields;

        /**
         * Used for tree-sitter node parse.
         */
        absl::flat_hash_map<std::string,std::function<GeneratedElement*(GeneratedCode &,TSNode &)>> nodeHandlerMap;

        void initTreeSitterFields(TSLanguage* language){

            constexpr const char* fieldOperator = "operator";
            constexpr const char* fieldLeft = "left";
            constexpr const char* fieldRight = "right";
            constexpr const char* fieldArgument = "argument";

            fields._operator = ts_language_field_id_for_name(language, fieldOperator, strlen(fieldOperator));
            fields.left = ts_language_field_id_for_name(language, fieldLeft, strlen(fieldLeft));
            fields.right = ts_language_field_id_for_name(language, fieldRight, strlen(fieldRight));
            fields.argument = ts_language_field_id_for_name(language, fieldArgument, strlen(fieldArgument));


        }

        /**
         * Generate this node to GeneratedElement
         * @param generatedCode
         * @param node
         * @return
         */
        GeneratedElement* generate(GeneratedCode & generatedCode, TSNode & node){
            if (ts_node_is_null(node)) {
                return nullptr;
            }

            auto nodeType = ts_node_type(node);
            auto iter = nodeHandlerMap.find(nodeType);
            if (iter != nodeHandlerMap.end()) {
                return iter->second(generatedCode, node);
            }
            dbg(std::string("do not handle ") + nodeType);

            return nullptr;
        }

        /**
         * Generate this node's children nodes to GeneratedElement. Do not generate this node.
         * @param generatedCode
         * @param node
         * @return always return nullptr or block ?
         */
        GeneratedElement* generateChildrenNode(GeneratedCode & generatedCode, TSNode & node){
            if (ts_node_is_null(node)) {
                return nullptr;
            }

            auto cursor = ts_tree_cursor_new(node);
            if (!ts_tree_cursor_goto_first_child(&cursor)) {
                return nullptr;
            }
            auto childNode = ts_tree_cursor_current_node(&cursor);
            generate(generatedCode, childNode);

            while (ts_tree_cursor_goto_next_sibling(&cursor)) {
                childNode = ts_tree_cursor_current_node(&cursor);
                generate(generatedCode, childNode);
            }

            return nullptr;
        }

        GeneratedElement *generateEmpty(GeneratedCode & generatedCode, TSNode & node){
            return nullptr;
        }


        /**
         * Use `dbg()` to display hierarchy.
         * @param generatedCode
         * @param node
         */
        void showNodeHierarchy(GeneratedCode & generatedCode, TSNode & node){
            if (ts_node_is_null(node)) {
                return;
            }

            char *string = ts_node_string(node);
            dbg(string);
            free(string);
        }

        void initCaseMap(){
            nodeHandlerMap["number"] = [](GeneratedCode& generatedCode,TSNode& node){
                auto token = generatedCode.getToken(&node);
                if (token == GeneratedCode::invalidToken) {
                    // may need to do something.
                }

                auto f = token.asFloat();
                if (f == static_cast<int>(f) ) {
                    return generatedCode.addElement(IntLiteral(token.asInt()));
                }

                return generatedCode.addElement(FloatLiteral(f));
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
                auto a = ts_node_child_by_field_id(node, fields.left);
                auto b = ts_node_child_by_field_id(node, fields._operator);
                auto c = ts_node_child_by_field_id(node, fields.right);
                auto left = generate(generatedCode, a);
                auto symbol = generatedCode.addElement(generatedCode.getToken(&b));
                auto right = generate(generatedCode, c);

                return generatedCode.addElement(BinaryExpr(left, symbol, right));
            };

            nodeHandlerMap["assignment_expression"] = [](GeneratedCode &generatedCode,TSNode &node){
                dbg(ts_node_child_count(node));
                auto a = ts_node_named_child(node, 0);
                auto b = ts_node_named_child(node, 1);
                dbg("assignment_expression");

                auto left = generate(generatedCode, a);
                auto right = generate(generatedCode, b);
                return generatedCode.addElement(AssignValueStmt(left, right));
            };

            nodeHandlerMap["unary_expression"] = [](GeneratedCode &generatedCode,TSNode &node){

                return nullptr;
            };

            nodeHandlerMap["program"] = generateChildrenNode;
            nodeHandlerMap["expression_statement"] = generateChildrenNode;
            nodeHandlerMap[";"] = generateEmpty;

        }

    }

    int initParser() {
        if (g_parser) {
            return 1;
        }

        g_parser = ts_parser_new();
        // Set the parser's language
        auto language = tree_sitter_javascript();
        ts_parser_set_language(g_parser, language);
        initTreeSitterFields(language);

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
        dbg(source);

        TSTree *tree = ts_parser_parse_string(
                g_parser,
                NULL,
                source,
                strlen(source)
        );

        GeneratedCode generatedCode(source);
        TSNode rootNode = ts_tree_root_node(tree);

        showNodeHierarchy(generatedCode, rootNode);
        generate(generatedCode, rootNode);

        if (generatedCode.assignValueStmt) {
            dbg("start visitor");
            GeneratedCodeVisitor visitor(&generatedCode);
            visitor.visit(generatedCode.assignValueStmt);
        }

        ts_tree_delete(tree);
        dbg("end of function parseSource");
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
            case GeneratedElementType::AssignValueStmt:
            {
                auto const & e = (AssignValueStmt const &)element;
                assignValueStmt = assignValueStmtArray.add(e);
                result = assignValueStmt;
                break;
            }
        }

        return result;
    }

    Token GeneratedCode::getToken(std::any &tsNode) const {
        auto node = std::any_cast<TSNode&>(tsNode);
        if (ts_node_is_null(node)) {
            return invalidToken;
        }
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

    BinaryExpr::BinaryExpr(GeneratedElement *left, GeneratedElement *symbol, GeneratedElement *right) : left(left),
                                                                                                        symbol(symbol),
                                                                                                        right(right) {}


    bool GeneratedCodeVisitor::visit(GeneratedElement *element) {
        if (element == nullptr) {
            return false;
        }

        dbg(element->getElementType());

        switch (element->getElementType()) {
            case GeneratedElementType::Token:
                return visit((Token*) element);
            case GeneratedElementType::IntLiteral:
                return visit((IntLiteral*) element);
            case GeneratedElementType::StringLiteral:
                break;
            case GeneratedElementType::BoolLiteral:
                break;
            case GeneratedElementType::CharLiteral:
                break;
            case GeneratedElementType::GeneratedBlock:
                break;
            case GeneratedElementType::FunctionDeclaration:
                break;
            case GeneratedElementType::FieldOrParameter:
                break;
            case GeneratedElementType::BinaryExpr:
                return visit((BinaryExpr*) element);
            case GeneratedElementType::AssignValueStmt:
                return visit((AssignValueStmt*) element);
        }

        return false;
    }

    GeneratedCodeVisitor::GeneratedCodeVisitor(GeneratedCode* generatedCode) : generatedCode(generatedCode) {

    }

    bool GeneratedCodeVisitor::visit(AssignValueStmt *assignValueStmt) {
        dbg("AssignValueStmt");

        visit(assignValueStmt->left);
        visit(assignValueStmt->right);
        return false;
    }

    bool GeneratedCodeVisitor::visit(BinaryExpr *binaryExpr) {
        dbg("BinaryExpr");

        visit(binaryExpr->left);
        visit(binaryExpr->symbol);
        visit(binaryExpr->right);
        return true;
    }

    bool GeneratedCodeVisitor::visit(IntLiteral *intLiteral) {
        dbg("IntLiteral", intLiteral->value);
        return true;
    }

    bool GeneratedCodeVisitor::visit(Token *token) {
        dbg("Token", token->word);
        return true;
    }


    GeneratedElementType AssignValueStmt::getElementType() const {
        return GeneratedElementType::AssignValueStmt;
    }

    AssignValueStmt::AssignValueStmt(GeneratedElement *left, GeneratedElement *right) : left(left), right(right) {}


    FloatLiteral::FloatLiteral(float value) : Literal(value) {

    }

    FloatLiteral::~FloatLiteral() {

    }

    GeneratedElementType FloatLiteral::getElementType() const {
        return GeneratedElementType::FloatLiteral;
    }
}