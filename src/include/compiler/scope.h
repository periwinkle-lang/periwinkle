#ifndef SCOPE_H
#define SCOPE_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

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
        std::vector<std::string> cells;
        std::vector<std::string> freeVariable;

        void addLocal(const std::string& name);
        void addCell(const std::string& name);
        void addFree(const std::string& name);
        void localVariableToCell(const std::string& name, Scope* owner);
        std::pair<Scope*, VariableType> resolve(const std::string& name, VariableType type);
    };
}

#endif
