#ifndef  COMPILATOR_H
#define  COMPILATOR_H

#include "expression.h"
#include "statement.h"
#include "vm.h"

namespace compiler
{
    class Compiler
    {
    private:
        parser::BlockStatement* root;
        std::string code;
        vm::Frame* frame;

        void compileBlock(parser::BlockStatement* block);
        void compileStatement(parser::Statement* statement);
        void compileExpression(parser::Expression* expression);
        void compileAssignmentExpression(parser::AssignmentExpression* expression);
        void compileLiteralExpression(parser::LiteralExpression* expression);
        void compileVariableExpression(parser::VariableExpression* expression);
        void compileCallExpression(parser::CallExpression* expression);
        void compileBinaryExpression(parser::BinaryExpression* expression);

        vm::WORD booleanConstIdx(bool value);
        vm::WORD realConstIdx(double value);
        vm::WORD integerConstIdx(i64 value);
        vm::WORD stringConstIdx(const std::string& value);
        vm::WORD nullConstIdx();
        vm::WORD nameIdx(const std::string& name); // Повертає індекс з CodeObject->names
        inline void emitOpCode(vm::OpCode op);
        inline void emitOperand(vm::WORD operand);
        inline int getOffset();
        inline void patchJumpAddress(int offset, vm::WORD newAddress);
    public:
        vm::Frame* compile();
        Compiler(parser::BlockStatement* root, std::string code);
    };
}

#endif
