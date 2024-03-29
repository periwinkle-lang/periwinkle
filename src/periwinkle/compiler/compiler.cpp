﻿#include <algorithm>
#include <variant>

#include "compiler.h"
#include "lexer.h"
#include "node_kind.h"
#include "code_object.h"
#include "bool_object.h"
#include "int_object.h"
#include "string_object.h"
#include "real_object.h"
#include "null_object.h"
#include "string_vector_object.h"
#include "types.h"
#include "builtins.h"
#include "plogger.h"
#include "exception.h"

using namespace compiler;
using namespace parser;
using enum vm::OpCode;
using enum parser::NodeKind;

#define FIND_CONST_IDX(OBJECT, OBJECT_TYPE)                               \
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i) \
    {                                                                     \
        auto constant = codeObject->constants[i];                         \
        if (OBJECT_IS(constant, &vm::OBJECT_TYPE) == false)               \
        {                                                                 \
            continue;                                                     \
        }                                                                 \
        else if (((vm::OBJECT*)constant)->value == value)                 \
        {                                                                 \
            return i;                                                     \
        }                                                                 \
    }                                                                     \
    codeObject->constants.push_back(vm::OBJECT::create(value));           \
    return vm::WORD(codeObject->constants.size() - 1);

#define STATE_POP() stateStack.pop_back()
#define STATE_BACK(stateType) ((stateType*)stateStack.back())
#define PUSH_SCOPE(node) scopeStack.push_back(scopeInfo[node])
#define SCOPE_POP() scopeStack.pop_back()
#define SCOPE_BACK() scopeStack.back()

struct LoopState : CompilerState
{
    vm::WORD startIp; // Початок циклу
    // Адреси, що будуть будуть змінені на адресу кінці циклу
    std::vector<vm::WORD> addressesForPatchWithEndBlock;
};

struct FunctionState : CompilerState
{
    vm::CodeObject* codeObject;
};

#define PUSH_LOOP_STATE(startIp) \
    stateStack.push_back(new LoopState{{CompilerStateType::LOOP}, startIp})

#define PUSH_FUNCTION_STATE(codeObject) \
    stateStack.push_back(new FunctionState{{CompilerStateType::FUNCTION}, codeObject})


vm::Frame* compiler::Compiler::compile()
{
    ScopeAnalyzer scopeAnalyzer(root);
    scopeInfo = scopeAnalyzer.analyze();
    PUSH_SCOPE(root);
    compileBlock(root);
    SCOPE_POP();
    emitOpCode(HALT);
    auto frame = new vm::Frame;
    frame->codeObject = codeObject;
    frame->globals = new vm::Frame::object_map_t;
    return frame;
}

void compiler::Compiler::compileBlock(BlockStatement* block)
{
    for (auto statement : block->statements)
    {
        compileStatement(statement);
    }
}

void compiler::Compiler::compileStatement(Statement* statement)
{
    switch (statement->kind)
    {
    case BLOCK_STATEMENT:
        compileBlock((BlockStatement*)statement);
        break;
    case EXPRESSION_STATEMENT:
    {
        auto expStatement = (ExpressionStatement*)statement;
        compileExpressionStatement(expStatement);

        // Очищення стека, якщо значення виразу нікуди не присвоюється.
        // Окрім виразу присвоєння значення змінній,
        // так як він не залишає нічого на стеку після себе
        if (expStatement->expression->kind != ASSIGNMENT_EXPRESSION)
        {
            emitOpCode(POP);
        }
        break;
    }
    case WHILE_STATEMENT:
        compileWhileStatement((WhileStatement*)statement);
        break;
    case BREAK_STATEMENT:
        compileBreakStatement((BreakStatement*)statement);
        break;
    case CONTINUE_STATEMENT:
        compileContinueStatement((ContinueStatement*)statement);
        break;
    case IF_STATEMENT:
        compileIfStatement((IfStatement*)statement);
        break;
    case FUNCTION_STATEMENT:
        compileFunctionDeclaration((FunctionDeclaration*)statement);
        break;
    case RETURN_STATEMENT:
        compileReturnStatement((ReturnStatement*)statement);
        break;
    case FOR_EACH_STATEMET:
        compileForEachStatement((ForEachStatement*)statement);
        break;
    default:
        plog::fatal << "Неможливо обробити вузол \""
            << parser::stringEnum::enumToString(statement->kind) << "\"";
    }
}

void compiler::Compiler::compileExpressionStatement(ExpressionStatement* statement)
{
    compileExpression(statement->expression);
}

void compiler::Compiler::compileWhileStatement(WhileStatement* statement)
{
    auto startWhileAddress = getOffset();
    compileExpression(statement->condition);
    emitOpCode(JMP_IF_FALSE);
    auto endWhileBlock = emitOperand(0);
    PUSH_LOOP_STATE(startWhileAddress);
    compileBlock(statement->block);
    emitOpCode(JMP);
    emitOperand(startWhileAddress);
    patchJumpAddress(endWhileBlock, getOffset());
    for (auto address : STATE_BACK(LoopState)->addressesForPatchWithEndBlock)
    {
        patchJumpAddress(address, getOffset());
    }
    STATE_POP();
}

void compiler::Compiler::compileBreakStatement(BreakStatement* statement)
{
    auto state = (LoopState*)unwindStateStack(CompilerStateType::LOOP);
    if (state)
    {
        setLineno(statement->break_.lineno);
        emitOpCode(JMP);
        auto endBlock = emitOperand(0);
        state->addressesForPatchWithEndBlock.push_back(endBlock);
    }
    else
    {
        throwCompileError("Оператор \"завершити\" знаходиться поза циклом!", statement->break_);
    }
}

void compiler::Compiler::compileContinueStatement(ContinueStatement* statement)
{
    auto state = (LoopState*)unwindStateStack(CompilerStateType::LOOP);
    if (state)
    {
        setLineno(statement->continue_.lineno);
        emitOpCode(JMP);
        emitOperand(state->startIp);
    }
    else
    {
        throwCompileError("Оператор \"пропустити\" знаходиться поза циклом!", statement->continue_);
    }
}

void compiler::Compiler::compileIfStatement(IfStatement* statement)
{
    compileExpression(statement->condition);
    emitOpCode(JMP_IF_FALSE);
    auto endIfBlock = emitOperand(0);
    compileBlock(statement->block);
    if (!statement->elseOrIf)
    {
        patchJumpAddress(endIfBlock, getOffset());
    }
    else
    {
        emitOpCode(JMP);
        auto endIfElseBlock = emitOperand(0);
        patchJumpAddress(endIfBlock, getOffset());

        auto elseOrIf = statement->elseOrIf.value();
        if (elseOrIf->kind == ELSE_STATEMENT)
        {
            auto elseStatement = (ElseStatement*)elseOrIf;
            compileBlock(elseStatement->block);
        }
        else if (elseOrIf->kind == IF_STATEMENT)
        {
            auto ifStatement = (IfStatement*)elseOrIf;
            compileIfStatement(ifStatement);
        }

        patchJumpAddress(endIfElseBlock, getOffset());
    }
}

void compiler::Compiler::compileFunctionDeclaration(parser::FunctionDeclaration* statement)
{
    auto& name = statement->id.text;
    auto fnCodeObject = vm::CodeObject::create(name);
    auto prevCodeObject = codeObject;
    codeObject = fnCodeObject;
    PUSH_FUNCTION_STATE(fnCodeObject);
    PUSH_SCOPE(statement);

    codeObject->locals = SCOPE_BACK()->locals;
    codeObject->cells = SCOPE_BACK()->cells;
    codeObject->freevars = SCOPE_BACK()->freeVariables;

    for (auto& argName : codeObject->locals)
    {
        for (auto& cellName : codeObject->cells)
        {
            if (argName == cellName)
            {
                codeObject->argsAsCells.push_back(argName);
            }
        }
    }

    codeObject->arity = statement->parameters.size() + statement->defaultParameters.size();
    if (statement->variadicParameter)
        codeObject->isVariadic = true;
    compileBlock(statement->block);
    emitOpCode(LOAD_CONST);
    emitOperand(nullConstIdx());
    emitOpCode(RETURN);

    SCOPE_POP();
    STATE_POP();
    codeObject = prevCodeObject;

    for (auto& defaultParameter : statement->defaultParameters)
    {
        fnCodeObject->defaults.push_back(defaultParameter.first.text);
        setLineno(defaultParameter.first.lineno);
        compileExpression(defaultParameter.second);
    }

    setLineno(statement->id.lineno);
    if (fnCodeObject->freevars.size() > 0)
    {
        auto state = (FunctionState*)unwindStateStack(CompilerStateType::FUNCTION);
        auto& cells = state->codeObject->cells;

        for (auto& name : state->codeObject->cells)
        {
            emitOpCode(GET_CELL);
            auto cellIdx = std::find(cells.begin(), cells.end(), name);
            emitOperand(cellIdx - cells.begin());
        }
    }

    codeObject->constants.push_back(fnCodeObject);
    emitOpCode(LOAD_CONST);
    emitOperand(codeObject->constants.size() - 1);
    emitOpCode(MAKE_FUNCTION);
    compileNameSet(name);
}

void compiler::Compiler::compileReturnStatement(parser::ReturnStatement* statement)
{
    auto state = (FunctionState*)unwindStateStack(CompilerStateType::FUNCTION);
    if (state)
    {
        setLineno(statement->return_.lineno);
        if (statement->returnValue)
        {
            compileExpression(statement->returnValue.value());
        }
        else
        {
            emitOpCode(LOAD_CONST);
            emitOperand(nullConstIdx());
        }
        emitOpCode(RETURN);
    }
    else
    {
        throwCompileError("Оператор \"повернути\" знаходиться поза функцією!", statement->return_);
    }
}

void compiler::Compiler::compileForEachStatement(ForEachStatement* statement)
{
    compileExpression(statement->expression);
    setLineno(statement->forEach.lineno);
    emitOpCode(GET_ITER);
    auto startForEachAddress = getOffset();
    emitOpCode(FOR_EACH);
    auto endForEachBlock = emitOperand(0);
    setLineno(statement->variable.lineno);
    compileNameSet(statement->variable.text);
    PUSH_LOOP_STATE(startForEachAddress);
    compileBlock(statement->block);
    emitOpCode(JMP);
    emitOperand(startForEachAddress);
    patchJumpAddress(endForEachBlock, getOffset());
    for (auto address : STATE_BACK(LoopState)->addressesForPatchWithEndBlock)
    {
        patchJumpAddress(address, getOffset());
    }
    STATE_POP();
}

void compiler::Compiler::compileExpression(Expression* expression)
{
    switch (expression->kind)
    {
    case ASSIGNMENT_EXPRESSION:
        compileAssignmentExpression((AssignmentExpression*)expression);
        break;
    case LITERAL_EXPRESSION:
        compileLiteralExpression((LiteralExpression*)expression);
        break;
    case VARIABLE_EXPRESSION:
        compileVariableExpression((VariableExpression*)expression);
        break;
    case CALL_EXPRESSION:
        compileCallExpression((CallExpression*)expression);
        break;
    case BINARY_EXPRESSION:
        compileBinaryExpression((BinaryExpression*)expression);
        break;
    case UNARY_EXPRESSION:
        compileUnaryExpression((UnaryExpression*)expression);
        break;
    case PARENTHESIZED_EXPRESSION:
        compileParenthesizedExpression((ParenthesizedExpression*)expression);
        break;
    case ATTRIBUTE_EXPRESSION:
        compileAttributeExpression((AttributeExpression*)expression);
        break;
    default:
        plog::fatal << "Неможливо обробити вузол \""
            << parser::stringEnum::enumToString(expression->kind) << "\"";
    }
}

void compiler::Compiler::compileAssignmentExpression(AssignmentExpression* expression)
{
    using enum lexer::TokenType;
    std::string name = expression->id.text;

    compileExpression(expression->expression);
    auto assignmentType = expression->assignment.tokenType;
    if (assignmentType == EQUAL)
    {
        setLineno(expression->assignment.lineno);
        compileNameSet(name);
        return;
    }

    setLineno(expression->id.lineno);
    compileNameGet(name);

    setLineno(expression->assignment.lineno);
    switch (assignmentType)
    {
    case PLUS_EQUAL: emitOpCode(ADD); break;
    case MINUS_EQUAL: emitOpCode(SUB); break;
    case STAR_EQUAL: emitOpCode(MUL); break;
    case SLASH_EQUAL: emitOpCode(DIV); break;
    case PERCENT_EQUAL: emitOpCode(MOD); break;
    case BACKSLASH_EQUAL: emitOpCode(FLOOR_DIV); break;
    default:
        plog::fatal << "Неправильний оператор присвоєння: \""
            << lexer::stringEnum::enumToString(assignmentType) << "\"";
    }

    compileNameSet(name);
}

void compiler::Compiler::compileLiteralExpression(LiteralExpression* expression)
{
    using enum lexer::TokenType;
    vm::WORD index;
    setLineno(expression->literalToken.lineno);
    switch (expression->literalToken.tokenType)
    {
    case NUMBER:
    {
        auto value = std::get<i64>(expression->value);
        index = integerConstIdx(value);
        break;
    }
    case REAL:
    {
        auto value = std::get<double>(expression->value);
        index = realConstIdx(value);
        break;
    }
    case BOOLEAN:
    {
        auto value = std::get<bool>(expression->value);
        index = booleanConstIdx(value);
        break;
    }
    case STRING:
    {
        auto& value = std::get<std::string>(expression->value);
        index = stringConstIdx(value);
        break;
    }
    case NULL_:
    {
        index = nullConstIdx();
        break;
    }
    default:
        break;
    }
    emitOpCode(LOAD_CONST);
    emitOperand(index);
}

void compiler::Compiler::compileVariableExpression(VariableExpression* expression)
{
    auto& variableName = expression->variable.text;
    setLineno(expression->variable.lineno);
    compileNameGet(variableName);
}

void compiler::Compiler::compileCallExpression(CallExpression* expression)
{
    auto argc = (vm::WORD)expression->arguments.size() + (vm::WORD)expression->namedArguments.size();
    bool withNamedArgs = (bool)expression->namedArguments.size();
    vm::WORD namedArgsIdx;

    if (expression->callable->kind == NodeKind::ATTRIBUTE_EXPRESSION)
    {
        // Якщо викликним виразом є атрибут,
        // то потрібно отримувати значення за допомогою LOAD_METHOD
        compileAttributeExpression((AttributeExpression*)expression->callable, true);
    }
    else
    {
        compileExpression(expression->callable);
    }

    for (auto argument : expression->arguments)
    {
        compileExpression(argument);
    }

    if (withNamedArgs)
    {
        std::vector<std::string> namedArgs;
        for (auto& namedArgument : expression->namedArguments)
        {
            for (auto& a: namedArgs)
            {
               if (namedArgument.first.text == a)
               {
                   throwCompileError(
                       utils::format(
                           "Іменований параметр \"%s\" повторюється", a.c_str()),
                       namedArgument.first
                   );
               }
            }

            namedArgs.push_back(namedArgument.first.text);
            compileExpression(namedArgument.second);
        }

        namedArgsIdx = stringVectorIdx(namedArgs);
    }

    // Якщо викликний вираз є атрибутом,
    // то викликати його треба за допомогою CALL_METHOD
    if (expression->callable->kind == NodeKind::ATTRIBUTE_EXPRESSION)
    {
        emitOpCode(withNamedArgs ? CALL_METHOD_NA : CALL_METHOD);
    }
    else
    {
        emitOpCode(withNamedArgs ? CALL_NA : CALL);
    }
    emitOperand(argc);

    if (withNamedArgs)
    {
        emitOperand(namedArgsIdx);
    }
}

void compiler::Compiler::compileBinaryExpression(BinaryExpression* expression)
{
    compileExpression(expression->right);
    compileExpression(expression->left);

    setLineno(expression->operator_.lineno);
    using enum vm::ObjectCompOperator;
    switch (expression->operator_.tokenType)
    {
    case lexer::TokenType::PLUS: emitOpCode(ADD); break;
    case lexer::TokenType::MINUS: emitOpCode(SUB); break;
    case lexer::TokenType::SLASH: emitOpCode(DIV); break;
    case lexer::TokenType::STAR: emitOpCode(MUL); break;
    case lexer::TokenType::PERCENT: emitOpCode(MOD); break;
    case lexer::TokenType::BACKSLASH: emitOpCode(FLOOR_DIV); break;
    case lexer::TokenType::AND: emitOpCode(AND); break;
    case lexer::TokenType::OR: emitOpCode(OR); break;
    case lexer::TokenType::IS: emitOpCode(IS); break;
    case lexer::TokenType::EQUAL_EQUAL:
        emitOpCode(COMPARE); emitOperand((vm::WORD)EQ); break;
    case lexer::TokenType::NOT_EQUAL:
        emitOpCode(COMPARE); emitOperand((vm::WORD)NE); break;
    case lexer::TokenType::GREATER:
        emitOpCode(COMPARE); emitOperand((vm::WORD)GT); break;
    case lexer::TokenType::GREATER_EQUAL:
        emitOpCode(COMPARE); emitOperand((vm::WORD)GE); break;
    case lexer::TokenType::LESS:
        emitOpCode(COMPARE); emitOperand((vm::WORD)LT); break;
    case lexer::TokenType::LESS_EQUAL:
        emitOpCode(COMPARE); emitOperand((vm::WORD)LE); break;
    default:
        plog::fatal << "Неправильний токен оператора: \""
            <<  lexer::stringEnum::enumToString(expression->operator_.tokenType) << "\"";
    }
}

void compiler::Compiler::compileUnaryExpression(UnaryExpression* expression)
{
    compileExpression(expression->operand);

    setLineno(expression->operator_.lineno);
    switch (expression->operator_.tokenType)
    {
    case lexer::TokenType::PLUS: emitOpCode(POS); break;
    case lexer::TokenType::MINUS: emitOpCode(NEG); break;
    case lexer::TokenType::NOT: emitOpCode(NOT); break;
    default:
        plog::fatal << "Неправильний токен унарного оператора: \""
            << lexer::stringEnum::enumToString(expression->operator_.tokenType) << "\"";
    }
}

void compiler::Compiler::compileParenthesizedExpression(ParenthesizedExpression* expression)
{
    compileExpression(expression->expression);
}

void compiler::Compiler::compileAttributeExpression(
    AttributeExpression* expression, bool isMethod)
{
    compileExpression(expression->expression);
    setLineno(expression->attribute.lineno);
    emitOpCode(isMethod ? LOAD_METHOD : GET_ATTR);
    emitOperand(nameIdx(expression->attribute.text));
}

void compiler::Compiler::compileNameGet(const std::string& name)
{
    auto scope = SCOPE_BACK();
    auto varGetter = scope->getVarGetter(name);

    emitOpCode(varGetter);
    if (varGetter == LOAD_LOCAL)
    {
        emitOperand(localIdx(name));
    }
    else if (varGetter == LOAD_CELL)
    {
        emitOperand(freeIdx(name));
    }
    else if (varGetter == LOAD_GLOBAL)
    {
        emitOperand(nameIdx(name));
    }
}

void compiler::Compiler::compileNameSet(const std::string& name)
{
    auto scope = SCOPE_BACK();
    auto varSetter = scope->getVarSetter(name);

    emitOpCode(varSetter);
    if (varSetter == STORE_LOCAL)
    {
        emitOperand(localIdx(name));
    }
    else if (varSetter == STORE_CELL)
    {
        emitOperand(freeIdx(name));
    }
    else if (varSetter == STORE_GLOBAL)
    {
        emitOperand(nameIdx(name));
    }
}

CompilerState* compiler::Compiler::unwindStateStack(CompilerStateType type)
{
    for (auto it = stateStack.rbegin(); it != stateStack.rend(); ++it)
    {
        if ((*it)->type == type)
        {
            return *it;
        }
    }
    return nullptr;
}

vm::WORD compiler::Compiler::booleanConstIdx(bool value)
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        if (OBJECT_IS(codeObject->constants[i], &vm::boolObjectType) == false)
        {
            continue;
        }
        if (((vm::BoolObject*)codeObject->constants[i])->value == value)
        {
            return i;
        }
    }
    codeObject->constants.push_back(P_BOOL(value));
    return (vm::WORD)codeObject->constants.size() - 1;
}

vm::WORD compiler::Compiler::realConstIdx(double value)
{
    FIND_CONST_IDX(RealObject, realObjectType)
}

vm::WORD compiler::Compiler::integerConstIdx(i64 value)
{
    FIND_CONST_IDX(IntObject, intObjectType)
}

vm::WORD compiler::Compiler::stringVectorIdx(const std::vector<std::string>& value)
{
    FIND_CONST_IDX(StringVectorObject, stringVectorObjectType)
}

vm::WORD compiler::Compiler::stringConstIdx(const std::string& value)
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        auto constant = codeObject->constants[i];
        if (OBJECT_IS(constant, &vm::stringObjectType) == false)
        {
            continue;
        }
        else if (((vm::StringObject*)constant)->asUtf8() == value)
        {
            return i;
        }
    }
    codeObject->constants.push_back(vm::StringObject::create(value));
        return vm::WORD(codeObject->constants.size() - 1);
}

vm::WORD compiler::Compiler::nullConstIdx()
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        if (OBJECT_IS(codeObject->constants[i], &vm::nullObjectType))
        {
            return i;
        }
    }
    codeObject->constants.push_back(&vm::P_null);
    return (vm::WORD)codeObject->constants.size() - 1;
}

vm::WORD compiler::Compiler::freeIdx(const std::string& name)
{
    auto& cells = codeObject->cells;
    auto cellIdx = std::find(cells.begin(), cells.end(), name);
    if (cellIdx != cells.end())
    {
        return cellIdx - cells.begin();
    }
    else
    {
        auto& freevars = codeObject->freevars;
        auto freeIndex = std::find(freevars.begin(), freevars.end(), name);
        if (freeIndex != freevars.end())
        {
            return freeIndex - freevars.begin() + cells.size();
        }
        else
        {
            plog::fatal << "Неможливо знайти змінну \"" << name << "\"";
        }
    }
}

vm::WORD compiler::Compiler::localIdx(const std::string& name)
{
    auto& locals = codeObject->locals;
    auto nameIdx = std::find(locals.begin(), locals.end(), name);
    if (nameIdx != locals.end())
    {
        return nameIdx - locals.begin();
    }
    else
    {
        plog::fatal << "Локальної змінної \"" << name << "\" не існує";
    }
}

vm::WORD compiler::Compiler::nameIdx(const std::string& name)
{
    auto& names = codeObject->names;
    auto nameIdx = std::find(names.begin(), names.end(), name);
    if (nameIdx != names.end())
    {
        return nameIdx - names.begin();
    }
    else
    {
        names.push_back(name);
        return names.size() - 1;
    }
}

void compiler::Compiler::throwCompileError(std::string message, lexer::Token token)
{
    vm::SyntaxException error(message, token.lineno, token.positionInLine);
    vm::throwSyntaxException(error, code);
    exit(1);
}

void compiler::Compiler::setLineno(size_t lineno)
{
    currentLineno = (vm::WORD)lineno;
}

vm::WORD compiler::Compiler::emitOpCode(vm::OpCode op)
{
    codeObject->code.push_back((vm::WORD)op);
    auto ip = vm::WORD(codeObject->code.size() - 1);
    codeObject->ipToLineno[ip] = currentLineno;
    return ip;
}

vm::WORD compiler::Compiler::emitOperand(vm::WORD operand)
{
    codeObject->code.push_back(operand);
    return vm::WORD(codeObject->code.size() - 1);
}

vm::WORD compiler::Compiler::getOffset()
{
    return (vm::WORD)codeObject->code.size();
}

void compiler::Compiler::patchJumpAddress(int offset, vm::WORD newAddress)
{
    codeObject->code[offset] = newAddress;
}

compiler::Compiler::Compiler(BlockStatement* root, std::string code)
    :
    root(root),
    code(code)
{
    codeObject = vm::CodeObject::create("TODO");
}
