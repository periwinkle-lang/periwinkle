#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <vector>
#include <variant>
#include "lexer.h"
#include "types.h"
#include "node.h"
#include "node_kind.h"
#include "plogger.h"

namespace parser
{
    struct Expression : Node
    {
        virtual NodeKind kind() override = 0;
    };

    struct AssignmentExpression : Expression
    {
        lexer::Token id;
        lexer::Token assignment;
        Expression* expression;

        AssignmentExpression(lexer::Token id, lexer::Token assignment, Expression* expression)
            : id(id), assignment(assignment), expression(expression)
        {
            using enum lexer::TokenType;
            switch (assignment.tokenType)
            {
            case EQUAL:
            case PLUS_EQUAL:
            case MINUS_EQUAL:
            case STAR_EQUAL:
            case SLASH_EQUAL:
            case PERCENT_EQUAL:
            case BACKSLASH_EQUAL:
                break;
            default:
                plog::fatal << "Неправильний оператор призначення: \""
                    << lexer::stringEnum::enumToString(assignment.tokenType) << "\"";
            }
        };

        NODE_KIND(ASSIGNMENT_EXPRESSION);
    };

    struct BinaryExpression : Expression
    {
        Expression* left;
        lexer::Token operator_;
        Expression* right;

        BinaryExpression(Expression* left, lexer::Token operator_, Expression* right)
            : left(left), operator_(operator_), right(right) {};

        NODE_KIND(BINARY_EXPRESSION);
    };

    struct UnaryExpression : Expression
    {
        lexer::Token operator_;
        Expression* operand;

        UnaryExpression(lexer::Token operator_, Expression* operand)
            : operator_(operator_), operand(operand) {};

        NODE_KIND(UNARY_EXPRESSION);
    };

    struct ParenthesizedExpression : Expression
    {
        lexer::Token lpar;
        Expression* expression;
        lexer::Token rpar;

        ParenthesizedExpression(lexer::Token lpar, Expression* expression, lexer::Token rpar)
            : lpar(lpar), expression(expression), rpar(rpar) {};

        NODE_KIND(PARENTHESIZED_EXPRESSION);
    };

    struct VariableExpression : Expression
    {
        lexer::Token variable;

        VariableExpression(lexer::Token variable) : variable(variable) {};

        NODE_KIND(VARIABLE_EXPRESSION);
    };

    struct LiteralExpression : Expression
    {
        lexer::Token literalToken;

        std::variant<i64, double, bool, std::string> value;

        LiteralExpression(lexer::Token literalToken) : literalToken(literalToken)
        {
            switch (literalToken.tokenType)
            {
            case lexer::TokenType::NUMBER:
                value = std::stoll(literalToken.text);
                break;
            case lexer::TokenType::REAL:
                value = std::stod(literalToken.text);
                break;
            case lexer::TokenType::BOOLEAN:
                value = std::string("правда").compare(literalToken.text) == 0;
                break;
            case lexer::TokenType::STRING:
                value = literalToken.text;
                break;
            case lexer::TokenType::NULL_:
                break;
            default:
                plog::fatal << "Переданий неправильний токен: \""
                    << lexer::stringEnum::enumToString(literalToken.tokenType) << "\"";
            }
        };

        NODE_KIND(LITERAL_EXPRESSION);
    };

    struct CallExpression : Expression
    {
        lexer::Token identifier;
        lexer::Token lpar;
        std::vector<Expression*> arguments;
        lexer::Token rpar;

        CallExpression(lexer::Token identifier, lexer::Token lpar,
            std::vector<Expression*> arguments, lexer::Token rpar)
            : identifier(identifier), lpar(lpar), arguments(arguments), rpar(rpar)
        {};

        NODE_KIND(CALL_EXPRESSION);
    };
}

#endif
