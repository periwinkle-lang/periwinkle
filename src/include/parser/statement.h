﻿#ifndef STATEMENT_H
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
        std::vector<Statement*> statements;

        BlockStatement(std::vector<Statement*> statements) : statements(statements) {};

        NODE_KIND(BLOCK_STATEMENT);
    };

    struct WhileStatement : Statement
    {
        lexer::Token keyword;
        Expression* condition;
        BlockStatement* block;

        WhileStatement(lexer::Token keyword, Expression* condition, BlockStatement* block)
            : keyword(keyword), condition(condition), block(block)
        {};

        NODE_KIND(WHILE_STATEMENT);
    };

    struct ExpressionStatement : Statement
    {
        Expression* expression;

        ExpressionStatement(Expression* expression) : expression(expression) {};

        NODE_KIND(EXPRESSION_STATEMENT);
    };

    struct BreakStatement : Statement
    {
        lexer::Token break_;

        BreakStatement(lexer::Token break_) : break_(break_) {};

        NODE_KIND(BREAK_STATEMENT);
    };

    struct ContinueStatement : Statement
    {
        lexer::Token continue_;

        ContinueStatement(lexer::Token continue_) : continue_(continue_) {};

        NODE_KIND(CONTINUE_STATEMENT);
    };
}

#endif
