#ifndef  COMPILATOR_H
#define  COMPILATOR_H

#include <vector>
#include "expression.h"
#include "statement.h"
#include "vm.h"

namespace compiler
{
    enum class CompilerStateType
    {
        WHILE
    };

    struct CompilerState
    {
        CompilerStateType type;
    };

    class Compiler
    {
    private:
        parser::BlockStatement* root;
        std::string code;
        vm::Frame* frame;
        std::vector<CompilerState*> stateStack;

        void compileBlock(parser::BlockStatement* block);
        void compileStatement(parser::Statement* statement);
        void compileExpressionStatement(parser::ExpressionStatement* statement);
        void compileWhileStatement(parser::WhileStatement* statement);
        void compileBreakStatement(parser::BreakStatement* statement);
        void compileContinueStatement(parser::ContinueStatement* statement);
        void compileIfStatement(parser::IfStatement* statement);

        void compileExpression(parser::Expression* expression);
        void compileAssignmentExpression(parser::AssignmentExpression* expression);
        void compileLiteralExpression(parser::LiteralExpression* expression);
        void compileVariableExpression(parser::VariableExpression* expression);
        void compileCallExpression(parser::CallExpression* expression);
        void compileBinaryExpression(parser::BinaryExpression* expression);

        CompilerState* unwindStateStack(CompilerStateType type);
        vm::WORD booleanConstIdx(bool value);
        vm::WORD realConstIdx(double value);
        vm::WORD integerConstIdx(i64 value);
        vm::WORD stringConstIdx(const std::string& value);
        vm::WORD nullConstIdx();
        vm::WORD nameIdx(const std::string& name); // Повертає індекс з CodeObject->names
        void throwCompileError(std::string message, lexer::Token token);
        inline vm::WORD emitOpCode(vm::OpCode op);
        inline vm::WORD emitOperand(vm::WORD operand);
        inline vm::WORD getOffset();
        inline void patchJumpAddress(int offset, vm::WORD newAddress);
    public:
        vm::Frame* compile();
        Compiler(parser::BlockStatement* root, std::string code);
    };
}

#endif
