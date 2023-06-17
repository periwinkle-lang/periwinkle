#include "scope.h"
#include "expression.h"
#include "plogger.h"
#include "builtins.h"

using namespace compiler;

void compiler::Scope::addLocal(const std::string& name)
{
    locals.push_back(name);
    variableInfo[name] = type == ScopeType::GLOBAL ? VariableType::GLOBAL : VariableType::LOCAL;
}

void compiler::Scope::addCell(const std::string& name)
{
    cells.push_back(name);
    variableInfo[name] = VariableType::CELL;
}

void compiler::Scope::addFree(const std::string& name)
{
    freeVariables.push_back(name);
    variableInfo[name] = VariableType::CELL;
}

void compiler::Scope::promote(const std::string& name, Scope* owner)
{
    owner->addCell(name);
    auto scope = this;
    while (scope != owner)
    {
        scope->addFree(name);
        scope = scope->parent;
    }
}

void compiler::Scope::maybePromote(const std::string& name)
{
    auto varType = (type == ScopeType::GLOBAL) ? VariableType::GLOBAL : VariableType::LOCAL;

    if (variableInfo.contains(name))
    {
        varType = variableInfo[name];
    }

    if (varType == VariableType::CELL) {
        return;
    }

    auto [parent, resolveVarType] = resolve(name, varType);

    variableInfo[name] = resolveVarType;

    if (resolveVarType == VariableType::CELL) {
        promote(name, parent);
    }
}

std::pair<Scope*, VariableType> compiler::Scope::resolve(
    const std::string& name, VariableType variableType)
{
    if (vm::getBuiltin()->contains(name) || type == ScopeType::GLOBAL)
    {
        return std::make_pair(nullptr, VariableType::GLOBAL);
    }

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
        plog::fatal << "Посилання на \"" << name << "\" не визначено";
    }

    if (parent->type == ScopeType::GLOBAL)
    {
        return std::make_pair(parent, VariableType::GLOBAL);
    }

    return parent->resolve(name, variableType);
}

vm::OpCode Scope::getVarGetter(const std::string& name)
{
    using enum VariableType;
    switch (variableInfo[name])
    {
    case GLOBAL:
        return vm::OpCode::LOAD_GLOBAL;
    case LOCAL:
        return vm::OpCode::LOAD_LOCAL;
    case CELL:
        return vm::OpCode::LOAD_CELL;
    default:
        plog::fatal << "Невідомий тип змінної \"" << (int)variableInfo[name] << "\"";
    }
}

vm::OpCode Scope::getVarSetter(const std::string& name)
{
    using enum VariableType;
    switch (variableInfo[name])
    {
    case GLOBAL:
        return vm::OpCode::STORE_GLOBAL;
    case LOCAL:
        return vm::OpCode::STORE_LOCAL;
    case CELL:
        return vm::OpCode::STORE_CELL;
    default:
        plog::fatal << "Невідомий тип змінної \"" << (int)variableInfo[name] << "\"";
    }
}

void ScopeAnalyzer::_analyze(parser::Node* node, Scope* parent)
{
    using enum parser::NodeKind;

    switch (node->kind)
    {
    case BLOCK_STATEMENT:
    {
        for (auto node : ((parser::BlockStatement*)node)->statements)
        {
            _analyze(node, parent);
        }
        break;
    }
    case EXPRESSION_STATEMENT:
    {
        _analyze(((parser::ExpressionStatement*)node)->expression, parent);
        break;
    }
    case WHILE_STATEMENT:
    {
        _analyze(((parser::WhileStatement*)node)->block, parent);
        break;
    }
    case IF_STATEMENT:
    {
        _analyze(((parser::IfStatement*)node)->block, parent);
        break;
    }
    case ELSE_STATEMENT:
    {
        _analyze(((parser::ElseStatement*)node)->block, parent);
        break;
    }
    case FUNCTION_STATEMENT:
    {
        auto fnDeclaration = (parser::FunctionDeclaration*)node;
        auto& fnName = fnDeclaration->id.text;
        parent->addLocal(fnName);

        auto fnScope = new Scope{ .type = ScopeType::FUNCTION, .parent = parent };
        scopeInfo[node] = fnScope;
        fnScope->addLocal(fnName);

        for (auto& parameter : fnDeclaration->parameters)
        {
            fnScope->addLocal(parameter.text);
        }

        if (fnDeclaration->variadicParameter)
        {
            fnScope->addLocal(fnDeclaration->variadicParameter.value().text);
        }

        for (auto& defaultParameter : fnDeclaration->defaultParameters)
        {
            fnScope->addLocal(defaultParameter.first.text);
        }

        _analyze(fnDeclaration->block, fnScope);
        break;
    }
    case RETURN_STATEMENT:
    {
        auto returnStatement = (parser::ReturnStatement*)node;
        if (returnStatement->returnValue)
        {
            _analyze(returnStatement->returnValue.value(), parent);
        }
        break;
    }
    case FOR_EACH_STATEMET:
    {
        auto forEach = (parser::ForEachStatement*)node;
        parent->addLocal(forEach->variable.text);
        _analyze(forEach->block, parent);
        break;
    }
    case ASSIGNMENT_EXPRESSION:
    {
        auto assignmentExpression = (parser::AssignmentExpression*)node;
        parent->maybePromote(assignmentExpression->id.text);
        _analyze(assignmentExpression->expression, parent);
        break;
    }
    case BINARY_EXPRESSION:
    {
        auto binaryExpression = (parser::BinaryExpression*)node;
        _analyze(binaryExpression->right, parent);
        _analyze(binaryExpression->left, parent);
        break;
    }
    case UNARY_EXPRESSION:
    {
        auto unaryExpression = (parser::UnaryExpression*)node;
        _analyze(unaryExpression->operand, parent);
        break;
    }
    case PARENTHESIZED_EXPRESSION:
    {
        auto parenthesizedExpression = (parser::ParenthesizedExpression*)node;
        _analyze(parenthesizedExpression->expression, parent);
        break;
    }
    case VARIABLE_EXPRESSION:
    {
        auto variable = (parser::VariableExpression*)node;
        parent->maybePromote(variable->variable.text);
        break;
    }
    case ATTRIBUTE_EXPRESSION:
    {
        auto attr = (parser::AttributeExpression*)node;
        _analyze(attr->expression, parent);
        break;
    }
    case CALL_EXPRESSION:
    {
        auto callExpression = (parser::CallExpression*)node;
        _analyze(callExpression->callable, parent);
        for (auto argument : callExpression->arguments)
        {
            _analyze(argument, parent);
        }
        break;
    }
    case CONTINUE_STATEMENT:
    case LITERAL_EXPRESSION:
    case BREAK_STATEMENT:
        break;
    default:
        plog::fatal << "Неможливо обробити вузол \""
                << parser::stringEnum::enumToString(node->kind) << "\"";
    }
}

ScopeAnalyzer::scope_info_t ScopeAnalyzer::analyze()
{
    auto rootScope = new Scope{ .type = ScopeType::GLOBAL, .parent = nullptr };

    scopeInfo[rootNode] = rootScope;
    _analyze(rootNode, rootScope);
    return scopeInfo;
}

ScopeAnalyzer::ScopeAnalyzer(parser::BlockStatement* root) : rootNode(root)
{
}
