#include "code_object.hpp"

using namespace vm;

static void traverse(CodeObject* codeObject)
{
    for (auto o : codeObject->constants)
    {
        mark(o);
    }
}

namespace vm
{
    TypeObject codeObjectType =
    {
        .base = nullptr,
        .name = "ОбєктКоду",
        .size = sizeof(CodeObject),
        .alloc = DEFAULT_ALLOC(CodeObject),
        .dealloc = DEFAULT_DEALLOC(CodeObject),
        .traverse = (traverseFunction)traverse,
    };

    struct ExceptionHandler;
}


std::optional<ExceptionHandler*> vm::CodeObject::getExceptionHandler(WORD ip)
{
    for (auto& handler : exceptionHandlers) {
        if (ip > handler.startAddress && ip < handler.endAddress) {
            return &handler;
        }
    }
    return std::nullopt;
}

ExceptionHandler* vm::CodeObject::getHandlerByStartIp(WORD ip)
{
    for (auto& handler : exceptionHandlers)
    {
        if (handler.startAddress == ip)
        {
            return &handler;
        }
    }
    return nullptr;
}

ExceptionHandler* vm::CodeObject::getHandlerByEndIp(WORD ip)
{
    for (auto& handler : exceptionHandlers)
    {
        if (handler.endAddress == ip)
        {
            return &handler;
        }
    }
    return nullptr;
}

CodeObject* vm::CodeObject::create(std::string name)
{
    auto codeObject = (CodeObject*)allocObject(&codeObjectType);
    codeObject->name = name;
    return codeObject;
}
