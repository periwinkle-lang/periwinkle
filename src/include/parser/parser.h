#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "types.h"
#include "lexer.h"
#include "statement.h"
#include "expression.h"

namespace parser
{
    int getUnaryOperatorPrecedence(lexer::TokenType type);
    int getBinaryOperatorPrecedence(lexer::TokenType type);

    class Parser
    {
    private:
        std::vector<lexer::Token> tokens;
        const std::string code;
        u64 sizeOfTokensVector;
        u64 position;

        BlockStatement* parseBlock();
        Statement* parseStatement();
        Statement* parseExpressionStatement();
        Statement* parseWhileStatement();

        Expression* parseExpression();
        Expression* parseAssignmentOrCallOrLiteralExpression();
        Expression* parseAssignmentExpression();
        Expression* parseBinaryExpression(int parentPrecedence = INT_MAX);
        Expression* parsePrimaryExpression();
        Expression* parseParenthesizedExpression();
        Expression* parseVariableOrCallExpression();
        Expression* parseVariableExpression();
        Expression* parseCallExpression();
        Expression* parseLiteralExpression();
        std::vector<Expression*> parseArguments();

        lexer::Token nextToken();
        lexer::Token peek(int offset);
        lexer::Token matchToken(lexer::TokenType type);
    public:
        BlockStatement* parse();
        Parser(std::vector<lexer::Token> tokens, std::string code);
    };
}

#endif
