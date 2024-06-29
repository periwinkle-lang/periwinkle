#ifndef  COMPILATOR_H
#define  COMPILATOR_H

#include <vector>

#include "ast.hpp"
#include "vm.hpp"
#include "scope.hpp"
#include "program_source.hpp"

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
        ast::BlockStatement* root;
        periwinkle::ProgramSource* source;
        vm::CodeObject* codeObject;
        std::vector<CompilerState*> stateStack;
        vm::WORD currentLineno = 0; // Номер лінії коду, який зараз компілюється
        std::vector<Scope*> scopeStack;
        ScopeAnalyzer::scope_info_t scopeInfo;
        bool isRootBlock = false;
        bool isLastStatementInBlock = false;
        bool isRootBlockHasReturn = false;

        void compileBlock(ast::BlockStatement* block);
        void compileStatement(ast::Statement* statement);
        void compileExpressionStatement(ast::ExpressionStatement* statement);
        void compileWhileStatement(ast::WhileStatement* statement);
        void compileBreakStatement(ast::BreakStatement* statement);
        void compileContinueStatement(ast::ContinueStatement* statement);
        void compileIfStatement(ast::IfStatement* statement);
        void compileFunctionDeclaration(ast::FunctionDeclaration* statement);
        void compileReturnStatement(ast::ReturnStatement* statement);
        void compileForEachStatement(ast::ForEachStatement* statement);
        void compileTryCatchStatement(ast::TryCatchStatement* statement);

        void compileExpression(ast::Expression* expression);
        void compileAssignmentExpression(ast::AssignmentExpression* expression);
        void compileLiteralExpression(ast::LiteralExpression* expression);
        void compileVariableExpression(ast::VariableExpression* expression);
        void compileCallExpression(ast::CallExpression* expression);
        void compileBinaryExpression(ast::BinaryExpression* expression);
        void compileUnaryExpression(ast::UnaryExpression* expression);
        void compileParenthesizedExpression(ast::ParenthesizedExpression* expression);
        void compileAttributeExpression(
            ast::AttributeExpression* expression, bool isMethod=false);

        void compileNameGet(const std::string& name);
        void compileNameSet(const std::string& name);
        void compileNameDelete(const std::string& name);

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
        void throwCompileError(std::string message, ast::Token token);
        // Встановлює номер стрічки в коді, який зараз компілюється.
        // !!!Викликати перед компіляцією стрічки!!!
        inline void setLineno(ast::Token token);
        inline vm::WORD emitOpCode(vm::OpCode op);
        inline vm::WORD emitOperand(vm::WORD operand);
        inline vm::WORD getOffset();
        inline void patchJumpAddress(int offset, vm::WORD newAddress);
    public:
        vm::Frame* compile();
        Compiler(ast::BlockStatement* root, periwinkle::ProgramSource* source);
    };
}

#endif
