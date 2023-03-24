#ifndef STATEMENT_H
#define STATEMENT_H

#include <string>
#include <vector>
#include <optional>

#include "node.h"
#include "expression.h"
#include "node_kind.h"
#include "lexer.h"

namespace parser
{
    struct Statement : Node
    {
        Statement(NodeKind kind) : Node(kind) {};
    };

    struct BlockStatement : Statement
    {
        std::vector<Statement*> statements;

        BlockStatement()
            : Statement(NodeKind::BLOCK_STATEMENT) {};
    };

    struct WhileStatement : Statement
    {
        lexer::Token keyword;
        Expression* condition;
        BlockStatement* block;

        WhileStatement()
            : Statement(NodeKind::WHILE_STATEMENT) {};
    };

    struct ExpressionStatement : Statement
    {
        Expression* expression;

        ExpressionStatement()
            : Statement(NodeKind::EXPRESSION_STATEMENT) {};
    };

    struct BreakStatement : Statement
    {
        lexer::Token break_;

        BreakStatement()
            : Statement(NodeKind::BREAK_STATEMENT) {};
    };

    struct ContinueStatement : Statement
    {
        lexer::Token continue_;

        ContinueStatement()
            : Statement(NodeKind::CONTINUE_STATEMENT) {};
    };

    struct IfStatement : Statement
    {
        lexer::Token if_;
        Expression* condition;
        BlockStatement* block;
        std::optional<Statement*> elseOrIf;

        IfStatement()
            : Statement(NodeKind::IF_STATEMENT) {};
    };

    struct ElseStatement : Statement
    {
        lexer::Token else_;
        BlockStatement* block;

        ElseStatement()
            : Statement(NodeKind::ELSE_STATEMENT) {};
    };

    struct FunctionDeclaration : Statement
    {
        lexer::Token keyword;
        lexer::Token id;
        lexer::Token lpar;
        std::vector<lexer::Token> parameters;
        std::optional<lexer::Token> variadicParameter;
        lexer::Token rpar;
        BlockStatement* block;

        FunctionDeclaration()
            : Statement(NodeKind::FUNCTION_STATEMENT) {};
    };

    struct ReturnStatement : Statement
    {
        lexer::Token return_;
        std::optional<Expression*> returnValue;

        ReturnStatement()
            : Statement(NodeKind::RETURN_STATEMENT) {};

        ~ReturnStatement()
        {
            if (returnValue)
            {
                delete returnValue.value();
            }
        }
    };

    struct ForEachStatement : Statement
    {
        lexer::Token forEach;
        lexer::Token variable;
        lexer::Token eachFrom; // Для "з"
        Expression* expression;
        BlockStatement* block;

        ForEachStatement()
            : Statement(NodeKind::FOR_EACH_STATEMET) {};
    };
}

#endif
