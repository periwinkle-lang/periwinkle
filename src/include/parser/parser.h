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
        using simpleParseFunction = std::function<Expression*(void)>;

        template <typename> struct LeftRecursionDecorator;
        template <typename> friend struct LeftRecursionDecorator;

        template<typename R, typename... Args>
        auto makeLeftRecRule(R(Parser::* f)(Args...), Parser* parser)
        {
            auto decorated = Parser::LeftRecursionDecorator<R(Args...)>(f);
            decorated.parser = parser;
            return decorated;
        }

        BlockStatement* parseBlock();
        Statement* parseStatement();
        Statement* parseExpressionStatement();
        Statement* parseWhileStatement();
        Statement* parseBreakStatement();
        Statement* parseContinueStatement();
        Statement* parseIfStatement(bool elseIf = false);
        Statement* parseElseOrIfStatement();
        Statement* parseElseStatement();
        Statement* parseFunctionDeclaration();
        std::vector <lexer::Token> parseParameters();
        Statement* parseReturnStatement();

        Expression* parseExpression();
        Expression* parseLhs();
        Expression* parseRhs();
        simpleParseFunction parseOperator7; Expression* _parseOperator7();
        simpleParseFunction parseOperator6; Expression* _parseOperator6();
        simpleParseFunction parseOperator5; Expression* _parseOperator5();
        simpleParseFunction parseOperator4; Expression* _parseOperator4();
        simpleParseFunction parseOperator3; Expression* _parseOperator3();
        simpleParseFunction parseOperator2; Expression* _parseOperator2();
        Expression* parseOperator1();
        Expression* parseAssignmentExpression();
        simpleParseFunction parsePrimaryExpression; Expression* _parsePrimaryExpression();
        Expression* parseParenthesizedExpression();
        Expression* parseAttributeExpression();
        Expression* parseVariableExpression();
        Expression* parseCallExpression();
        Expression* parseLiteralExpression();
        std::vector<Expression*> parseArguments();

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
