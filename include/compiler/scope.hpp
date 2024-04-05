#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>

#include "ast.hpp"
#include "vm.hpp"

namespace compiler
{
    enum class ScopeType
    {
        GLOBAL, FUNCTION,
    };

    enum class VariableType
    {
        GLOBAL, LOCAL, CELL,
    };

    struct Scope
    {
        ScopeType type;
        Scope* parent;
        std::unordered_map<std::string, VariableType> variableInfo;
        std::vector<std::string> locals;
        std::vector<std::string> cells;
        std::vector<std::string> freeVariables;

        void addLocal(const std::string& name);
        void addCell(const std::string& name);
        void addFree(const std::string& name);

        // Переносить змінну з LOCAL до CELL
        void promote(const std::string& name, Scope* owner);
        void maybePromote(const std::string& name);
        std::pair<Scope*, VariableType> resolve(const std::string& name, VariableType type);

        vm::OpCode getVarGetter(const std::string& name);
        vm::OpCode getVarSetter(const std::string& name);
    };

    class ScopeAnalyzer
    {
    public:
        using scope_info_t = std::map<const ast::Node*, Scope*>;
    private:
        scope_info_t scopeInfo;
        ast::BlockStatement* rootNode;

        void _analyze(ast::Node* node, Scope* parent);
    public:
        scope_info_t analyze();

        ScopeAnalyzer(ast::BlockStatement* root);
    };
}

#endif
