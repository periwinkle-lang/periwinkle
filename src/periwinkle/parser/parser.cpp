#include "parser.h"
#include "node.h"
#include "exception.h"

using namespace parser;
using lexer::TokenType;
using enum lexer::TokenType;
using lexer::Token;

#define CURRENT peek(0)
#define AHEAD peek(1)

int parser::getUnaryOperatorPrecedence(TokenType type)
{
    switch (type)
    {
    case TokenType::PLUS:
    case TokenType::MINUS:
    //case TokenType::NOT:
        return 1;
    default:
        return INT_MAX;
    }
}

int parser::getBinaryOperatorPrecedence(TokenType type)
{
    switch (type)
    {
    case TokenType::STAR:
    case TokenType::SLASH:
    case TokenType::PERCENT:
        return 2;
    case TokenType::PLUS:
    case TokenType::MINUS:
        return 3;
    case TokenType::EQUAL:
    //case TokenType::NOT_EQUAL:
    //case TokenType::LESS:
    /*case TokenType::GREATER:
        return 4;
    case TokenType::AND:
        return 5;
    case TokenType::OR:
        return 6;*/
    default:
        return INT_MAX;
    }
}

BlockStatement* parser::Parser::parseBlock()
{
    std::vector<Expression*> expressions;

    while (CURRENT.tokenType != TokenType::EOF_)
    {
        expressions.push_back((Expression*)parseExpression());
    }

    return new BlockStatement(expressions);
}

Statement* parser::Parser::parseStatement()
{
    switch (CURRENT.tokenType)
    {
    default:
        plog::fatal << "Неможливо обробити токен \""
            << lexer::stringEnum::enumToString(CURRENT.tokenType) << "\"";
    }
}

Expression* parser::Parser::parseAssignmentExpression()
{
    Token variable = matchToken(ID);
    Token assigment = nextToken();
    Expression* expression = parseBinaryExpression();

    return new AssignmentExpression(variable, assigment, expression);
}

Expression* parser::Parser::parseExpression()
{
    switch (CURRENT.tokenType)
    {
    case ID:
        return parseAssignmentOrCallExpression();
    default:
        vm::SyntaxException exception("Неправильний синтаксис", CURRENT.lineno);
        vm::throwSyntaxException(exception, code, CURRENT.position);
        exit(1);
    }
}

Expression* parser::Parser::parseAssignmentOrCallExpression()
{
    if (AHEAD.tokenType == LPAR)
    {
        return parseCallExpression();
    }
    else
    {
        return parseAssignmentExpression();
    }
}

Expression* parser::Parser::parseBinaryExpression(int parentPrecedence)
{
    Expression* left;
    int unaryPrecedence = getUnaryOperatorPrecedence(CURRENT.tokenType);
    if (unaryPrecedence != INT_MAX && unaryPrecedence <= parentPrecedence)
    {
        Token operator_ = nextToken();
        Expression* operand = parseBinaryExpression(unaryPrecedence);
        left = new UnaryExpression(operator_, operand);
    }
    else
    {
        left = parsePrimaryExpression();
    }

    for (;;)
    {
        int precedence = getBinaryOperatorPrecedence(CURRENT.tokenType);
        if (precedence == INT_MAX || precedence >= parentPrecedence)
        {
            break;
        }

        Token operator_ = nextToken();
        Expression* right = parseBinaryExpression(precedence);
        left = new BinaryExpression(left, operator_, right);
    }

    return left;
}

Expression* parser::Parser::parsePrimaryExpression()
{
    switch (CURRENT.tokenType)
    {
    case LPAR:
        return parseParenthesizedExpression();
    case ID:
        return parseVariableOrCallExpression();
    case STRING:
    case NUMBER:
    case BOOLEAN:
    case REAL:
    case NULL_:
        return parseLiteralExpression();
    default:
        vm::SyntaxException exception("Неправильний синтаксис", CURRENT.lineno);
        vm::throwSyntaxException(exception, code, CURRENT.position);
        exit(1);
    }
}

Expression* parser::Parser::parseParenthesizedExpression()
{
    Token left = matchToken(LPAR);
    Expression* expression = parseExpression();
    Token right = matchToken(RPAR);
    return new ParenthesizedExpression(left, expression, right);
}

Expression* parser::Parser::parseVariableOrCallExpression()
{
    if (AHEAD.tokenType == LPAR)
    {
        return parseCallExpression();
    }
    else
    {
        return parseVariableExpression();
    }
}

Expression* parser::Parser::parseVariableExpression()
{
    Token id = matchToken(ID);
    return new VariableExpression(id);
}

Expression* parser::Parser::parseCallExpression()
{
    auto identifier = matchToken(ID);
    auto lpar = matchToken(LPAR);
    auto arguments = parseArguments();
    auto rpar = matchToken(RPAR);

    return new CallExpression(identifier, lpar, arguments, rpar);
}

Expression* parser::Parser::parseLiteralExpression()
{
    Token literalToken = nextToken();
    return new LiteralExpression(literalToken);
}

std::vector<Expression*> parser::Parser::parseArguments()
{
    std::vector<Expression*> arguments;

    while (CURRENT.tokenType != RPAR
        && CURRENT.tokenType != EOF_)
    {
        auto expression = parseBinaryExpression();
        arguments.push_back(expression);

        if (CURRENT.tokenType == COMMA)
        {
            nextToken();
        }
        else
        {
            break;
        }
    }

    return arguments;
}

lexer::Token parser::Parser::nextToken()
{
    auto current = CURRENT;
    position++;
    return current;
}

lexer::Token parser::Parser::peek(int offset)
{
    u64 index = position + offset;
    return index > sizeOfTokensVector ?
        lexer::Token{ TokenType::EOF_, "", 0, 0 } : tokens[index];
}

lexer::Token parser::Parser::matchToken(TokenType type)
{
    if (CURRENT.tokenType == type)
    {
        return nextToken();
    }

    std::cerr << "Неочікуваний токен на позиції: " << position << ". Очікується токен: "
        << lexer::stringEnum::enumToString(type) << ", натомість на цьому місці був токен: ("
        << lexer::stringEnum::enumToString(CURRENT.tokenType) << " " << CURRENT.text << ")" << std::endl;
    exit(1);
}

BlockStatement* parser::Parser::parse()
{
    return parseBlock();
}

parser::Parser::Parser(std::vector<lexer::Token> tokens, std::string code)
    :
    tokens(tokens),
    code(code),
    sizeOfTokensVector(tokens.size()),
    position(0)
{
}
