#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <vector>
#include <variant>

#include "lexer.h"
#include "types.h"
#include "node.h"
#include "node_kind.h"

namespace parser
{
    struct Expression : Node
    {
        Expression(NodeKind kind) : Node(kind) {};
    };

    struct AssignmentExpression : Expression
    {
        lexer::Token id;
        lexer::Token assignment;
        Expression* expression;

        AssignmentExpression()
            : Expression(NodeKind::ASSIGNMENT_EXPRESSION) {};
    };

    struct BinaryExpression : Expression
    {
        Expression* left;
        lexer::Token operator_;
        Expression* right;

        BinaryExpression()
            : Expression(NodeKind::BINARY_EXPRESSION) {};
    };

    struct UnaryExpression : Expression
    {
        lexer::Token operator_;
        Expression* operand;

        UnaryExpression()
            : Expression(NodeKind::UNARY_EXPRESSION) {};
    };

    struct ParenthesizedExpression : Expression
    {
        lexer::Token lpar;
        Expression* expression;
        lexer::Token rpar;

        ParenthesizedExpression()
            : Expression(NodeKind::PARENTHESIZED_EXPRESSION) {};
    };

    struct VariableExpression : Expression
    {
        lexer::Token variable;

        VariableExpression()
            : Expression(NodeKind::VARIABLE_EXPRESSION) {};
    };

    struct AttributeExpression : Expression
    {
        Expression* expression;
        lexer::Token attribute;

        AttributeExpression()
            : Expression(NodeKind::ATTRIBUTE_EXPRESSION) {};
    };

    struct LiteralExpression : Expression
    {
        lexer::Token literalToken;
        std::variant<i64, double, bool, std::string> value;

        LiteralExpression()
            : Expression(NodeKind::LITERAL_EXPRESSION) {};
    };

    struct CallExpression : Expression
    {
        Expression* callable;
        lexer::Token lpar;
        std::vector<Expression*> arguments;
        lexer::Token rpar;

        CallExpression()
            : Expression(NodeKind::CALL_EXPRESSION) {};
    };
}

#endif
