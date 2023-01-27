#ifndef NODE_KIND_H
#define NODE_KIND_H

#include "string_enum.h"

namespace parser
{
    STRING_ENUM(
        NodeKind,
        // Statement
        BLOCK_STATEMENT,
        EXPRESSION_STATEMENT,
        WHILE_STATEMENT,
        BREAK_STATEMENT,
        CONTINUE_STATEMENT,
        IF_STATEMENT,
        ELSE_STATEMENT,

        // Expression
        ASSIGNMENT_EXPRESSION,
        BINARY_EXPRESSION,
        UNARY_EXPRESSION,
        PARENTHESIZED_EXPRESSION,
        VARIABLE_EXPRESSION,
        LITERAL_EXPRESSION,
        CALL_EXPRESSION,
    )
}
#endif
