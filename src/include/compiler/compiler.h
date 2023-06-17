#ifndef  COMPILATOR_H
#define  COMPILATOR_H

#include <vector>

#include "expression.h"
#include "statement.h"
#include "vm.h"
#include "scope.h"

namespace compiler
{
    enum class CompilerStateType
    {
        LOOP, FUNCTION
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
        vm::CodeObject* codeObject;
        std::vector<CompilerState*> stateStack;
        vm::WORD currentLineno = 0; // Номер лінії коду, який зараз компілюється
        std::vector<Scope*> scopeStack;
        ScopeAnalyzer::scope_info_t scopeInfo;

        void compileBlock(parser::BlockStatement* block);
        void compileStatement(parser::Statement* statement);
        void compileExpressionStatement(parser::ExpressionStatement* statement);
        void compileWhileStatement(parser::WhileStatement* statement);
        void compileBreakStatement(parser::BreakStatement* statement);
        void compileContinueStatement(parser::ContinueStatement* statement);
        void compileIfStatement(parser::IfStatement* statement);
        void compileFunctionDeclaration(parser::FunctionDeclaration* statement);
        void compileReturnStatement(parser::ReturnStatement* statement);
        void compileForEachStatement(parser::ForEachStatement* statement);

        void compileExpression(parser::Expression* expression);
        void compileAssignmentExpression(parser::AssignmentExpression* expression);
        void compileLiteralExpression(parser::LiteralExpression* expression);
        void compileVariableExpression(parser::VariableExpression* expression);
        void compileCallExpression(parser::CallExpression* expression);
        void compileBinaryExpression(parser::BinaryExpression* expression);
        void compileUnaryExpression(parser::UnaryExpression* expression);
        void compileParenthesizedExpression(parser::ParenthesizedExpression* expression);
        void compileAttributeExpression(
            parser::AttributeExpression* expression, bool isMethod=false);

        void compileNameGet(const std::string& name);
        void compileNameSet(const std::string& name);

        CompilerState* unwindStateStack(CompilerStateType type);
        vm::WORD booleanConstIdx(bool value);
        vm::WORD realConstIdx(double value);
        vm::WORD integerConstIdx(i64 value);
        vm::WORD stringVectorIdx(const std::vector<std::string>& value);
        vm::WORD stringConstIdx(const std::string& value);
        vm::WORD nullConstIdx();
        vm::WORD freeIdx(const std::string& name); // Повертає індекс для Frame->freevars
        vm::WORD localIdx(const std::string& name); // Повертає індекс з CodeObject->locals
        vm::WORD nameIdx(const std::string& name); // Повертає індекс з CodeObject->names
        void throwCompileError(std::string message, lexer::Token token);
        // Встановлює номер стрічки в коді, який зараз компілюється.
        // !!!Викликати перед компіляцією стрічки!!!
        inline void setLineno(size_t lineno);
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
