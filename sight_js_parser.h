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

    struct FieldOrParameter;
    struct GeneratedBlock;
    struct FunctionDeclaration;

    enum class GeneratedElementType {
        Token,
        IntLiteral,
        FloatLiteral,
        StringLiteral,
        BoolLiteral,
        CharLiteral,
        GeneratedBlock,
        FunctionDeclaration,
        FieldOrParameter,
        BinaryExpr,

        AssignValueStmt,
    };

    struct GeneratedElement : ResetAble{
        int fromNode = 0;

        GeneratedElement() = default;
        explicit GeneratedElement(int fromNode);
        virtual ~GeneratedElement() = default;

        virtual GeneratedElementType getElementType() const = 0;
        void reset() override;
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

        Token() = default;
        Token(const char* string);
        Token(const char* source, size_t startPos, size_t endPos);

        /**
         * convert part string to int
         * @return If success, the value. Otherwise fallback.
         */
        int asInt(int fallback = 0) const;

        /**
         * convert part string to float
         * @return
         */
        float asFloat(float fallback = 0) const;

        /**
         * convert part string to double
         * @return
         */
        double asDouble(double fallback = 0) const;

        /**
         * convert part string to bool
         * @return
         */
        bool asBool(bool fallback = false) const;

        /**
         *
         * @param rhs
         * @return
         */
        bool operator==(const Token& rhs) const;

        GeneratedElementType getElementType() const override;
    };

    template <class T>
    struct Literal : GeneratedElement {
        T value;

        Literal() = default;
        explicit Literal(T value);
        ~Literal() override = default;
    };

    template<class T>
    Literal<T>::Literal(T value) {
        this->value = value;
    }

    struct IntLiteral : Literal<int> {
        IntLiteral() = default;
        explicit IntLiteral(int value);
        ~IntLiteral() override;

        GeneratedElementType getElementType() const override;
    };

    struct FloatLiteral : Literal<float>{
        FloatLiteral() =default;
        explicit FloatLiteral(float value);
        ~FloatLiteral() override;

        GeneratedElementType getElementType() const override;
    };
    struct StringLiteral : Literal<std::string> {
        StringLiteral() = default;
        explicit StringLiteral(std::string value);
    };
    struct CharLiteral : Literal<char> {
        CharLiteral() = default;
        explicit CharLiteral(char value);
    };
    struct BoolLiteral : Literal<bool> {
        BoolLiteral() = default;
        explicit BoolLiteral(bool value);
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

        GeneratedElementType getElementType() const override;
    };

    /**
     * A set of stmts.
     */
    struct GeneratedBlock : GeneratedElement {
        std::vector<GeneratedStatement*> statements;

        GeneratedElementType getElementType() const override;
    };

    struct FunctionDeclaration :GeneratedElement {
        Token name;
        // the type of return value.
        Token returnType;
        std::vector<FieldOrParameter> params;
        // need free ?
        AnnotationExpr* annotations = nullptr;
        GeneratedBlock block;

        FunctionDeclaration(const char*name, const char*returnType);
        FunctionDeclaration() = default;
        ~FunctionDeclaration() = default;

        GeneratedElementType getElementType() const override;
    };

    struct GeneratedClass : GeneratedElement {
        std::vector<FieldOrParameter> fields;
        std::vector<FunctionDeclaration> functions;
    };

    struct FunctionCallExpr : GeneratedStatement {
        GeneratedElement* object;
        Token name;
        std::vector<GeneratedElement*> args;
        // For static call use.
        Token classname;
    };

    struct FunctionRefExpr : FunctionCallExpr {

    };

    struct BinaryExpr : GeneratedElement {
        GeneratedElement* left;
        GeneratedElement* symbol;
        GeneratedElement* right;

        BinaryExpr() = default;

        BinaryExpr(GeneratedElement *left, GeneratedElement *symbol, GeneratedElement *right);

        ~BinaryExpr() = default;
        GeneratedElementType getElementType() const override;
    };


    struct AssignValueStmt : GeneratedStatement {
        GeneratedElement* left = nullptr;
        GeneratedElement* right = nullptr;

        AssignValueStmt() = default;
        ~AssignValueStmt() = default;

        AssignValueStmt(GeneratedElement *left, GeneratedElement *right);

        GeneratedElementType getElementType() const override;
    };

    struct IfStmt : GeneratedStatement {

    };

    struct ForStmt : GeneratedStatement {

    };

    class GeneratedCode{
    public:
        FunctionDeclaration* currentFunction = nullptr;
        GeneratedStatement* currentStatement = nullptr;
        GeneratedClass* currentClass = nullptr;
        AssignValueStmt* assignValueStmt = nullptr;

        const static Token invalidToken;

        GeneratedCode(const char*source);
        ~GeneratedCode() = default;


        Token getToken(size_t startPos, size_t endPos) const;

        /**
         *
         * @param nodePointer TSNode*
         * @return
         */
        Token getToken(void *nodePointer) const;

        Token getToken(std::any & tsNode) const;

        /**
         * Use this when node type is string.
         * @param startPos
         * @param endPos
         * @return
         */
        [[nodiscard]] std::string getString(size_t startPos, size_t endPos) const;


        void addFunction(const char*name, const char* returnType);

        GeneratedElement* addElement(GeneratedElement const & element);

    private:
        const char* source;
        size_t sourceLength;

        // memory
        SightArray<Token> tokenArray;
        SightArray<FunctionDeclaration> functionArray;
        SightArray<BinaryExpr> binaryExprArray;
        SightArray<IntLiteral> intLiteralArray;
        SightArray<AssignValueStmt> assignValueStmtArray;
    };


    /**
     * Visitor
     */
    class GeneratedCodeVisitor {
    public:

        GeneratedCodeVisitor(GeneratedCode* generatedCode);

        /**
         * Visit a element.
         * @param element
         * @return true goto next, false break.
         */
        virtual bool visit(GeneratedElement *element);

        virtual bool visit(AssignValueStmt* assignValueStmt);

        virtual bool visit(BinaryExpr* binaryExpr);

        virtual bool visit(IntLiteral* intLiteral);

        virtual bool visit(Token* token);



    private:
        GeneratedCode* generatedCode = nullptr;
    };


    int initParser();

    void freeParser();

    /**
     *
     * @param source
     */
    void parseSource(const char * source);

}