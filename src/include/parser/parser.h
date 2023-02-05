#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <optional>
#include "types.h"
#include "lexer.h"
#include "statement.h"
#include "expression.h"

namespace parser
{
    int getUnaryOperatorPrecedence(lexer::TokenType type);
    int getBinaryOperatorPrecedence(lexer::TokenType type);

    // Парсить тонени в абстрактне синтаксичне дерево за правиплами, які описні в файлі "барвінок.ebnf"
    class Parser
    {
    private:
        std::vector<lexer::Token> tokens;
        const std::string code;
        u64 sizeOfTokensVector;
        u64 position;

        BlockStatement* parseBlock(Node* parent);
        Statement* parseStatement(Node* parent);
        Statement* parseExpressionStatement(Node* parent);
        Statement* parseWhileStatement(Node* parent);
        Statement* parseBreakStatement(Node* parent);
        Statement* parseContinueStatement(Node* parent);
        Statement* parseIfStatement(Node* parent, bool elseIf = false);
        std::optional<Statement*> parseElseOrIfStatement(Node* parent);

        Expression* parseExpression(Node* parent);
        Expression* parseLhs(Node* parent);
        Expression* parseRhs(Node* parent);
        Expression* parseAssignmentExpression(Node* parent);
        Expression* parseUnaryExpression(Node* parent);
        Expression* parseBinaryExpression(Node* parent, int parentPrecedence = INT_MAX);
        Expression* parsePrimaryExpression(Node* parent);
        Expression* parseParenthesizedExpression(Node* parent);
        Expression* parseVariableExpression(Node* parent);
        Expression* parseCallExpression(Node* parent);
        Expression* parseLiteralExpression(Node* parent);
        std::vector<Expression*> parseArguments(Node* parent);

        static bool isOperator(lexer::Token token);
        static bool isUnaryOperator(lexer::Token token);
        lexer::Token nextToken();
        lexer::Token peek(int offset);
        lexer::Token matchToken(lexer::TokenType type);
    public:
        BlockStatement* parse();
        Parser(std::vector<lexer::Token> tokens, std::string code);
    };
}

#endif
