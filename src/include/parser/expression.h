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
        Expression(Node* parent, NodeKind kind) : Node(parent, kind) {};
    };

    struct AssignmentExpression : Expression
    {
        lexer::Token id;
        lexer::Token assignment;
        Expression* expression;

        AssignmentExpression(Node* parent)
            : Expression(parent, NodeKind::ASSIGNMENT_EXPRESSION) {};
    };

    struct BinaryExpression : Expression
    {
        Expression* left;
        lexer::Token operator_;
        Expression* right;

        BinaryExpression(Node* parent)
            : Expression(parent, NodeKind::BINARY_EXPRESSION) {};
    };

    struct UnaryExpression : Expression
    {
        lexer::Token operator_;
        Expression* operand;

        UnaryExpression(Node* parent)
            : Expression(parent, NodeKind::UNARY_EXPRESSION) {};
    };

    struct ParenthesizedExpression : Expression
    {
        lexer::Token lpar;
        Expression* expression;
        lexer::Token rpar;

        ParenthesizedExpression(Node* parent)
            : Expression(parent, NodeKind::PARENTHESIZED_EXPRESSION) {};
    };

    struct VariableExpression : Expression
    {
        lexer::Token variable;

        VariableExpression(Node* parent)
            : Expression(parent, NodeKind::VARIABLE_EXPRESSION) {};
    };

    struct AttributeExpression : Expression
    {
        Expression* expression;
        lexer::Token attribute;

        AttributeExpression(Node* parent)
            : Expression(parent, NodeKind::ATTRIBUTE_EXPRESSION) {};
    };

    struct LiteralExpression : Expression
    {
        lexer::Token literalToken;
        std::variant<i64, double, bool, std::string> value;

        LiteralExpression(Node* parent)
            : Expression(parent, NodeKind::LITERAL_EXPRESSION) {};
    };

    struct CallExpression : Expression
    {
        Expression* callable;
        lexer::Token lpar;
        std::vector<Expression*> arguments;
        lexer::Token rpar;

        CallExpression(Node* parent)
            : Expression(parent, NodeKind::CALL_EXPRESSION) {};
    };
}

#endif
