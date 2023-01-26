#ifndef STATEMENT_H
#define STATEMENT_H

#include <string>
#include <vector>
#include "node.h"
#include "expression.h"
#include "node_kind.h"
#include "lexer.h"
#include "scope.h"


namespace parser
{
    struct Statement : Node
    {
        virtual NodeKind kind() override = 0;
    };

    struct BlockStatement : Statement
    {
        std::vector<Expression*> expressions;

        BlockStatement(std::vector<Expression*> expressions) : expressions(expressions) {};

        NODE_KIND(BLOCK_STATEMENT);
    };

    /*
    struct VariableStatement : Statement
    {
        Token identifier;

        VariableStatement(int lineno, Token identifier)
            : Statement(lineno), identifier(identifier)
        {};

        virtual NodeKind kind() final { return NodeKind::VARIABLE_STATEMENT; };
    };

    struct ParameterStatement : Statement
    {
        Token identifier;
        Token equal;
        Expression* defaultValue;

        ParameterStatement(int lineno, Token identifier, Token equal, Expression* defaultValue)
            : Statement(lineno), identifier(identifier), equal(equal), defaultValue(defaultValue)
        {};

        virtual NodeKind kind() final { return NodeKind::PARAMETER_STATEMENT; };
    };

    struct FunctionDeclaration : Statement
    {
        Token functionToken;
        Token identifier;
        Token openParenthesis;
        std::vector<Node*> parametersAndSeparators;
        Token closeParenthesis;
        BlockStatement* body;

        FunctionDeclaration(int lineno, Token functionToken, Token identifier, Token openParenthesis,
            std::vector<Node*> parametersAndSeparators, Token closeParenthesis, BlockStatement* body)
            : Statement(lineno), functionToken(functionToken), identifier(identifier),
            openParenthesis(openParenthesis), parametersAndSeparators(parametersAndSeparators),
            closeParenthesis(closeParenthesis), body(body)
        {};

        virtual NodeKind kind() final { return NodeKind::FUNCTION_STATEMENT; };
    };

    struct ElseStatement : Statement
    {
        Token keyword;
        Statement* statement;

        ElseStatement(int lineno, Token keyword, Statement* statement)
            : Statement(lineno), keyword(keyword), statement(statement)
        {};

        virtual NodeKind kind() final { return NodeKind::ELSE_STATEMENT; };
    };

    struct IfStatement : Statement
    {
        Token keyword;
        Expression* condition;
        BlockStatement* then;
        ElseStatement* else_;

        IfStatement(int lineno, Token keyword, Expression* condition, BlockStatement* then, ElseStatement* else_)
            : Statement(lineno), keyword(keyword), condition(condition), then(then), else_(else_)
        {};

        virtual NodeKind kind() final { return NodeKind::IF_STATEMENT; };
    };

    struct WhileStatement : Statement
    {
        Token keyword;
        Expression* condition;
        BlockStatement* statement;

        WhileStatement(int lineno, Token keyword, Expression* condition, BlockStatement* statement)
            : Statement(lineno), keyword(keyword), condition(condition), statement(statement)
        {};

        virtual NodeKind kind() final { return NodeKind::WHILE_STATEMENT; };
    };

    struct ForStatement : Statement
    {
        Token keyword;
        Token identifier;
        Token colon;
        Expression* iterationExpression;
        BlockStatement* statement;

        ForStatement(int lineno, Token keyword, Token identifier, Token colon,
            Expression* iterationExpression, BlockStatement* statement)
            : Statement(lineno), keyword(keyword), identifier(identifier), colon(colon),
            iterationExpression(iterationExpression), statement(statement)
        {};

        virtual NodeKind kind() final { return NodeKind::FOR_STATEMENT; };
    };

    struct BreakStatement : Statement
    {
        Token keyword;

        BreakStatement(int lineno, Token keyword) : Statement(lineno), keyword(keyword) {};

        virtual NodeKind kind() final { return NodeKind::BREAK_STATEMENT; };
    };

    struct ContinueStatement : Statement
    {
        Token keyword;

        ContinueStatement(int lineno, Token keyword) : Statement(lineno), keyword(keyword) {};

        virtual NodeKind kind() final { return NodeKind::CONTINUE_STATEMENT; };
    };

    struct ReturnStatement : Statement
    {
        Token keyword;
        Expression* expression;

        ReturnStatement(int lineno, Token keyword, Expression* expression)
            : Statement(lineno), keyword(keyword), expression(expression) {};

        virtual NodeKind kind() final { return NodeKind::RETURN_STATEMENT; };
    };

    struct ExpressionStatement : Statement
    {
        Expression* expression;

        ExpressionStatement(int lineno, Expression* expression)
            : Statement(lineno), expression(expression) {};

        virtual NodeKind kind() final { return NodeKind::EXPRESSION_STATEMENT; };
    };

    struct CommaStatement : Statement
    {
        Token comma;

        CommaStatement(int lineno, Token comma) : Statement(lineno), comma(comma) {};

        virtual NodeKind kind() final { return NodeKind::COMMA_STATEMENT; };
    };

    struct SemicolonStatement : Statement
    {
        Token semicolon;

        SemicolonStatement(int lineno, Token semicolon) : Statement(lineno), semicolon(semicolon) {};

        virtual NodeKind kind() final { return NodeKind::SEMICOLON_STATEMENT; };
    };*/
}

#endif
