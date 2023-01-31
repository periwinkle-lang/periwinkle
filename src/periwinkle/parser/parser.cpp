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
    case TokenType::BACKSLASH:
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
    std::vector<Statement*> statements;

    while (CURRENT.tokenType != EOF_)
    {
        statements.push_back(parseStatement());
    }
    return new BlockStatement(statements);
}

Statement* parser::Parser::parseStatement()
{
    switch (CURRENT.tokenType)
    {
    case WHILE:
        return parseWhileStatement();
    case BREAK:
        return parseBreakStatement();
    case CONTINUE:
        return parseContinueStatement();
    case IF:
        return parseIfStatement();
    default:
        return parseExpressionStatement();
    }
}

Statement* parser::Parser::parseExpressionStatement()
{
    Expression* expression = parseExpression();
    return new ExpressionStatement(expression);
}

Statement* parser::Parser::parseWhileStatement()
{
    Token keyword = matchToken(WHILE);
    Expression* condition = parseExpression();
    std::vector<Statement*> statements;
    while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END)
    {
        statements.push_back(parseStatement());
    }
    BlockStatement* block = new BlockStatement(statements);
    matchToken(END);

    return new WhileStatement(keyword, condition, block);
}

Statement* parser::Parser::parseBreakStatement()
{
    Token break_ = matchToken(BREAK);
    return new BreakStatement(break_);
}

Statement* parser::Parser::parseContinueStatement()
{
    Token continue_ = matchToken(CONTINUE);
    return new ContinueStatement(continue_);
}

Statement* parser::Parser::parseIfStatement(bool elseIf)
{
    Token keyword = matchToken(elseIf ? ELSE_IF : IF);
    Expression* condition = parseExpression();
    std::vector<Statement*> statements;
    while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END
        && CURRENT.tokenType != ELSE_IF && CURRENT.tokenType != ELSE)
    {
        statements.push_back(parseStatement());
    }
    BlockStatement* block = new BlockStatement(statements);
    auto elseOrIf = parseElseOrIfStatement();

    if (!elseOrIf)
    {
        matchToken(END);
    }

    return new IfStatement(keyword, condition, block, elseOrIf);
}

std::optional<Statement*> parser::Parser::parseElseOrIfStatement()
{
    if (CURRENT.tokenType == IF || CURRENT.tokenType == ELSE_IF)
    {
        return parseIfStatement(CURRENT.tokenType == ELSE_IF);
    }
    else if (CURRENT.tokenType == ELSE)
    {
        Token keyword = matchToken(ELSE);
        std::vector<Statement*> statements;
        while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END)
        {
            statements.push_back(parseStatement());
        }
        BlockStatement* block = new BlockStatement(statements);
        matchToken(END);

        return new ElseStatement(keyword, block);
    }
    return std::nullopt;
}

Expression* parser::Parser::parseAssignmentExpression()
{
    Token variable = matchToken(ID);
    Token assignment = nextToken();
    Expression* expression = parseExpression();

    return new AssignmentExpression(variable, assignment, expression);
}

Expression* parser::Parser::parseExpression()
{
    switch (AHEAD.tokenType)
    {
    case EQUAL:
    case PLUS_EQUAL:
    case MINUS_EQUAL:
    case STAR_EQUAL:
    case SLASH_EQUAL:
    case PERCENT_EQUAL:
    case BACKSLASH_EQUAL:
        return parseAssignmentExpression();
    default:
        return parseBinaryExpression();
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
        vm::SyntaxException exception("Неправильний синтаксис", CURRENT.lineno, CURRENT.positionInLine);
        vm::throwSyntaxException(exception, code);
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
        auto expression = parseExpression();
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
    return index >= sizeOfTokensVector ?
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
