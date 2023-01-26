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
    compileBlock(statement->block);
    emitOpCode(JMP);
    emitOperand(startWhileAddress);
    patchJumpAddress(endWhileBlock, getOffset());
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

    switch (expression->assignment.tokenType)
    {
    case EQUAL:
    {
        compileExpression(expression->expression);
        emitOpCode(STORE_GLOBAL);
        emitOperand(nameIdx);
        break;
    }
    default:
        plog::fatal << "Оператор ще не реалізовано: \""
            << lexer::stringEnum::enumToString(expression->assignment.tokenType) << "\"";
    }
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

//void compiler::Compiler::compileIfStatement(IfStatement* statement)
//{
//    compileExpression(statement->condition);
//    emitOpCode(OpCode::JMP_IF_FALSE);
//    auto elseJmpAddr = getOffset();
//    emitOperand(0);
//    compileBlock(statement->then);
//    if (statement->else_ == nullptr)
//    {
//        patchJumpAddress(elseJmpAddr, getOffset());
//    }
//    else
//    {
//        auto end = getOffset();
//        emitOpCode(OpCode::JMP);
//
//        if (statement->else_->statement->kind() == NodeKind::IF_STATEMENT)
//        {
//            auto elseIf = (IfStatement*)statement->else_->statement;
//            compileIfStatement(elseIf);
//        }
//        else
//        {
//            auto else_ = (BlockStatement*)statement->else_->statement;
//            compileBlock(else_);
//        }
//
//        patchJumpAddress(end, getOffset());
//    }
//}

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

int compiler::Compiler::getOffset()
{
    return frame->codeObject->code.size();
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
