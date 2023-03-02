#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <map>

#include "types.h"
#include "lexer.h"
#include "statement.h"
#include "expression.h"

namespace parser
{
    // Парсить токени в абстрактне синтаксичне дерево за правилами,
    // які описані в файлі "барвінок.ebnf"
    class Parser
    {
    private:
        std::vector<lexer::Token> tokens;
        const std::string code;
        u64 sizeOfTokensVector;
        u64 position;
        std::map<u64, std::map<void*, std::pair<Node*, u64>>> memo;
        using simpleParseFunction = std::function<Expression*(Node*)>;

        template <typename> struct LeftRecursionDecorator;
        template <typename> friend struct LeftRecursionDecorator;

        template<typename R, typename... Args>
        auto makeLeftRecRule(R(Parser::* f)(Node*, Args...), Parser* parser)
        {
            auto decorated = Parser::LeftRecursionDecorator<R(Node*, Args...)>(f);
            decorated.parser = parser;
            return decorated;
        }

        BlockStatement* parseBlock(Node* parent);
        Statement* parseStatement(Node* parent);
        Statement* parseExpressionStatement(Node* parent);
        Statement* parseWhileStatement(Node* parent);
        Statement* parseBreakStatement(Node* parent);
        Statement* parseContinueStatement(Node* parent);
        Statement* parseIfStatement(Node* parent, bool elseIf = false);
        Statement* parseElseOrIfStatement(Node* parent);
        Statement* parseElseStatement(Node* parent);
        Statement* parseFunctionDeclaration(Node* parent);
        std::vector <lexer::Token> parseParameters(Node* parent);
        Statement* parseReturnStatement(Node* parent);

        Expression* parseExpression(Node* parent);
        Expression* parseLhs(Node* parent);
        Expression* parseRhs(Node* parent);
        simpleParseFunction parseOperator7; Expression* _parseOperator7(Node* parent);
        simpleParseFunction parseOperator6; Expression* _parseOperator6(Node* parent);
        simpleParseFunction parseOperator5; Expression* _parseOperator5(Node* parent);
        simpleParseFunction parseOperator4; Expression* _parseOperator4(Node* parent);
        simpleParseFunction parseOperator3; Expression* _parseOperator3(Node* parent);
        simpleParseFunction parseOperator2; Expression* _parseOperator2(Node* parent);
        Expression* parseOperator1(Node* parent);
        Expression* parseAssignmentExpression(Node* parent);
        Expression* parsePrimaryExpression(Node* parent);
        Expression* parseParenthesizedExpression(Node* parent);
        Expression* parseVariableExpression(Node* parent);
        Expression* parseCallExpression(Node* parent);
        Expression* parseLiteralExpression(Node* parent);
        std::vector<Expression*> parseArguments(Node* parent);

        std::optional<lexer::Token> parseAssignmentOperator();
        std::optional<lexer::Token> parseUnaryOperator();

        lexer::Token nextToken();
        lexer::Token peekToken();
        std::optional<lexer::Token> matchToken(lexer::TokenType type);
        [[noreturn]] void throwParserError(std::string message, lexer::Token token);
    public:
        BlockStatement* parse();
        Parser(std::vector<lexer::Token> tokens, std::string code);
    };
}

#endif
