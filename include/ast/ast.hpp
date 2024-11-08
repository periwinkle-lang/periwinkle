#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>

#include "string_enum.hpp"
#include "types.hpp"


namespace ast
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
        FUNCTION_STATEMENT,
        RETURN_STATEMENT,
        FOR_EACH_STATEMENT,
        TRY_CATCH_STATEMENT,
        RAISE_STATEMENT,

        // Expression
        ASSIGNMENT_EXPRESSION,
        BINARY_EXPRESSION,
        UNARY_EXPRESSION,
        PARENTHESIZED_EXPRESSION,
        VARIABLE_EXPRESSION,
        ATTRIBUTE_EXPRESSION,
        LITERAL_EXPRESSION,
        CALL_EXPRESSION,
    )

    struct Node
    {
        NodeKind kind;

        Node(NodeKind kind) : kind(kind) {};
        virtual ~Node() {};
    };

    struct Statement : Node
    {
        Statement(NodeKind kind) : Node(kind) {};
    };

    struct Expression : Node
    {
        Expression(NodeKind kind) : Node(kind) {};
    };

    struct Token
    {
        size_t lineno;
        size_t col;
        std::string text;
    };

    struct BlockStatement : Statement
    {
        std::vector<Statement*> statements;

        BlockStatement(std::vector<Statement*> statements)
            : Statement(NodeKind::BLOCK_STATEMENT), statements(statements) {};

        virtual ~BlockStatement()
        {
            for (auto node: statements)
                delete node;
        };
    };

    struct WhileStatement : Statement
    {
        Token keyword;
        std::unique_ptr<Expression> condition;
        std::unique_ptr<BlockStatement> block;

        WhileStatement(Token keyword, Expression* condition, BlockStatement* block)
            : Statement(NodeKind::WHILE_STATEMENT), keyword(keyword), condition(condition), block(block) {};
    };

    struct ExpressionStatement : Statement
    {
        std::unique_ptr<Expression> expression;

        ExpressionStatement(Expression* expression)
            : Statement(NodeKind::EXPRESSION_STATEMENT), expression(expression) {};
    };

    struct BreakStatement : Statement
    {
        Token break_;

        BreakStatement(Token break_)
            : Statement(NodeKind::BREAK_STATEMENT), break_(break_) {};
    };

    struct ContinueStatement : Statement
    {
        Token continue_;

        ContinueStatement(Token continue_)
            : Statement(NodeKind::CONTINUE_STATEMENT), continue_(continue_) {};
    };

    struct IfStatement : Statement
    {
        Token if_;
        std::unique_ptr<Expression> condition;
        std::unique_ptr<BlockStatement> block;
        std::optional<std::unique_ptr<Statement>> elseOrIf;

        IfStatement(Token if_, Expression* condition, BlockStatement* block, std::optional<Statement*> elseOrIf)
            : Statement(NodeKind::IF_STATEMENT), if_(if_), condition(condition), block(block), elseOrIf(elseOrIf) {};
    };

    struct ElseStatement : Statement
    {
        Token else_;
        std::unique_ptr<BlockStatement> block;

        ElseStatement(Token else_, BlockStatement* block)
            : Statement(NodeKind::ELSE_STATEMENT), else_(else_), block(block) {};
    };

    struct FunctionDeclaration : Statement
    {
        using parameters_t = std::vector<Token>;
        using variadicParameter_t = std::optional<Token>;
        using defaultParameter_t = std::pair<Token, Expression*>;
        using defaultParameters_t = std::vector<defaultParameter_t>;

        Token id;
        parameters_t parameters;
        variadicParameter_t variadicParameter;
        defaultParameters_t defaultParameters;
        std::unique_ptr<BlockStatement> block;

        FunctionDeclaration(
            Token id, parameters_t parameters, variadicParameter_t variadicParameter,
            defaultParameters_t defaultParameters, BlockStatement* block)
            :
            Statement(NodeKind::FUNCTION_STATEMENT), id(id), parameters(parameters),
            variadicParameter(variadicParameter), defaultParameters(defaultParameters), block(block) {};
    };

    struct ReturnStatement : Statement
    {
        Token return_;
        std::optional<std::unique_ptr<Expression>> returnValue;

        ReturnStatement(Token return_, std::optional<Expression*> returnValue)
            : Statement(NodeKind::RETURN_STATEMENT), return_(return_), returnValue(returnValue) {};
    };

    struct ForEachStatement : Statement
    {
        Token forEach;
        Token variable;
        std::unique_ptr<Expression> expression;
        std::unique_ptr<BlockStatement> block;

        ForEachStatement(Token forEach, Token variable, Expression* expression, BlockStatement* block)
            :
            Statement(NodeKind::FOR_EACH_STATEMENT), forEach(forEach), variable(variable),
            expression(expression), block(block) {};
    };

    struct CatchBlock
    {
        Token catch_;
        Token exceptionName;
        std::optional<Token> as;
        std::optional<Token> variableName;
        std::unique_ptr<BlockStatement> block;
    };

    struct FinallyBlock
    {
        Token finally_;
        std::unique_ptr<BlockStatement> block;
    };

    struct TryCatchStatement : Statement
    {
        Token try_;
        std::unique_ptr<BlockStatement> block;
        std::vector<CatchBlock*> catchBlocks;
        std::optional<std::unique_ptr<FinallyBlock>> finallyBlock;

        TryCatchStatement(Token try_, BlockStatement* block, std::vector<CatchBlock*> catchBlocks,
            std::optional<FinallyBlock*> finallyBlock)
            :
            Statement(NodeKind::TRY_CATCH_STATEMENT), try_(try_), block(block),
            catchBlocks(catchBlocks), finallyBlock(finallyBlock) {};

        virtual ~TryCatchStatement()
        {
            for (auto node: catchBlocks)
                delete node;
        };
    };

    struct RaiseStatement : Statement
    {
        Token raise;
        std::unique_ptr<Expression> exception;

        RaiseStatement(Token raise, Expression* exception)
            : Statement(NodeKind::RAISE_STATEMENT), raise(raise), exception(exception) {};
    };

    struct AssignmentExpression : Expression
    {
        Token id;
        Token assignment;
        std::unique_ptr<Expression> expression;

        AssignmentExpression(Token id, Token assignment, Expression* expression)
            : Expression(NodeKind::ASSIGNMENT_EXPRESSION), id(id), assignment(assignment), expression(expression) {};
    };

    struct BinaryExpression : Expression
    {
        std::unique_ptr<Expression> left;
        Token op;
        std::unique_ptr<Expression> right;

        BinaryExpression(Expression* left, Token op, Expression* right)
            : Expression(NodeKind::BINARY_EXPRESSION), left(left), op(op), right(right) {};
    };

    struct UnaryExpression : Expression
    {
        Token op;
        std::unique_ptr<Expression> operand;

        UnaryExpression(Token op, Expression* operand)
            : Expression(NodeKind::UNARY_EXPRESSION), op(op), operand(operand) {};
    };

    struct ParenthesizedExpression : Expression
    {
        std::unique_ptr<Expression> expression;

        ParenthesizedExpression(Expression* expression)
            : Expression(NodeKind::PARENTHESIZED_EXPRESSION), expression(expression) {};
    };

    struct VariableExpression : Expression
    {
        Token variable;

        VariableExpression(Token variable)
            : Expression(NodeKind::VARIABLE_EXPRESSION), variable(variable) {};
    };

    struct AttributeExpression : Expression
    {
        std::unique_ptr<Expression> expression;
        Token attribute;

        AttributeExpression(Expression* expression, Token attribute)
            : Expression(NodeKind::ATTRIBUTE_EXPRESSION), expression(expression), attribute(attribute) {};
    };

    struct LiteralString
    {
        Token token;
        std::string str;
    };

    struct LiteralExpression : Expression
    {
        enum class Type
        {
            NUMBER, REAL, BOOLEAN, STRING, NULL_
        };

        Token literalToken;
        Type literalType;
        using stringType = std::vector<LiteralString*>;
        std::variant<i64, double, bool, stringType> value;

        LiteralExpression(Token literalToken, Type literalType, std::variant<i64, double, bool, stringType> value)
            : Expression(NodeKind::LITERAL_EXPRESSION), literalToken(literalToken), literalType(literalType), value(value) {};

        ~LiteralExpression()
        {
            if (literalType == Type::STRING)
            {
                for (auto literal : std::get<stringType>(value))
                    delete literal;
            }
        }
    };

    struct CallExpression : Expression
    {
        using arguments_t = std::vector<Expression*>;
        using namedArgument_t = std::pair<Token, Expression*>;
        using namedArguments_t = std::vector<namedArgument_t>;

        std::unique_ptr<Expression> callable;
        arguments_t arguments;
        namedArguments_t namedArguments;

        CallExpression(Expression* callable, arguments_t arguments, namedArguments_t namedArguments)
            : Expression(NodeKind::CALL_EXPRESSION), callable(callable), arguments(arguments), namedArguments(namedArguments) {};

        virtual ~CallExpression()
        {
            for (auto node: arguments)
                delete node;
            for (auto node: namedArguments)
                delete node.second;
        };
    };
}

#endif
