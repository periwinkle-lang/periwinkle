#include "parser.h"
#include "node.h"
#include "exception.h"
#include "plogger.h"

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
    case TokenType::NOT:
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
    case TokenType::EQUAL_EQUAL:
    case TokenType::NOT_EQUAL:
    case TokenType::LESS:
    case TokenType::GREATER:
    case TokenType::LESS_EQUAL:
    case TokenType::GREATER_EQUAL:
        return 4;
    case TokenType::AND:
        return 5;
    case TokenType::OR:
        return 6;
    default:
        return INT_MAX;
    }
}

BlockStatement* parser::Parser::parseBlock(Node* parent)
{
    auto blockStatement = new BlockStatement(parent);
    while (CURRENT.tokenType != EOF_)
    {
        blockStatement->statements.push_back(parseStatement(blockStatement));
    }
    return blockStatement;
}

Statement* parser::Parser::parseStatement(Node* parent)
{
    switch (CURRENT.tokenType)
    {
    case WHILE:
        return parseWhileStatement(parent);
    case BREAK:
        return parseBreakStatement(parent);
    case CONTINUE:
        return parseContinueStatement(parent);
    case IF:
        return parseIfStatement(parent);
    default:
        return parseExpressionStatement(parent);
    }
}

Statement* parser::Parser::parseExpressionStatement(Node* parent)
{
    auto expressionStatement = new ExpressionStatement(parent);
    expressionStatement->expression = parseExpression(expressionStatement);
    return expressionStatement;
}

Statement* parser::Parser::parseWhileStatement(Node* parent)
{
    auto whileStatement = new WhileStatement(parent);
    whileStatement->keyword = matchToken(WHILE);
    whileStatement->condition = parseRhs(whileStatement);
    auto block = new BlockStatement(whileStatement);
    while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END)
    {
        block->statements.push_back(parseStatement(block));
    }
    whileStatement->block = block;

    matchToken(END);
    return whileStatement;
}

Statement* parser::Parser::parseBreakStatement(Node* parent)
{
    auto breakStatement = new BreakStatement(parent);
    breakStatement->break_ = matchToken(BREAK);
    return breakStatement;
}

Statement* parser::Parser::parseContinueStatement(Node* parent)
{
    auto continueStatement = new ContinueStatement(parent);
    continueStatement->continue_ = matchToken(CONTINUE);
    return continueStatement;
}

Statement* parser::Parser::parseIfStatement(Node* parent, bool elseIf)
{
    auto ifStatement = new IfStatement(parent);
    ifStatement->if_ = matchToken(elseIf ? ELSE_IF : IF);
    ifStatement->condition = parseRhs(ifStatement);
    auto block = new BlockStatement(ifStatement);
    while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END
        && CURRENT.tokenType != ELSE_IF && CURRENT.tokenType != ELSE)
    {
        block->statements.push_back(parseStatement(block));
    }
    ifStatement->block = block;
    ifStatement->elseOrIf = parseElseOrIfStatement(ifStatement);

    if (!ifStatement->elseOrIf)
    {
        matchToken(END);
    }
    return ifStatement;
}

std::optional<Statement*> parser::Parser::parseElseOrIfStatement(Node* parent)
{
    if (CURRENT.tokenType == IF || CURRENT.tokenType == ELSE_IF)
    {
        return parseIfStatement(parent, CURRENT.tokenType == ELSE_IF);
    }
    else if (CURRENT.tokenType == ELSE)
    {
        auto elseStatement = new ElseStatement(parent);
        elseStatement->else_ = matchToken(ELSE);
        auto block = new BlockStatement(elseStatement);
        while (CURRENT.tokenType != EOF_ && CURRENT.tokenType != END)
        {
            block->statements.push_back(parseStatement(block));
        }
        elseStatement->block = block;
        matchToken(END);
        return elseStatement;
    }
    return std::nullopt;
}

Expression* parser::Parser::parseLhs(Node* parent)
{
    return parseVariableExpression(parent);
}

Expression* parser::Parser::parseRhs(Node* parent)
{
    if (isUnaryOperator(CURRENT))
    {
        return parseUnaryExpression(parent);
    }
    else if (isOperator(AHEAD))
    {
       return parseBinaryExpression(parent);
    }
    else if (CURRENT.tokenType == ID)
    {
        if (AHEAD.tokenType == LPAR)
        {
            return parseCallExpression(parent);
        }
        return parseVariableExpression(parent);
    }
    else if (CURRENT.tokenType == LPAR)
    {
        return parseParenthesizedExpression(parent);
    }
    return parseLiteralExpression(parent);
}

Expression* parser::Parser::parseAssignmentExpression(Node* parent)
{
    auto assignmentExpression = new AssignmentExpression(parent);
    assignmentExpression->id = matchToken(ID);
    auto assignment = nextToken();
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
    assignmentExpression->assignment = assignment;
    assignmentExpression->expression = parseRhs(assignmentExpression);
    return assignmentExpression;
}

Expression* parser::Parser::parseExpression(Node* parent)
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
        return parseAssignmentExpression(parent);
    default:
        return parseCallExpression(parent);
    }
}

Expression* parser::Parser::parseUnaryExpression(Node* parent)
{
    auto unaryExpression = new UnaryExpression(parent);
    unaryExpression->operator_ = nextToken();
    unaryExpression->operand = parseRhs(unaryExpression);
    return unaryExpression;
}

Expression* parser::Parser::parseBinaryExpression(Node* parent, int parentPrecedence)
{
    Expression* left;
    int unaryPrecedence = getUnaryOperatorPrecedence(CURRENT.tokenType);
    if (unaryPrecedence != INT_MAX && unaryPrecedence <= parentPrecedence)
    {
        auto unaryExpression = new UnaryExpression(parent);
        unaryExpression->operator_ = nextToken();
        unaryExpression->operand = parseBinaryExpression(unaryExpression, unaryPrecedence);
        left = unaryExpression;
    }
    else
    {
        left = parsePrimaryExpression(parent);
    }

    for (;;)
    {
        int precedence = getBinaryOperatorPrecedence(CURRENT.tokenType);
        if (precedence == INT_MAX || precedence >= parentPrecedence)
        {
            break;
        }

        auto binaryExpression = new BinaryExpression(parent);
        binaryExpression->operator_= nextToken();
        binaryExpression->right = parseBinaryExpression(binaryExpression, precedence);
        binaryExpression->left = left;
        left = binaryExpression;
    }

    return left;

    //auto binaryExpression = new BinaryExpression(parent);
    //binaryExpression->left = parseRhs(binaryExpression);
    //binaryExpression->operator_ = nextToken();
    //binaryExpression->right = parseRhs(binaryExpression);
    //return binaryExpression;
}

Expression* parser::Parser::parsePrimaryExpression(Node* parent)
{
    switch (CURRENT.tokenType)
    {
    case LPAR:
        return parseParenthesizedExpression(parent);
    case ID:
    {
        if (AHEAD.tokenType == LPAR)
        {
            return parseCallExpression(parent);
        }
        return parseVariableExpression(parent);
    }
    case STRING:
    case NUMBER:
    case BOOLEAN:
    case REAL:
    case NULL_:
        return parseLiteralExpression(parent);
    default:
        vm::SyntaxException exception("Неправильний синтаксис", CURRENT.lineno, CURRENT.positionInLine);
        vm::throwSyntaxException(exception, code);
        exit(1);
    }
}

Expression* parser::Parser::parseParenthesizedExpression(Node* parent)
{
    auto parExpression = new ParenthesizedExpression(parent);
    parExpression->lpar = matchToken(LPAR);
    parExpression->expression = parseRhs(parExpression);
    parExpression->rpar = matchToken(RPAR);
    return parExpression;
}

Expression* parser::Parser::parseVariableExpression(Node* parent)
{
    auto varExpression = new VariableExpression(parent);
    varExpression->variable = matchToken(ID);
    return varExpression;
}

Expression* parser::Parser::parseCallExpression(Node* parent)
{
    auto callExpression = new CallExpression(parent);
    callExpression->identifier = matchToken(ID);
    callExpression->lpar = matchToken(LPAR);
    callExpression->arguments = parseArguments(callExpression);
    callExpression->rpar = matchToken(RPAR);
    return callExpression;
}

Expression* parser::Parser::parseLiteralExpression(Node* parent)
{
    auto literalExpression = new LiteralExpression(parent);
    auto literalToken = nextToken();
    switch (literalToken.tokenType)
    {
    case lexer::TokenType::NUMBER:
        literalExpression->value = std::stoll(literalToken.text);
        break;
    case lexer::TokenType::REAL:
        literalExpression->value = std::stod(literalToken.text);
        break;
    case lexer::TokenType::BOOLEAN:
        literalExpression->value = std::string("правда").compare(literalToken.text) == 0;
        break;
    case lexer::TokenType::STRING:
        literalExpression->value = literalToken.text;
        break;
    case lexer::TokenType::NULL_:
        break;
    default:
        plog::fatal << "Переданий неправильний токен: \""
            << lexer::stringEnum::enumToString(literalToken.tokenType) << "\"";
    }
    literalExpression->literalToken = literalToken;
    return literalExpression;
}

std::vector<Expression*> parser::Parser::parseArguments(Node* parent)
{
    std::vector<Expression*> arguments;

    while (CURRENT.tokenType != RPAR
        && CURRENT.tokenType != EOF_)
    {
        auto expression = parseRhs(parent);
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

bool parser::Parser::isOperator(Token token)
{
    switch (token.tokenType)
    {
    case GREATER_EQUAL:
    case LESS_EQUAL:
    case EQUAL_EQUAL:
    case NOT_EQUAL:
    case PLUS:
    case MINUS:
    case SLASH:
    case STAR:
    case PERCENT:
    case BACKSLASH:
    case GREATER:
    case LESS:
    case AND:
    case OR:
    case NOT:
    case EQUAL:
        return true;
    default:
        return false;
    }
}

bool parser::Parser::isUnaryOperator(Token token)
{
    switch (token.tokenType)
    {
    case PLUS:
    case MINUS:
    case NOT:
        return true;
    default:
        return false;
    }
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
    return parseBlock(nullptr);
}

parser::Parser::Parser(std::vector<lexer::Token> tokens, std::string code)
    :
    tokens(tokens),
    code(code),
    sizeOfTokensVector(tokens.size()),
    position(0)
{
}
