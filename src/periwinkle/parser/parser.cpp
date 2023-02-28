#include "parser.h"
#include "node.h"
#include "exception.h"
#include "plogger.h"

using namespace parser;
using lexer::TokenType;
using enum lexer::TokenType;
using lexer::Token;

#define SIMPLE_RULE(rule, ...)             \
    if (auto __##rule = rule(__VA_ARGS__)) \
        return __##rule;


template <typename R, typename... Args>
struct parser::Parser::LeftRecursionDecorator<R(Args...)>
{
    std::function<R(Parser*, Args...)> func;
    Parser* parser;

    LeftRecursionDecorator(std::function<R(Parser*, Args...)> func) : func(func) {}

    R operator()(Args... args)
    {
        auto mark = parser->position;
        void* key = (void*)&func;

        if (parser->memo[mark].contains(key))
        {
            auto [result, endPosition] = parser->memo[mark][key];
            parser->position = endPosition;
            return (R)result;
        }
        else
        {
            parser->memo[mark][key] = { nullptr, mark };
            Node* lastResult = nullptr;
            auto lastPosition = mark;
            u64 endPosition;

            while (true)
            {
                parser->position = mark;
                Node* result = func(parser, args...);
                endPosition = parser->position;
                if (endPosition <= lastPosition)
                    break;
                parser->memo[mark][key] = { result, endPosition };
                lastResult = result;
                lastPosition = endPosition;
            }

            parser->position = lastPosition;
            return (R)lastResult;
        }
    }
};

BlockStatement* parser::Parser::parseBlock(Node* parent)
{
    auto blockStatement = new BlockStatement(parent);
    while (auto statement = parseStatement(blockStatement))
    {
        blockStatement->statements.push_back(statement);
    }
    return blockStatement;
}

Statement* parser::Parser::parseStatement(Node* parent)
{
    SIMPLE_RULE(parseWhileStatement, parent);
    SIMPLE_RULE(parseBreakStatement, parent);
    SIMPLE_RULE(parseContinueStatement, parent);
    SIMPLE_RULE(parseIfStatement, parent);
    SIMPLE_RULE(parseFunctionDeclaration, parent);
    SIMPLE_RULE(parseReturnStatement, parent);
    SIMPLE_RULE(parseExpressionStatement, parent);
    return nullptr;
}

Statement* parser::Parser::parseExpressionStatement(Node* parent)
{
    auto expressionStatement = new ExpressionStatement(parent);
    if ((expressionStatement->expression = parseExpression(expressionStatement)))
        return expressionStatement;

    delete expressionStatement;
    return nullptr;
}

Statement* parser::Parser::parseWhileStatement(Node* parent)
{
    auto mark = position;
    auto whileStatement = new WhileStatement(parent);
    if (auto keyword = matchToken(WHILE))
    {
        whileStatement->keyword = keyword.value();
        if ((whileStatement->condition = parseRhs(whileStatement)))
        {
            auto block = new BlockStatement(whileStatement);
            while (!matchToken(END))
            {
                if (auto statement = parseStatement(block))
                {
                    block->statements.push_back(statement);
                }
                else
                {
                    throwParserError(
                        "Інструкція \"поки\" повинна закінчуватись на ключове слово \"кінець\"",
                        whileStatement->keyword);
                }
            }
            whileStatement->block = block;
            return whileStatement;
        }
    }

    position = mark;
    delete whileStatement;
    return nullptr;
}

Statement* parser::Parser::parseBreakStatement(Node* parent)
{
    if (auto keyword = matchToken(BREAK))
    {
        auto breakStatement = new BreakStatement(parent);
        breakStatement->break_ = keyword.value();
        return breakStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseContinueStatement(Node* parent)
{
    if (auto keyword = matchToken(CONTINUE))
    {
        auto continueStatement = new ContinueStatement(parent);
        continueStatement->continue_ = keyword.value();
        return continueStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseIfStatement(Node* parent, bool elseIf)
{
    auto mark = position;
    auto ifStatement = new IfStatement(parent);
    if (auto keyword = matchToken(elseIf ? ELSE_IF : IF))
    {
        ifStatement->if_ = keyword.value();
        if ((ifStatement->condition = parseRhs(ifStatement)))
        {
            auto block = new BlockStatement(ifStatement);
            while (!matchToken(END))
            {
                if (auto elseOrIf = parseElseOrIfStatement(ifStatement))
                {
                    ifStatement->elseOrIf = elseOrIf;
                    break;
                }
                else
                {
                    if (auto statement = parseStatement(block))
                    {
                        block->statements.push_back(statement);
                    }
                    else
                    {
                        throwParserError("Неправильний синтаксис", peekToken());
                    }
                }
            }
            ifStatement->block = block;
            return ifStatement;
        }
    }

    position = mark;
    delete ifStatement;
    return nullptr;
}

Statement* parser::Parser::parseElseOrIfStatement(Node* parent)
{
    if (peekToken().tokenType == IF || peekToken().tokenType == ELSE_IF)
    {
        if (auto statement = parseIfStatement(parent, peekToken().tokenType == ELSE_IF))
        {
            return statement;
        }
    }

    if (auto elseStatement = parseElseStatement(parent))
    {
        return elseStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseElseStatement(Node* parent)
{
    auto mark = position;
    auto elseStatement = new ElseStatement(parent);
    if (auto keyword = matchToken(ELSE))
    {
        elseStatement->else_ = keyword.value();
        auto block = new BlockStatement(elseStatement);
        while (!matchToken(END))
        {
            if (auto statement = parseStatement(block))
            {
                block->statements.push_back(statement);
            }
            else
            {
                throwParserError(
                    "Інструкція \"інакше\" повинна закінчуватись на ключове слово \"кінець\"",
                    elseStatement->else_);
            }
        }
        elseStatement->block = block;
        return elseStatement;
    }

    position = mark;
    delete elseStatement;
    return nullptr;
}

Statement* parser::Parser::parseFunctionDeclaration(Node* parent)
{
    auto mark = position;
    auto fnDeclaration = new FunctionDeclaration(parent);
    if (auto keyword = matchToken(FUNCTION))
    {
        fnDeclaration->keyword = keyword.value();
        if (auto identifier = matchToken(ID))
        {
            fnDeclaration->id = identifier.value();
            if (auto lpar = matchToken(LPAR))
            {
                fnDeclaration->lpar = lpar.value();
                fnDeclaration->parameters = parseParameters(fnDeclaration);
                if (auto rpar = matchToken(RPAR))
                {
                    fnDeclaration->rpar = rpar.value();
                    auto block = new BlockStatement(fnDeclaration);
                    while (!matchToken(END))
                    {
                        if (auto statement = parseStatement(block))
                        {
                            block->statements.push_back(statement);
                        }
                        else
                        {
                            throwParserError(
                                "Інструкція \"функція\" повинна закінчуватись на ключове"
                                "слово \"кінець\"",
                                fnDeclaration->keyword);
                        }
                    }
                    fnDeclaration->block = block;
                    return fnDeclaration;
                }
                else
                {
                    throwParserError("Відсутня закриваюча дужка", fnDeclaration->keyword);
                }
            }
        }
    }

    position = mark;
    delete fnDeclaration;
    return nullptr;
}

std::vector<Token> parser::Parser::parseParameters(Node* parent)
{
    std::vector<Token> parameters;

    while (peekToken().tokenType != RPAR)
    {
        if (auto identifier = matchToken(ID))
        {
            parameters.push_back(identifier.value());
        }
        else
        {
            break;
        }

        if (!matchToken(COMMA))
        {
            break;
        }
    }

    return parameters;
}

Statement* parser::Parser::parseReturnStatement(Node* parent)
{
    auto mark = position;
    auto returnStatement = new ReturnStatement(parent);
    if (auto keyword = matchToken(RETURN))
    {
        returnStatement->return_ = keyword.value();
        if (matchToken(SEMICOLON))
        {
            returnStatement->returnValue = std::nullopt;
            return returnStatement;
        }

        auto returnValue = parseRhs(returnStatement);
        returnStatement->returnValue = returnValue == nullptr ?
            std::nullopt : std::optional(returnValue);
        return returnStatement;
    }

    position = mark;
    delete returnStatement;
    return nullptr;
}

Expression* parser::Parser::parseLhs(Node* parent)
{
    SIMPLE_RULE(parseVariableExpression, parent);
    return nullptr;
}

Expression* parser::Parser::parseRhs(Node* parent)
{
    SIMPLE_RULE(parseOperator7, parent);
    return nullptr;
}

Expression* parser::Parser::_parseOperator7(Node* parent)
{
    auto mark = position;
    auto binary = new BinaryExpression(parent);
    if ((binary->left = parseOperator7(binary)))
    {
        if (auto op = matchToken(OR))
        {
            binary->operator_ = op.value();
            if ((binary->right = parseOperator6(binary)))
            {
                return binary;
            }
            else
            {
                throwParserError("Неправильний синтаксис", peekToken());
            }
        }
    }
    position = mark;
    delete binary;

    if (auto node = parseOperator6(parent))
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator6(Node* parent)
{
    auto mark = position;
    auto binary = new BinaryExpression(parent);
    if ((binary->left = parseOperator6(binary)))
    {
        if (auto op = matchToken(AND))
        {
            binary->operator_ = op.value();
            if ((binary->right = parseOperator5(binary)))
            {
                return binary;
            }
            else
            {
                throwParserError("Неправильний синтаксис", peekToken());
            }
        }
    }
    position = mark;
    delete binary;

    if (auto node = parseOperator5(parent))
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator5(Node* parent)
{
    if (auto op = matchToken(NOT))
    {
        auto unary = new UnaryExpression(parent);
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator5(unary)))
        {
            return unary;
        }
        else
        {
            throwParserError("Після оператора \"не\" очікується вираз", unary->operator_);
        }
    }

    if (auto node = parseOperator4(parent))
    {
        return node;
    }

    return nullptr;
}


Expression* parser::Parser::_parseOperator4(Node* parent)
{
    auto mark = position;
    auto binary = new BinaryExpression(parent);
    if ((binary->left = parseOperator4(binary)))
    {
        switch (peekToken().tokenType)
        {
        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL:
        case EQUAL_EQUAL:
        case NOT_EQUAL:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator3(binary)))
            {
                return binary;
            }
            else
            {
                throwParserError("Неправильний синтаксис", binary->operator_);
            }
        }
        default:
            break;
        }
    }
    position = mark;
    delete binary;

    if (auto node = parseOperator3(parent))
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator3(Node* parent)
{
    auto mark = position;
    auto binary = new BinaryExpression(parent);
    if ((binary->left = parseOperator3(binary)))
    {
        switch (peekToken().tokenType)
        {
        case PLUS:
        case MINUS:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator2(binary)))
            {
                return binary;
            }
            else
            {
                throwParserError("Неправильний синтаксис", binary->operator_);
            }
        }
        default:
            break;
        }
    }
    position = mark;
    delete binary;

    if (auto node = parseOperator2(parent))
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator2(Node* parent)
{
    auto mark = position;
    auto binary = new BinaryExpression(parent);
    if ((binary->left = parseOperator2(binary)))
    {
        switch (peekToken().tokenType)
        {
        case STAR:
        case SLASH:
        case PERCENT:
        case BACKSLASH:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator1(binary)))
            {
                return binary;
            }
            else
            {
                throwParserError("Неправильний синтаксис", binary->operator_);
            }
        }
        default:
            break;
        }
    }
    position = mark;
    delete binary;

    if (auto node = parseOperator1(parent))
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::parseOperator1(Node* parent)
{
    if (auto op = matchToken(PLUS))
    {
        auto unary = new UnaryExpression(parent);
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator1(unary)))
        {
            return unary;
        }
        else
        {
            throwParserError("Після оператора \"+\" очікується вираз", op.value());
        }
    }

    if (auto op = matchToken(MINUS))
    {
        auto unary = new UnaryExpression(parent);
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator1(unary)))
        {
            return unary;
        }
        else
        {
            throwParserError("Після оператора \"-\" очікується вираз", op.value());
        }
    }

    if (auto primary = parsePrimaryExpression(parent))
    {
        return primary;
    }

    return nullptr;
}

Expression* parser::Parser::parseAssignmentExpression(Node* parent)
{
    auto mark = position;
    auto assignmentExpression = new AssignmentExpression(parent);
    if (auto identifier = matchToken(ID))
    {
        assignmentExpression->id = identifier.value();
        if (auto assignmentOperator = parseAssignmentOperator())
        {
            assignmentExpression->assignment = assignmentOperator.value();
            if ((assignmentExpression->expression = parseRhs(assignmentExpression)))
            {
                return assignmentExpression;
            }
            else
            {
                throwParserError("Неможливо присвоїти змінній такий вираз",
                    assignmentExpression->assignment);
            }
        }
    }

    position = mark;
    delete assignmentExpression;
    return nullptr;
}

Expression* parser::Parser::parseExpression(Node* parent)
{
    SIMPLE_RULE(parseAssignmentExpression, parent);
    SIMPLE_RULE(parseCallExpression, parent);
    return nullptr;
}

Expression* parser::Parser::parsePrimaryExpression(Node* parent)
{
    SIMPLE_RULE(parseCallExpression, parent);
    SIMPLE_RULE(parseVariableExpression, parent);
    SIMPLE_RULE(parseLiteralExpression, parent);
    SIMPLE_RULE(parseParenthesizedExpression, parent);
    return nullptr;
}

Expression* parser::Parser::parseParenthesizedExpression(Node* parent)
{
    auto mark = position;
    auto parExpression = new ParenthesizedExpression(parent);
    if (auto lpar = matchToken(LPAR))
    {
        parExpression->lpar = lpar.value();
        if ((parExpression->expression = parseRhs(parExpression)))
        {
            if (auto rpar = matchToken(RPAR))
            {
                parExpression->rpar = rpar.value();
                return parExpression;
            }
        }
    }

    position = mark;
    delete parExpression;
    return nullptr;
}

Expression* parser::Parser::parseVariableExpression(Node* parent)
{
    auto varExpression = new VariableExpression(parent);
    if (auto identifier = matchToken(ID))
    {
        varExpression->variable = identifier.value();
        return varExpression;
    }

    delete varExpression;
    return nullptr;
}

Expression* parser::Parser::parseCallExpression(Node* parent)
{
    auto mark = position;
    auto callExpression = new CallExpression(parent);
    if (auto identifier = matchToken(ID))
    {
        callExpression->identifier = identifier.value();
        if (auto lpar = matchToken(LPAR))
        {
            callExpression->lpar = lpar.value();
            callExpression->arguments = parseArguments(callExpression);

            if (auto rpar = matchToken(RPAR))
            {
                callExpression->rpar = rpar.value();
                return callExpression;
            }
        }
    }

    position = mark;
    delete callExpression;
    return nullptr;
}

Expression* parser::Parser::parseLiteralExpression(Node* parent)
{
    auto literalExpression = new LiteralExpression(parent);

    if (auto number = matchToken(NUMBER))
    {
        literalExpression->literalToken = number.value();
        literalExpression->value = std::stoll(number.value().text);
        return literalExpression;
    }

    if (auto real = matchToken(REAL))
    {
        literalExpression->literalToken = real.value();
        literalExpression->value = std::stod(real.value().text);
        return literalExpression;
    }

    if (auto boolean = matchToken(BOOLEAN))
    {
        literalExpression->literalToken = boolean.value();
        literalExpression->value = std::string("істина").compare(boolean.value().text) == 0;
        return literalExpression;
    }

    if (auto str = matchToken(STRING))
    {
        literalExpression->literalToken = str.value();
        literalExpression->value = str.value().text;
        return literalExpression;
    }

    if (auto null_ = matchToken(NULL_))
    {
        literalExpression->literalToken = null_.value();
        return literalExpression;
    }

    return nullptr;
}

std::vector<Expression*> parser::Parser::parseArguments(Node* parent)
{
    std::vector<Expression*> arguments;

    while (peekToken().tokenType != RPAR)
    {
        if (auto rhs = parseRhs(parent))
        {
            arguments.push_back(rhs);
        }
        else
        {
            break;
        }

        if (!matchToken(COMMA))
        {
            break;
        }
    }

    return arguments;
}

std::optional<Token> parser::Parser::parseAssignmentOperator()
{
    switch (peekToken().tokenType)
    {
    case EQUAL:
    case PLUS_EQUAL:
    case MINUS_EQUAL:
    case STAR_EQUAL:
    case SLASH_EQUAL:
    case BACKSLASH_EQUAL:
    case PERCENT_EQUAL:
        return nextToken();
    default:
        return std::nullopt;
    }
}

std::optional<Token> parser::Parser::parseUnaryOperator()
{
    switch (peekToken().tokenType)
    {
    case PLUS:
    case MINUS:
    case NOT:
        return nextToken();
    default:
        return std::nullopt;
    }
}



lexer::Token parser::Parser::nextToken()
{
    auto current = peekToken();
    position++;
    return current;
}

lexer::Token parser::Parser::peekToken()
{
    return position >= sizeOfTokensVector ?
        Token{ TokenType::EOF_, "", 0, 0 } : tokens[position];
}

std::optional<Token> parser::Parser::matchToken(TokenType type)
{
    if (peekToken().tokenType == type)
    {
        return nextToken();
    }

    return std::nullopt;
}

void parser::Parser::throwParserError(std::string message, Token token)
{
    vm::SyntaxException exception(message, token.lineno, token.positionInLine);
    vm::throwSyntaxException(exception, code);
    exit(1);
}

BlockStatement* parser::Parser::parse()
{
    return parseBlock(nullptr);
}

parser::Parser::Parser(std::vector<Token> tokens, std::string code)
    :
    tokens(tokens),
    code(code),
    sizeOfTokensVector(tokens.size()),
    position(0)
{
    parseOperator7 = makeLeftRecRule(&Parser::_parseOperator7, this);
    parseOperator6 = makeLeftRecRule(&Parser::_parseOperator6, this);
    parseOperator5 = makeLeftRecRule(&Parser::_parseOperator5, this);
    parseOperator4 = makeLeftRecRule(&Parser::_parseOperator4, this);
    parseOperator3 = makeLeftRecRule(&Parser::_parseOperator3, this);
    parseOperator2 = makeLeftRecRule(&Parser::_parseOperator2, this);
}
