#include <algorithm>
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
#include "types.h"
#include "builtins.h"
#include "plogger.h"
#include "exception.h"

using namespace compiler;
using namespace parser;
using enum vm::OpCode;
using enum parser::NodeKind;

#define FIND_CONST_IDX(OBJECT, OBJECT_TYPES)                              \
    auto codeObject = frame->codeObject;                                  \
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i) \
    {                                                                     \
        auto constant = codeObject->constants[i];                         \
        if (constant->objectType->type != vm::ObjectTypes::OBJECT_TYPES ) \
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

struct WhileState : CompilerState
{
    vm::WORD startIp; // Початок циклу
    // Адреси, що будуть будуть змінені на адресу кінці циклу
    std::vector<vm::WORD> addressesForPatchWithEndBlock;
};

#define PUSH_WHILE_STATE(startIp) \
    stateStack.push_back(new WhileState{{CompilerStateType::WHILE}, startIp})


vm::Frame* compiler::Compiler::compile()
{
    compileBlock(root);
    emitOpCode(HALT);
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
    switch (statement->kind())
    {
    case BLOCK_STATEMENT:
        compileBlock((BlockStatement*)statement);
        break;
    case EXPRESSION_STATEMENT:
        compileExpressionStatement((ExpressionStatement*)statement);
        break;
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
    default:
        plog::fatal << "Неможливо обробити вузол \"" << parser::stringEnum::enumToString(statement->kind())
            << "\"" << std::endl;
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
    PUSH_WHILE_STATE(startWhileAddress);
    compileBlock(statement->block);
    emitOpCode(JMP);
    emitOperand(startWhileAddress);
    patchJumpAddress(endWhileBlock, getOffset());
    for (auto address : STATE_BACK(WhileState)->addressesForPatchWithEndBlock)
    {
        patchJumpAddress(address, getOffset());
    }
    STATE_POP();
}

void compiler::Compiler::compileBreakStatement(BreakStatement* statement)
{
    auto state = (WhileState*)unwindStateStack(CompilerStateType::WHILE);
    if (state)
    {
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
    auto state = (WhileState*)unwindStateStack(CompilerStateType::WHILE);
    if (state)
    {
        emitOpCode(JMP);
        emitOperand(state->startIp);
    }
    else
    {
        throwCompileError("Оператор \"продовжити\" знаходиться поза циклом!", statement->continue_);
    }
}

void compiler::Compiler::compileIfStatement(IfStatement* statement)
{
    compileExpression(statement->condition);
    emitOpCode(JMP_IF_FALSE);
    auto endIfBlock = emitOperand(0);
    compileBlock(statement->block);
    if (statement->elseOrIf == nullptr)
    {
        patchJumpAddress(endIfBlock, getOffset());
    }
    else
    {
        emitOpCode(JMP);
        auto endIfElseBlock = emitOperand(0);
        patchJumpAddress(endIfBlock, getOffset());

        if (statement->elseOrIf->kind() == ELSE_STATEMENT)
        {
            auto elseStatement = (ElseStatement*)statement->elseOrIf;
            compileBlock(elseStatement->block);
        }
        else if (statement->elseOrIf->kind() == IF_STATEMENT)
        {
            auto ifStatement = (IfStatement*)statement->elseOrIf;
            compileIfStatement(ifStatement);
        }

        patchJumpAddress(endIfElseBlock, getOffset());
    }
}

void compiler::Compiler::compileExpression(Expression* expression)
{
    switch (expression->kind())
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
    default:
        plog::fatal << "Неможливо обробити вузол \""
            << parser::stringEnum::enumToString(expression->kind()) << "\"";
    }
}

void compiler::Compiler::compileAssignmentExpression(AssignmentExpression* expression)
{
    using enum lexer::TokenType;
    std::string name = expression->id.text;
    auto nameIdx = this->nameIdx(name);

    compileExpression(expression->expression);
    auto assignmentType = expression->assignment.tokenType;
    if (assignmentType == EQUAL)
    {
        emitOpCode(STORE_GLOBAL);
        emitOperand(nameIdx);
        return;
    }

    emitOpCode(LOAD_NAME);
    emitOperand(nameIdx);

    switch (assignmentType)
    {
    case PLUS_EQUAL: emitOpCode(ADD); break;
    case MINUS_EQUAL: emitOpCode(SUB); break;
    case STAR_EQUAL: emitOpCode(MUL); break;
    case SLASH_EQUAL: emitOpCode(DIV); break;
    case PERCENT_EQUAL: emitOpCode(MOD); break;
    default:
        plog::fatal << "Неправильний оператор присвоєння: \""
            << lexer::stringEnum::enumToString(assignmentType) << "\"";
    }

    emitOpCode(STORE_GLOBAL);
    emitOperand(nameIdx);
}

void compiler::Compiler::compileLiteralExpression(LiteralExpression* expression)
{
    using enum lexer::TokenType;
    vm::WORD index;
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
    auto variableNameIdx = nameIdx(variableName);
    emitOpCode(LOAD_GLOBAL);
    emitOperand(variableNameIdx);
}

void compiler::Compiler::compileCallExpression(CallExpression* expression)
{
    auto& functionName = expression->identifier.text;
    auto functionNameIdx = nameIdx(functionName);
    auto argc = (vm::WORD)expression->arguments.size();

    for (auto argument : expression->arguments)
    {
        compileExpression(argument);
    }

    emitOpCode(LOAD_NAME);
    emitOperand(functionNameIdx);
    emitOpCode(CALL);
    emitOperand(argc);
}

void compiler::Compiler::compileBinaryExpression(BinaryExpression* expression)
{
    compileExpression(expression->right);
    compileExpression(expression->left);

    switch (expression->operator_.tokenType)
    {
    case lexer::TokenType::PLUS:
        emitOpCode(ADD);
        break;
    case lexer::TokenType::MINUS:
        emitOpCode(SUB);
        break;
    case lexer::TokenType::SLASH:
        emitOpCode(DIV);
        break;
    case lexer::TokenType::STAR:
        emitOpCode(MUL);
        break;
    case lexer::TokenType::PERCENT:
        emitOpCode(MOD);
        break;
    default:
        plog::fatal << "Неправильний токен оператора: \""
            <<  lexer::stringEnum::enumToString(expression->operator_.tokenType) << "\"";
    }
}

CompilerState* compiler::Compiler::unwindStateStack(CompilerStateType type)
{
    for (auto item : stateStack)
    {
        if (item->type == type)
        {
            return item;
        }
    }
    return nullptr;
}

vm::WORD compiler::Compiler::booleanConstIdx(bool value)
{
    FIND_CONST_IDX(BoolObject, BOOL)
}

vm::WORD compiler::Compiler::realConstIdx(double value)
{
    FIND_CONST_IDX(RealObject, REAL);
}

vm::WORD compiler::Compiler::integerConstIdx(i64 value)
{
    FIND_CONST_IDX(IntObject, INTEGER)
}

vm::WORD compiler::Compiler::stringConstIdx(const std::string& value)
{
    FIND_CONST_IDX(StringObject, STRING)
}

vm::WORD compiler::Compiler::nullConstIdx()
{
    auto codeObject = frame->codeObject;
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        if (codeObject->constants[i]->objectType->type == vm::ObjectTypes::NULL_)
        {
            return i;
        }
    }
    codeObject->constants.push_back(&vm::nullObject);
    return (vm::WORD)codeObject->constants.size() - 1;
}

vm::WORD compiler::Compiler::nameIdx(const std::string& name)
{
    auto& names = frame->codeObject->names;
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
    vm::SyntaxException error(message, token.lineno);
    vm::throwSyntaxException(error, code, token.position);
    exit(1);
}

vm::WORD compiler::Compiler::emitOpCode(vm::OpCode op)
{
    frame->codeObject->code.push_back((vm::WORD)op);
    return vm::WORD(frame->codeObject->code.size() - 1);
}

vm::WORD compiler::Compiler::emitOperand(vm::WORD operand)
{
    frame->codeObject->code.push_back(operand);
    return vm::WORD(frame->codeObject->code.size() - 1);
}

vm::WORD compiler::Compiler::getOffset()
{
    return (vm::WORD)frame->codeObject->code.size();
}

void compiler::Compiler::patchJumpAddress(int offset, vm::WORD newAddress)
{
    frame->codeObject->code[offset] = newAddress;
}

compiler::Compiler::Compiler(BlockStatement* root, std::string code)
    :
    root(root),
    code(code),
    frame(new vm::Frame)
{
    frame->codeObject = vm::CodeObject::create("TODO");
}
