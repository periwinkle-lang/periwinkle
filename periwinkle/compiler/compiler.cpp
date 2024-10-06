#include <algorithm>
#include <variant>
#include <unordered_set>
#include <format>

#include "compiler.hpp"
#include "code_object.hpp"
#include "bool_object.hpp"
#include "int_object.hpp"
#include "string_object.hpp"
#include "real_object.hpp"
#include "null_object.hpp"
#include "string_vector_object.hpp"
#include "types.hpp"
#include "plogger.hpp"
#include "utils.hpp"
#include "keyword.hpp"
#include "periwinkle.hpp"
#include "unicode.hpp"

using namespace compiler;
using namespace ast;
using enum vm::OpCode;
using enum ast::NodeKind;

#define FIND_CONST_IDX(OBJECT, OBJECT_TYPE)                               \
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i) \
    {                                                                     \
        auto constant = codeObject->constants[i];                         \
        if (OBJECT_IS(constant, &vm::OBJECT_TYPE) == false)               \
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
#define PUSH_SCOPE(node) scopeStack.push_back(scopeInfo[node])
#define SCOPE_POP() scopeStack.pop_back()
#define SCOPE_BACK() scopeStack.back()

struct LoopState : CompilerState
{
    vm::WORD startIp; // Початок циклу
    // Адреси, що будуть будуть змінені на адресу кінці циклу
    std::vector<vm::WORD> addressesForPatchWithEndBlock;
};

struct FunctionState : CompilerState
{
    vm::CodeObject* codeObject;
};

#define PUSH_LOOP_STATE(startIp) \
    stateStack.push_back(new LoopState{{CompilerStateType::LOOP}, startIp})

#define PUSH_FUNCTION_STATE(codeObject) \
    stateStack.push_back(new FunctionState{{CompilerStateType::FUNCTION}, codeObject})


vm::Frame* compiler::Compiler::compile()
{
    ScopeAnalyzer scopeAnalyzer(root);
    scopeInfo = scopeAnalyzer.analyze();
    PUSH_SCOPE(root);
    for (auto statement = root->statements.begin(); statement != root->statements.end(); ++statement)
    {
        isRootBlock = true;
        isLastStatementInBlock = statement == root->statements.end() - 1;
        compileStatement(*statement);
    }
    SCOPE_POP();
    if (!isRootBlockHasReturn)
    {
        emitOpCode(LOAD_CONST, nullConstIdx());
        emitOpCode(RETURN);
    }
    auto frame = new vm::Frame;
    frame->codeObject = codeObject;
    frame->globals = new vm::Frame::object_map_t;
    codeObject->source = source;
    return frame;
}

void compiler::Compiler::compileBlock(BlockStatement* block)
{
    isRootBlock = false;
    for (auto statement : block->statements)
    {
        compileStatement(statement);
    }
}

void compiler::Compiler::compileStatement(Statement* statement)
{
    switch (statement->kind)
    {
    case BLOCK_STATEMENT:
        compileBlock((BlockStatement*)statement);
        break;
    case EXPRESSION_STATEMENT:
    {
        auto expStatement = (ExpressionStatement*)statement;
        compileExpressionStatement(expStatement);

        // Очищення стека, якщо значення виразу нікуди не присвоюється.
        // Окрім виразу присвоєння значення змінній,
        // так як він не залишає нічого на стеку після себе
        if (expStatement->expression->kind != ASSIGNMENT_EXPRESSION)
        {
            if (isRootBlock && isLastStatementInBlock)
            {
                emitOpCode(RETURN);
                isRootBlockHasReturn = true;
            }
            else
                emitOpCode(POP);
        }
        break;
    }
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
    case FUNCTION_STATEMENT:
        compileFunctionDeclaration((FunctionDeclaration*)statement);
        break;
    case RETURN_STATEMENT:
        compileReturnStatement((ReturnStatement*)statement);
        break;
    case FOR_EACH_STATEMENT:
        compileForEachStatement((ForEachStatement*)statement);
        break;
    case TRY_CATCH_STATEMENT:
        compileTryCatchStatement((TryCatchStatement*)statement);
        break;
    case RAISE_STATEMENT:
        compileRaiseStatement((RaiseStatement*)statement);
        break;
    default:
        plog::fatal << "Неможливо обробити вузол \""
            << ast::stringEnum::enumToString(statement->kind) << "\"";
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
    auto endWhileBlock = emitOpCode(JMP_IF_FALSE, 0);
    PUSH_LOOP_STATE(startWhileAddress);
    compileBlock(statement->block);
    emitOpCode(JMP, startWhileAddress);
    patchJumpAddress(endWhileBlock, getOffset());
    for (auto address : STATE_BACK(LoopState)->addressesForPatchWithEndBlock)
    {
        patchJumpAddress(address, getOffset());
    }
    STATE_POP();
}

void compiler::Compiler::compileBreakStatement(BreakStatement* statement)
{
    auto state = (LoopState*)unwindStateStack(CompilerStateType::LOOP);
    if (state)
    {
        setLineno(statement->break_);
        auto endBlock = emitOpCode(JMP, 0);
        state->addressesForPatchWithEndBlock.push_back(endBlock);
    }
    else
    {
        throwCompileError("Оператор \"завершити\" знаходиться поза циклом", statement->break_);
    }
}

void compiler::Compiler::compileContinueStatement(ContinueStatement* statement)
{
    auto state = (LoopState*)unwindStateStack(CompilerStateType::LOOP);
    if (state)
    {
        setLineno(statement->continue_);
        emitOpCode(JMP, state->startIp);
    }
    else
    {
        throwCompileError("Оператор \"пропустити\" знаходиться поза циклом", statement->continue_);
    }
}

void compiler::Compiler::compileIfStatement(IfStatement* statement)
{
    compileExpression(statement->condition);
    auto endIfBlock = emitOpCode(JMP_IF_FALSE, 0);
    compileBlock(statement->block);
    if (!statement->elseOrIf)
    {
        patchJumpAddress(endIfBlock, getOffset());
    }
    else
    {
        auto endIfElseBlock = emitOpCode(JMP, 0);
        patchJumpAddress(endIfBlock, getOffset());

        auto elseOrIf = statement->elseOrIf.value();
        if (elseOrIf->kind == ELSE_STATEMENT)
        {
            auto elseStatement = (ElseStatement*)elseOrIf;
            compileBlock(elseStatement->block);
        }
        else if (elseOrIf->kind == IF_STATEMENT)
        {
            auto ifStatement = (IfStatement*)elseOrIf;
            compileIfStatement(ifStatement);
        }

        patchJumpAddress(endIfElseBlock, getOffset());
    }
}

static std::optional<Token> checkDuplicateParameters(ast::FunctionDeclaration* func) {
    std::unordered_set<std::string> parameterNames;

    for (const auto& param : func->parameters) {
        if (parameterNames.find(param.text) != parameterNames.end()) {
            return param;
        }
        parameterNames.insert(param.text);
    }

    if (func->variadicParameter.has_value()) {
        const Token& variadicParam = func->variadicParameter.value();
        if (parameterNames.find(variadicParam.text) != parameterNames.end()) {
            return variadicParam;
        }
        parameterNames.insert(variadicParam.text);
    }

    for (const auto& defaultParam : func->defaultParameters) {
        const Token& param = defaultParam.first;
        if (parameterNames.find(param.text) != parameterNames.end()) {
            return param;
        }
        parameterNames.insert(param.text);
    }

    return std::nullopt;
}

void compiler::Compiler::compileFunctionDeclaration(ast::FunctionDeclaration* statement)
{
    if (auto param = checkDuplicateParameters(statement)) {
        throwCompileError(std::format(
            "Параметр з ім'ям \"{}\" повторюється", param.value().text),
            param.value());
    }
    auto& name = statement->id.text;
    auto fnCodeObject = vm::CodeObject::create(name);
    auto prevCodeObject = codeObject;
    codeObject = fnCodeObject;
    PUSH_FUNCTION_STATE(fnCodeObject);
    PUSH_SCOPE(statement);

    codeObject->source = source;
    codeObject->locals = SCOPE_BACK()->locals;
    codeObject->cells = SCOPE_BACK()->cells;
    codeObject->freevars = SCOPE_BACK()->freeVariables;

    for (auto& argName : codeObject->locals)
    {
        for (auto& cellName : codeObject->cells)
        {
            if (argName == cellName)
            {
                codeObject->argsAsCells.push_back(argName);
            }
        }
    }

    codeObject->arity = statement->parameters.size() + statement->defaultParameters.size();
    if (statement->variadicParameter)
        codeObject->isVariadic = true;
    compileBlock(statement->block);
    emitOpCode(LOAD_CONST, nullConstIdx());
    emitOpCode(RETURN);

    SCOPE_POP();
    STATE_POP();
    codeObject = prevCodeObject;

    for (auto& defaultParameter : statement->defaultParameters)
    {
        fnCodeObject->defaults.push_back(defaultParameter.first.text);
        setLineno(defaultParameter.first);
        compileExpression(defaultParameter.second);
    }

    setLineno(statement->id);
    if (fnCodeObject->freevars.size() > 0)
    {
        auto state = (FunctionState*)unwindStateStack(CompilerStateType::FUNCTION);
        auto& cells = state->codeObject->cells;

        for (auto& name : state->codeObject->cells)
        {
            auto cellIdx = std::find(cells.begin(), cells.end(), name);
            emitOpCode(GET_CELL, cellIdx - cells.begin());
        }
    }

    codeObject->constants.push_back(fnCodeObject);
    emitOpCode(LOAD_CONST, codeObject->constants.size() - 1);
    emitOpCode(MAKE_FUNCTION);
    compileNameSet(name);
}

void compiler::Compiler::compileReturnStatement(ast::ReturnStatement* statement)
{
    auto state = (FunctionState*)unwindStateStack(CompilerStateType::FUNCTION);
    if (state)
    {
        setLineno(statement->return_);
        if (statement->returnValue)
        {
            compileExpression(statement->returnValue.value());
        }
        else
        {
            emitOpCode(LOAD_CONST, nullConstIdx());
        }
        emitOpCode(RETURN);
    }
    else
    {
        throwCompileError("Оператор \"повернути\" знаходиться поза функцією", statement->return_);
    }
}

void compiler::Compiler::compileForEachStatement(ForEachStatement* statement)
{
    compileExpression(statement->expression);
    setLineno(statement->forEach);
    emitOpCode(UNARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::GET_ITER));
    auto startForEachAddress = getOffset();
    auto endForEachBlock = emitOpCode(FOR_EACH, 0);
    setLineno(statement->variable);
    compileNameSet(statement->variable.text);
    PUSH_LOOP_STATE(startForEachAddress);
    compileBlock(statement->block);
    emitOpCode(JMP, startForEachAddress);
    patchJumpAddress(endForEachBlock, getOffset());
    for (auto address : STATE_BACK(LoopState)->addressesForPatchWithEndBlock)
    {
        patchJumpAddress(address, getOffset());
    }
    STATE_POP();
}

void compiler::Compiler::compileTryCatchStatement(TryCatchStatement* statement)
{
    vm::ExceptionHandler excHandler{};
    excHandler.startAddress = getOffset();
    setLineno(statement->try_);
    emitOpCode(TRY);
    compileBlock(statement->block);
    std::vector<vm::WORD> ends;
    ends.reserve(statement->catchBlocks.size()
        + 1 // Для JMP в блоці TRY
        - 1 // Останньому обробнику не потрібен JMP
    );
    ends.push_back(emitOpCode(JMP, 0));
    excHandler.firstHandlerAddress = getOffset();
    for (auto i = statement->catchBlocks.cbegin(); i != statement->catchBlocks.cend(); ++i)
    {
        auto catchBlock = *i;
        setLineno(catchBlock->exceptionName);
        compileNameGet(catchBlock->exceptionName.text);
        setLineno(catchBlock->catch_);
        auto endCatchBlock = emitOpCode(CATCH, 0);
        if (catchBlock->variableName.has_value())
        {
            setLineno(catchBlock->as.value());
            compileNameSet(catchBlock->variableName.value().text);
        }
        else
        {
            emitOpCode(POP);
        }
        compileBlock(catchBlock->block);
        if (catchBlock->variableName.has_value())
        {
            compileNameDelete(catchBlock->variableName.value().text);
        }
        patchJumpAddress(endCatchBlock, getOffset());
        if (i != statement->catchBlocks.cend())
        {
            ends.push_back(emitOpCode(JMP, 0));
        }
    }
    for (auto a : ends)
    {
        patchJumpAddress(a, getOffset());
    }
    if (statement->finallyBlock.has_value())
    {
        excHandler.finallyAddress = getOffset();
        auto finallyBlock = statement->finallyBlock.value();
        setLineno(finallyBlock->finally_);
        compileBlock(finallyBlock->block);
    }
    excHandler.endAddress = getOffset();
    emitOpCode(END_TRY);
    codeObject->exceptionHandlers.push_back(excHandler);
}

void compiler::Compiler::compileRaiseStatement(RaiseStatement* statement)
{
    compileExpression(statement->exception);
    emitOpCode(RAISE);
}

void compiler::Compiler::compileExpression(Expression* expression)
{
    switch (expression->kind)
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
    case UNARY_EXPRESSION:
        compileUnaryExpression((UnaryExpression*)expression);
        break;
    case PARENTHESIZED_EXPRESSION:
        compileParenthesizedExpression((ParenthesizedExpression*)expression);
        break;
    case ATTRIBUTE_EXPRESSION:
        compileAttributeExpression((AttributeExpression*)expression);
        break;
    default:
        plog::fatal << "Неможливо обробити вузол \""
            << ast::stringEnum::enumToString(expression->kind) << "\"";
    }
}

void compiler::Compiler::compileAssignmentExpression(AssignmentExpression* expression)
{
    std::string name = expression->id.text;
    compileExpression(expression->expression);
    auto& op = expression->assignment.text;
    if (op == Keyword::EQUAL)
    {
        setLineno(expression->assignment);
        compileNameSet(name);
        return;
    }

    setLineno(expression->id);
    compileNameGet(name);

    setLineno(expression->assignment);
    vm::ObjectOperatorOffset operatorOffset;
    if      (op == Keyword::ADD_EQUAL) operatorOffset = vm::ObjectOperatorOffset::ADD;
    else if (op == Keyword::SUB_EQUAL) operatorOffset = vm::ObjectOperatorOffset::SUB;
    else if (op == Keyword::MUL_EQUAL) operatorOffset = vm::ObjectOperatorOffset::MUL;
    else if (op == Keyword::DIV_EQUAL) operatorOffset = vm::ObjectOperatorOffset::DIV;
    else if (op == Keyword::MOD_EQUAL) operatorOffset = vm::ObjectOperatorOffset::MOD;
    else if (op == Keyword::FLOOR_DIV_EQUAL) operatorOffset = vm::ObjectOperatorOffset::FLOOR_DIV;
    else plog::fatal << "Неправильний оператор присвоєння: \"" << op << "\"";
    emitOpCode(BINARY_OP, static_cast<vm::WORD>(operatorOffset));
    compileNameSet(name);
}

void compiler::Compiler::compileLiteralExpression(LiteralExpression* expression)
{
    vm::WORD index;
    setLineno(expression->literalToken);
    using enum LiteralExpression::Type;
    switch (expression->literalType)
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
        auto parsedString = unicode::toUtf32(std::get<std::string>(expression->value));
        size_t j = 0; // Індекс для запису в результат
        for (size_t i = 0; i < parsedString.length(); ++i) {
            if (parsedString[i] == U'\\' && i + 1 < parsedString.length()) {
                char32_t nextChar = parsedString[i + 1];
                switch (nextChar) {
                    case U'"':
                        parsedString[j++] = U'"';
                        ++i;
                        break;
                    case U'\\':
                        parsedString[j++] = U'\\';
                        ++i;
                        break;
                    case U'a': // англійська версія
                    case U'а': // українська
                        parsedString[j++] = U'\a';
                        ++i;
                        break;
                    case U'b':
                    case U'б':
                        parsedString[j++] = U'\b';
                        ++i;
                        break;
                    case U'f':
                    case U'ф':
                        parsedString[j++] = U'\f';
                        ++i;
                        break;
                    case U'n':
                    case U'н':
                        parsedString[j++] = U'\n';
                        ++i;
                        break;
                    case U'r':
                    case U'р':
                        parsedString[j++] = U'\r';
                        ++i;
                        break;
                    case U't':
                    case U'т':
                        parsedString[j++] = U'\t';
                        ++i;
                        break;
                    case U'v':
                    case U'в':
                        parsedString[j++] = U'\v';
                        ++i;
                        break;
                    case U'0':
                        parsedString[j++] = U'\0';
                        ++i;
                        break;
                    default:
                        throwCompileError(
                            std::format("\\{} не є керувальною послідовністю",
                                unicode::toUtf8(std::u32string_view(&parsedString[i + 1], 1))),
                            expression->literalToken
                        );
                }
            } else {
                parsedString[j++] = parsedString[i];
            }
        }
        parsedString.resize(j);
        index = stringConstIdx(parsedString);
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
    emitOpCode(LOAD_CONST, index);
}

void compiler::Compiler::compileVariableExpression(VariableExpression* expression)
{
    auto& variableName = expression->variable.text;
    setLineno(expression->variable);
    compileNameGet(variableName);
}

void compiler::Compiler::compileCallExpression(CallExpression* expression)
{
    auto argc = (vm::WORD)expression->arguments.size() + (vm::WORD)expression->namedArguments.size();
    bool withNamedArgs = (bool)expression->namedArguments.size();
    vm::WORD namedArgsIdx;

    if (expression->callable->kind == NodeKind::ATTRIBUTE_EXPRESSION)
    {
        // Якщо викликним виразом є атрибут,
        // то потрібно отримувати значення за допомогою LOAD_METHOD
        compileAttributeExpression((AttributeExpression*)expression->callable, true);
    }
    else
    {
        compileExpression(expression->callable);
    }

    for (auto argument : expression->arguments)
    {
        compileExpression(argument);
    }

    if (withNamedArgs)
    {
        std::vector<std::string> namedArgs;
        for (auto& namedArgument : expression->namedArguments)
        {
            for (auto& a: namedArgs)
            {
               if (namedArgument.first.text == a)
               {
                   throwCompileError(
                       std::format(
                           "Іменований параметр \"{}\" повторюється", a),
                       namedArgument.first
                   );
               }
            }

            namedArgs.push_back(namedArgument.first.text);
            compileExpression(namedArgument.second);
        }

        namedArgsIdx = stringVectorIdx(namedArgs);
    }

    // Якщо викликний вираз є атрибутом,
    // то викликати його треба за допомогою CALL_METHOD
    if (expression->callable->kind == NodeKind::ATTRIBUTE_EXPRESSION)
    {
        emitOpCode(withNamedArgs ? CALL_METHOD_NA : CALL_METHOD, argc);
    }
    else
    {
        emitOpCode(withNamedArgs ? CALL_NA : CALL, argc);
    }

    if (withNamedArgs)
    {
        emitOperand(namedArgsIdx);
    }
}

void compiler::Compiler::compileBinaryExpression(BinaryExpression* expression)
{
    using enum vm::ObjectCompOperator;
    auto& op = expression->op.text;

    if (op == Keyword::AND || op == Keyword::OR)
    {
        compileExpression(expression->left);
        setLineno(expression->op);
        auto end = emitOpCode(op == Keyword::AND ? JMP_IF_FALSE_OR_POP : JMP_IF_TRUE_OR_POP, 0);
        compileExpression(expression->right);
        patchJumpAddress(end, getOffset());
        return;
    }
    compileExpression(expression->right);
    compileExpression(expression->left);
    setLineno(expression->op);

    if      (op == Keyword::ADD) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::ADD));
    else if (op == Keyword::SUB) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::SUB));
    else if (op == Keyword::DIV) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::DIV));
    else if (op == Keyword::MUL) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::MUL));
    else if (op == Keyword::MOD) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::MOD));
    else if (op == Keyword::FLOOR_DIV) emitOpCode(BINARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::FLOOR_DIV));
    else if (op == Keyword::IS) emitOpCode(IS);
    else if (op == Keyword::IS_NOT) emitOpCode(IS, 1);
    else if (op == Keyword::EQUAL_EQUAL) emitOpCode(COMPARE, static_cast<vm::WORD>(EQ));
    else if (op == Keyword::NOT_EQUAL) emitOpCode(COMPARE, static_cast<vm::WORD>(NE));
    else if (op == Keyword::GREATER) emitOpCode(COMPARE, static_cast<vm::WORD>(GT));
    else if (op == Keyword::GREATER_EQUAL) emitOpCode(COMPARE, static_cast<vm::WORD>(GE));
    else if (op == Keyword::LESS) emitOpCode(COMPARE, static_cast<vm::WORD>(LT));
    else if (op == Keyword::LESS_EQUAL) emitOpCode(COMPARE, static_cast<vm::WORD>(LE));
    else plog::fatal << "Неправильний токен оператора: \"" << op << "\"";
}

void compiler::Compiler::compileUnaryExpression(UnaryExpression* expression)
{
    compileExpression(expression->operand);
    setLineno(expression->op);

    auto& op = expression->op.text;
    if (op == Keyword::ADD) emitOpCode(UNARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::POS));
    else if (op == Keyword::SUB) emitOpCode(UNARY_OP, static_cast<vm::WORD>(vm::ObjectOperatorOffset::NEG));
    else if (op == Keyword::NOT) emitOpCode(NOT);
    else plog::fatal << "Неправильний токен унарного оператора: \"" << op << "\"";
}

void compiler::Compiler::compileParenthesizedExpression(ParenthesizedExpression* expression)
{
    compileExpression(expression->expression);
}

void compiler::Compiler::compileAttributeExpression(
    AttributeExpression* expression, bool isMethod)
{
    compileExpression(expression->expression);
    setLineno(expression->attribute);
    emitOpCode(isMethod ? LOAD_METHOD : GET_ATTR, nameIdx(expression->attribute.text));
}

void compiler::Compiler::compileNameGet(const std::string& name)
{
    auto scope = SCOPE_BACK();
    auto varGetter = scope->getVarGetter(name);

    vm::WORD index;
    switch(varGetter)
    {
    case LOAD_LOCAL: index = localIdx(name); break;
    case LOAD_CELL: index = freeIdx(name); break;
    case LOAD_GLOBAL: index = nameIdx(name); break;
    default:
        plog::fatal << "Невідомний varGetter";
    }
    emitOpCode(varGetter, index);
}

void compiler::Compiler::compileNameSet(const std::string& name)
{
    auto scope = SCOPE_BACK();
    auto varSetter = scope->getVarSetter(name);

    vm::WORD index;
    switch(varSetter)
    {
    case STORE_LOCAL: index = localIdx(name); break;
    case STORE_CELL: index = freeIdx(name); break;
    case STORE_GLOBAL: index = nameIdx(name); break;
    default:
        plog::fatal << "Невідомний varSetter";
    }
    emitOpCode(varSetter, index);
}

void compiler::Compiler::compileNameDelete(const std::string& name)
{
    auto scope = SCOPE_BACK();
    auto varDeleter = scope->getVarDeleter(name);

    vm::WORD index;
    switch(varDeleter)
    {
    case DELETE_LOCAL: index = localIdx(name); break;
    case DELETE_GLOBAL: index = nameIdx(name); break;
    default:
        plog::fatal << "Невідомний varDeleter";
    }
    emitOpCode(varDeleter, index);
}

CompilerState* compiler::Compiler::unwindStateStack(CompilerStateType type)
{
    for (auto it = stateStack.rbegin(); it != stateStack.rend(); ++it)
    {
        if ((*it)->type == type)
        {
            return *it;
        }
    }
    return nullptr;
}

vm::WORD compiler::Compiler::booleanConstIdx(bool value)
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        if (OBJECT_IS(codeObject->constants[i], &vm::boolObjectType) == false)
        {
            continue;
        }
        if (((vm::BoolObject*)codeObject->constants[i])->value == value)
        {
            return i;
        }
    }
    codeObject->constants.push_back(P_BOOL(value));
    return (vm::WORD)codeObject->constants.size() - 1;
}

vm::WORD compiler::Compiler::realConstIdx(double value)
{
    FIND_CONST_IDX(RealObject, realObjectType)
}

vm::WORD compiler::Compiler::integerConstIdx(i64 value)
{
    FIND_CONST_IDX(IntObject, intObjectType)
}

vm::WORD compiler::Compiler::stringVectorIdx(const std::vector<std::string>& value)
{
    FIND_CONST_IDX(StringVectorObject, stringVectorObjectType)
}

vm::WORD compiler::Compiler::stringConstIdx(const std::u32string& value)
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        auto constant = codeObject->constants[i];
        if (OBJECT_IS(constant, &vm::stringObjectType) == false)
        {
            continue;
        }
        else if (static_cast<vm::StringObject*>(constant)->value == value)
        {
            return i;
        }
    }
    codeObject->constants.push_back(vm::StringObject::create(value));
        return vm::WORD(codeObject->constants.size() - 1);
}

vm::WORD compiler::Compiler::nullConstIdx()
{
    for (vm::WORD i = 0; i < (vm::WORD)codeObject->constants.size(); ++i)
    {
        if (OBJECT_IS(codeObject->constants[i], &vm::nullObjectType))
        {
            return i;
        }
    }
    codeObject->constants.push_back(&vm::P_null);
    return (vm::WORD)codeObject->constants.size() - 1;
}

vm::WORD compiler::Compiler::freeIdx(const std::string& name)
{
    auto& cells = codeObject->cells;
    auto cellIdx = std::find(cells.begin(), cells.end(), name);
    if (cellIdx != cells.end())
    {
        return cellIdx - cells.begin();
    }
    else
    {
        auto& freevars = codeObject->freevars;
        auto freeIndex = std::find(freevars.begin(), freevars.end(), name);
        if (freeIndex != freevars.end())
        {
            return freeIndex - freevars.begin() + cells.size();
        }
        else
        {
            plog::fatal << "Неможливо знайти змінну \"" << name << "\"";
        }
    }
}

vm::WORD compiler::Compiler::localIdx(const std::string& name)
{
    auto& locals = codeObject->locals;
    auto nameIdx = std::find(locals.begin(), locals.end(), name);
    if (nameIdx != locals.end())
    {
        return nameIdx - locals.begin();
    }
    else
    {
        plog::fatal << "Локальної змінної \"" << name << "\" не існує";
    }
}

vm::WORD compiler::Compiler::nameIdx(const std::string& name)
{
    auto& names = codeObject->names;
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

void compiler::Compiler::throwCompileError(std::string message, Token token)
{
    utils::throwSyntaxError(source, message, token.lineno, token.col - 1);
    exit(1);
}

void compiler::Compiler::setLineno(Token token)
{
    currentLineno = (vm::WORD)token.lineno;
}

vm::WORD compiler::Compiler::emitOpCode(vm::OpCode op, vm::WORD operand)
{
    codeObject->code.push_back(static_cast<vm::WORD>(op) + (operand << 8));
    auto ip = vm::WORD(codeObject->code.size() - 1);
    codeObject->ipToLineno[ip] = currentLineno;
    return ip;
}

vm::WORD compiler::Compiler::emitOperand(vm::WORD operand)
{
    codeObject->code.push_back(operand);
    return vm::WORD(codeObject->code.size() - 1);
}

vm::WORD compiler::Compiler::getOffset()
{
    return (vm::WORD)codeObject->code.size();
}

void compiler::Compiler::patchJumpAddress(int offset, vm::WORD newAddress, bool isOp)
{
    if (isOp)
        codeObject->code[offset] |= newAddress << 8;
    else
        codeObject->code[offset] = newAddress;
}

compiler::Compiler::Compiler(BlockStatement* root, periwinkle::ProgramSource* source)
    :
    root(root),
    source(source)
{
    codeObject = vm::CodeObject::create("");
}
