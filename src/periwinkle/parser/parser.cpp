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

BlockStatement* parser::Parser::parseBlock()
{
    auto blockStatement = new BlockStatement();
    while (auto statement = parseStatement())
    {
        blockStatement->statements.push_back(statement);
    }
    return blockStatement;
}

Statement* parser::Parser::parseStatement()
{
    SIMPLE_RULE(parseWhileStatement);
    SIMPLE_RULE(parseForEachStatement);
    SIMPLE_RULE(parseBreakStatement);
    SIMPLE_RULE(parseContinueStatement);
    SIMPLE_RULE(parseIfStatement);
    SIMPLE_RULE(parseFunctionDeclaration);
    SIMPLE_RULE(parseReturnStatement);
    SIMPLE_RULE(parseExpressionStatement);
    return nullptr;
}

Statement* parser::Parser::parseExpressionStatement()
{
    auto expressionStatement = new ExpressionStatement();
    if ((expressionStatement->expression = parseExpression()))
        return expressionStatement;

    delete expressionStatement;
    return nullptr;
}

Statement* parser::Parser::parseWhileStatement()
{
    auto mark = position;
    auto whileStatement = new WhileStatement();
    if (auto keyword = matchToken(WHILE))
    {
        whileStatement->keyword = keyword.value();
        if ((whileStatement->condition = parseRhs()))
        {
            auto block = new BlockStatement();
            while (!matchToken(END))
            {
                if (auto statement = parseStatement())
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

Statement* parser::Parser::parseBreakStatement()
{
    if (auto keyword = matchToken(BREAK))
    {
        auto breakStatement = new BreakStatement();
        breakStatement->break_ = keyword.value();
        return breakStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseContinueStatement()
{
    if (auto keyword = matchToken(CONTINUE))
    {
        auto continueStatement = new ContinueStatement();
        continueStatement->continue_ = keyword.value();
        return continueStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseIfStatement(bool elseIf)
{
    auto mark = position;
    auto ifStatement = new IfStatement();
    if (auto keyword = matchToken(elseIf ? ELSE_IF : IF))
    {
        ifStatement->if_ = keyword.value();
        if ((ifStatement->condition = parseRhs()))
        {
            auto block = new BlockStatement();
            while (!matchToken(END))
            {
                if (auto elseOrIf = parseElseOrIfStatement())
                {
                    ifStatement->elseOrIf = elseOrIf;
                    break;
                }
                else
                {
                    if (auto statement = parseStatement())
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

Statement* parser::Parser::parseElseOrIfStatement()
{
    if (peekToken().tokenType == IF || peekToken().tokenType == ELSE_IF)
    {
        if (auto statement = parseIfStatement(peekToken().tokenType == ELSE_IF))
        {
            return statement;
        }
    }

    if (auto elseStatement = parseElseStatement())
    {
        return elseStatement;
    }

    return nullptr;
}

Statement* parser::Parser::parseElseStatement()
{
    auto mark = position;
    auto elseStatement = new ElseStatement();
    if (auto keyword = matchToken(ELSE))
    {
        elseStatement->else_ = keyword.value();
        auto block = new BlockStatement();
        while (!matchToken(END))
        {
            if (auto statement = parseStatement())
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

Statement* parser::Parser::parseFunctionDeclaration()
{
    auto mark = position;
    auto fnDeclaration = new FunctionDeclaration();
    if (auto keyword = matchToken(FUNCTION))
    {
        fnDeclaration->keyword = keyword.value();
        if (auto identifier = matchToken(ID))
        {
            fnDeclaration->id = identifier.value();
            if (auto lpar = matchToken(LPAR))
            {
                fnDeclaration->lpar = lpar.value();
                auto [parameters, variadicParameter] = parseParameters();
                fnDeclaration->parameters = parameters;
                fnDeclaration->variadicParameter = variadicParameter;
                if (auto rpar = matchToken(RPAR))
                {
                    fnDeclaration->rpar = rpar.value();
                    auto block = new BlockStatement();
                    while (!matchToken(END))
                    {
                        if (auto statement = parseStatement())
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

std::pair<
    std::vector<lexer::Token>,
    std::optional<lexer::Token> > parser::Parser::parseParameters()
{
    std::vector<Token> parameters;
    std::optional<lexer::Token> variadicParameter = std::nullopt;

    while (peekToken().tokenType != RPAR)
    {
        if (variadicParameter = parseVariadicParameter())
        {
            if (peekToken().tokenType != RPAR)
            {
                throwParserError(
                    "Варіативний параметр має стояти в кінці списку параметрів функції",
                    peekToken());
            }
            break;
        }

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

    return {parameters, variadicParameter};
}

std::optional<lexer::Token> parser::Parser::parseVariadicParameter()
{
    auto mark = position;
    if (auto identifier = matchToken(ID))
    {
        if (matchToken(ELLIPSIS))
        {
            return std::optional(identifier);
        }
    }

    position = mark;
    return std::nullopt;
}

Statement* parser::Parser::parseReturnStatement()
{
    auto mark = position;
    auto returnStatement = new ReturnStatement();
    if (auto keyword = matchToken(RETURN))
    {
        returnStatement->return_ = keyword.value();
        if (matchToken(SEMICOLON))
        {
            returnStatement->returnValue = std::nullopt;
            return returnStatement;
        }

        auto returnValue = parseRhs();
        returnStatement->returnValue = returnValue == nullptr ?
            std::nullopt : std::optional(returnValue);
        return returnStatement;
    }

    position = mark;
    delete returnStatement;
    return nullptr;
}

Statement* parser::Parser::parseForEachStatement()
{
    auto mark = position;
    auto forEachStatement = new ForEachStatement();
    if (auto keyword = matchToken(EACH))
    {
        forEachStatement->forEach = keyword.value();
        if (auto variable = matchToken(ID))
        {
            forEachStatement->variable = variable.value();
            if (auto eachFrom = matchToken(EACH_FROM))
            {
                forEachStatement->eachFrom = eachFrom.value();
                if ((forEachStatement->expression = parseRhs()))
                {
                    auto block = new BlockStatement();
                    while (!matchToken(END))
                    {
                        if (auto statement = parseStatement())
                        {
                            block->statements.push_back(statement);
                        }
                        else
                        {
                            throwParserError(
                                "Інструкція \"кожній\" повинна закінчуватись на"
                                "ключове слово \"кінець\"",
                                forEachStatement->forEach);
                        }
                    }
                    forEachStatement->block = block;
                    return forEachStatement;
                }
            }
        }
    }

    position = mark;
    delete forEachStatement;
    return nullptr;
}

Expression* parser::Parser::parseLhs()
{
    SIMPLE_RULE(parseVariableExpression);
    return nullptr;
}

Expression* parser::Parser::parseRhs()
{
    SIMPLE_RULE(parseOperator7);
    return nullptr;
}

Expression* parser::Parser::_parseOperator7()
{
    auto mark = position;
    auto binary = new BinaryExpression();
    if ((binary->left = parseOperator7()))
    {
        if (auto op = matchToken(OR))
        {
            binary->operator_ = op.value();
            if ((binary->right = parseOperator6()))
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

    if (auto node = parseOperator6())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator6()
{
    auto mark = position;
    auto binary = new BinaryExpression();
    if ((binary->left = parseOperator6()))
    {
        if (auto op = matchToken(AND))
        {
            binary->operator_ = op.value();
            if ((binary->right = parseOperator5()))
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

    if (auto node = parseOperator5())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator5()
{
    if (auto op = matchToken(NOT))
    {
        auto unary = new UnaryExpression();
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator5()))
        {
            return unary;
        }
        else
        {
            throwParserError("Після оператора \"не\" очікується вираз", unary->operator_);
        }
    }

    if (auto node = parseOperator4())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator4()
{
    auto mark = position;
    auto binary = new BinaryExpression();
    if ((binary->left = parseOperator4()))
    {
        switch (peekToken().tokenType)
        {
        case IS:
        case LESS:
        case LESS_EQUAL:
        case GREATER:
        case GREATER_EQUAL:
        case EQUAL_EQUAL:
        case NOT_EQUAL:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator3()))
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

    if (auto node = parseOperator3())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator3()
{
    auto mark = position;
    auto binary = new BinaryExpression();
    if ((binary->left = parseOperator3()))
    {
        switch (peekToken().tokenType)
        {
        case PLUS:
        case MINUS:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator2()))
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

    if (auto node = parseOperator2())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::_parseOperator2()
{
    auto mark = position;
    auto binary = new BinaryExpression();
    if ((binary->left = parseOperator2()))
    {
        switch (peekToken().tokenType)
        {
        case STAR:
        case SLASH:
        case PERCENT:
        case BACKSLASH:
        {
            binary->operator_ = nextToken();
            if ((binary->right = parseOperator1()))
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

    if (auto node = parseOperator1())
    {
        return node;
    }

    return nullptr;
}

Expression* parser::Parser::parseOperator1()
{
    if (auto op = matchToken(PLUS))
    {
        auto unary = new UnaryExpression();
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator1()))
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
        auto unary = new UnaryExpression();
        unary->operator_ = op.value();
        if ((unary->operand = parseOperator1()))
        {
            return unary;
        }
        else
        {
            throwParserError("Після оператора \"-\" очікується вираз", op.value());
        }
    }

    if (auto primary = parsePrimaryExpression())
    {
        return primary;
    }

    return nullptr;
}

Expression* parser::Parser::parseAssignmentExpression()
{
    auto mark = position;
    auto assignmentExpression = new AssignmentExpression();
    if (auto identifier = matchToken(ID))
    {
        assignmentExpression->id = identifier.value();
        if (auto assignmentOperator = parseAssignmentOperator())
        {
            assignmentExpression->assignment = assignmentOperator.value();
            if ((assignmentExpression->expression = parseRhs()))
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

Expression* parser::Parser::parseExpression()
{
    SIMPLE_RULE(parseAssignmentExpression);
    SIMPLE_RULE(parseRhs);
    return nullptr;
}

Expression* parser::Parser::_parsePrimaryExpression()
{
    SIMPLE_RULE(parseAttributeExpression);
    SIMPLE_RULE(parseCallExpression);
    SIMPLE_RULE(parseVariableExpression);
    SIMPLE_RULE(parseLiteralExpression);
    SIMPLE_RULE(parseParenthesizedExpression);
    return nullptr;
}

Expression* parser::Parser::parseParenthesizedExpression()
{
    auto mark = position;
    auto parExpression = new ParenthesizedExpression();
    if (auto lpar = matchToken(LPAR))
    {
        parExpression->lpar = lpar.value();
        if ((parExpression->expression = parseRhs()))
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

Expression* parser::Parser::parseAttributeExpression()
{
    auto mark = position;
    auto attrExpression = new AttributeExpression();
    if ((attrExpression->expression = parsePrimaryExpression()))
    {
        if (matchToken(DOT))
        {
            if (auto attribute = matchToken(ID))
            {
                attrExpression->attribute = attribute.value();
                return attrExpression;
            }
        }
    }

    position = mark;
    delete attrExpression;
    return nullptr;
}

Expression* parser::Parser::parseVariableExpression()
{
    auto varExpression = new VariableExpression();
    if (auto identifier = matchToken(ID))
    {
        varExpression->variable = identifier.value();
        return varExpression;
    }

    delete varExpression;
    return nullptr;
}

Expression* parser::Parser::parseCallExpression()
{
    auto mark = position;
    auto callExpression = new CallExpression();
    if ((callExpression->callable = parsePrimaryExpression()))
    {
        if (auto lpar = matchToken(LPAR))
        {
            callExpression->lpar = lpar.value();
            callExpression->arguments = parseArguments();

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

Expression* parser::Parser::parseLiteralExpression()
{
    auto literalExpression = new LiteralExpression();

    if (auto number = matchToken(NUMBER))
    {
        literalExpression->literalToken = number.value();
        try
        {
            literalExpression->value = std::stoll(number.value().text);
        }
        catch (const std::out_of_range& e)
        {
            throwParserError(
                "Число не входить в діапазон можливих значень числа",
                literalExpression->literalToken);
        }
        return literalExpression;
    }

    if (auto real = matchToken(REAL))
    {
        literalExpression->literalToken = real.value();
        try
        {
            literalExpression->value = std::stod(real.value().text);
        }
        catch (const std::out_of_range& e)
        {
            throwParserError(
                "Число не входить в діапазон можливих значень дійсних чисел",
                literalExpression->literalToken);
        }
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

std::vector<Expression*> parser::Parser::parseArguments()
{
    std::vector<Expression*> arguments;

    while (peekToken().tokenType != RPAR)
    {
        if (auto rhs = parseRhs())
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
    // Пропуск шебанг стрічки
    matchToken(SHEBANG);

    auto block = parseBlock();
    if (matchToken(EOF_))
    {
        return block;
    }
    throwParserError("Неправильний синтаксис", peekToken());
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
    parsePrimaryExpression = makeLeftRecRule(&Parser::_parsePrimaryExpression, this);
}
