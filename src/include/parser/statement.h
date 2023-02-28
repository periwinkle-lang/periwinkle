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
        Statement(Node* parent, NodeKind kind) : Node(parent, kind) {};
    };

    struct BlockStatement : Statement
    {
        std::vector<Statement*> statements;

        BlockStatement(Node* parent)
            : Statement(parent, NodeKind::BLOCK_STATEMENT) {};
    };

    struct WhileStatement : Statement
    {
        lexer::Token keyword;
        Expression* condition;
        BlockStatement* block;

        WhileStatement(Node* parent)
            : Statement(parent, NodeKind::WHILE_STATEMENT) {};
    };

    struct ExpressionStatement : Statement
    {
        Expression* expression;

        ExpressionStatement(Node* parent)
            : Statement(parent, NodeKind::EXPRESSION_STATEMENT) {};
    };

    struct BreakStatement : Statement
    {
        lexer::Token break_;

        BreakStatement(Node* parent)
            : Statement(parent, NodeKind::BREAK_STATEMENT) {};
    };

    struct ContinueStatement : Statement
    {
        lexer::Token continue_;

        ContinueStatement(Node* parent)
            : Statement(parent, NodeKind::CONTINUE_STATEMENT) {};
    };

    struct IfStatement : Statement
    {
        lexer::Token if_;
        Expression* condition;
        BlockStatement* block;
        std::optional<Statement*> elseOrIf;

        IfStatement(Node* parent)
            : Statement(parent, NodeKind::IF_STATEMENT) {};
    };

    struct ElseStatement : Statement
    {
        lexer::Token else_;
        BlockStatement* block;

        ElseStatement(Node* parent)
            : Statement(parent, NodeKind::ELSE_STATEMENT) {};
    };

    struct FunctionDeclaration : Statement
    {
        lexer::Token keyword;
        lexer::Token id;
        lexer::Token lpar;
        std::vector<lexer::Token> parameters;
        lexer::Token rpar;
        BlockStatement* block;

        FunctionDeclaration(Node* parent)
            : Statement(parent, NodeKind::FUNCTION_STATEMENT) {};
    };

    struct ReturnStatement : Statement
    {
        lexer::Token return_;
        std::optional<Expression*> returnValue;

        ReturnStatement(Node* parent)
            : Statement(parent, NodeKind::RETURN_STATEMENT) {};

        ~ReturnStatement()
        {
            if (returnValue)
            {
                delete returnValue.value();
            }
        }
    };
}

#endif
