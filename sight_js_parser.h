//
// Created by Aincvy(aincvy@gmail.com) on 2021/7/21.
//

#pragma once

#include "sight_defines.h"
#include "string"
#include "vector"

namespace sight {

    struct FieldOrParameter;
    struct GeneratedBlock;
    struct FunctionDeclaration;

    struct GeneratedElement {
        int fromNode = 0;
    };

    /**
     * Represent a statement;
     */
    struct GeneratedStatement : GeneratedElement {

    };

    /**
     * Represent a word. e.g. varName, identifier
     */
    struct Token : GeneratedElement {
        char word[NAME_BUF_SIZE] = {0};

        Token(const char* string);
        Token(const char* source, size_t startPos, size_t endPos);
    };

    template <class T>
    struct Literal : GeneratedElement {
        T value;
    };

    struct IntLiteral : Literal<int> {
    };
    struct StringLiteral : Literal<std::string> {
    };
    struct CharLiteral : Literal<char> {
    };
    struct BoolLiteral : Literal<bool> {
    };

    struct AnnotationExpr : GeneratedElement {
        Token type;
        // need free ?
        FieldOrParameter* params = nullptr;
    };

    struct FieldOrParameter : GeneratedElement {
        Token name;
        Token type;
        Token defaultValue;
        // need free ?
        AnnotationExpr* annotations = nullptr;
    };

    /**
     * A set of stmts.
     */
    struct GeneratedBlock : GeneratedElement {
        std::vector<GeneratedStatement> statements;
    };

    struct FunctionDeclaration :GeneratedElement {
        Token name;
        // the type of return value.
        Token returnType;
        std::vector<FieldOrParameter> params;
        // need free ?
        AnnotationExpr* annotations = nullptr;
        GeneratedBlock block;
    };

    struct GeneratedClass : GeneratedElement {
        std::vector<FieldOrParameter> fields;
        std::vector<FunctionDeclaration> functions;
    };

    struct FunctionCallExpr : GeneratedStatement {
        GeneratedElement object;
        Token name;
        std::vector<GeneratedElement> args;
        // For static call use.
        Token classname;
    };

    struct FunctionRefExpr : FunctionCallExpr {

    };


    struct AssignValueStmt : GeneratedStatement {

    };

    struct IfStmt : GeneratedStatement {

    };

    struct ForStmt : GeneratedStatement {

    };

    class GeneratedCode{
    public:
        FunctionDeclaration* currentFunction = nullptr;
        GeneratedStatement* currentStatement = nullptr;

        const static Token invalidToken;

        GeneratedCode(const char*source);
        ~GeneratedCode() = default;

        Token getToken(size_t startPos, size_t endPos);
        Token getToken(void *nodePointer);

    private:
        std::vector<GeneratedElement> elements;
        const char* source;
        size_t sourceLength;
    };


    /**
     * test tree-sitter functions
     * @return
     */
    int testParser();

    int initParser();

    void freeParser();

    /**
     *
     * @param source
     */
    void parseSource(const char * source);

}