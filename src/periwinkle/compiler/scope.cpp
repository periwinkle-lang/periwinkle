#include "scope.h"

using namespace compiler;

void compiler::Scope::addLocal(const std::string& name)
{
    variableInfo[name] = type == ScopeType::GLOBAL ? VariableType::GLOBAL : VariableType::LOCAL;
}

void compiler::Scope::addCell(const std::string& name)
{
    cells.push_back(name);
    variableInfo[name] = VariableType::CELL;
}

void compiler::Scope::addFree(const std::string& name)
{
    freeVariable.push_back(name);
    variableInfo[name] = VariableType::CELL;
}

void compiler::Scope::localVariableToCell(const std::string& name, Scope* owner)
{
    owner->addCell(name);
    auto scope = this;
    while (scope != owner)
    {
        scope->addFree(name);
        scope = scope->parent;
    }
}

std::pair<Scope*, VariableType> compiler::Scope::resolve(const std::string& name, VariableType variableType)
{
    if (variableInfo.contains(name))
    {
        return std::make_pair(this, variableType);
    }

    if (type == ScopeType::FUNCTION)
    {
        variableType = VariableType::CELL;
    }

    if (parent == nullptr)
    {
        std::cerr << "Посилання на: " << name << " не визначено";
        exit(1);
    }

    if (parent->type == ScopeType::GLOBAL)
    {
        return std::make_pair(parent, VariableType::GLOBAL);
    }

    return parent->resolve(name, variableType);
}
