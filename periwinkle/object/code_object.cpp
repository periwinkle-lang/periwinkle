#include "code_object.hpp"

using namespace vm;

namespace vm
{
    TypeObject codeObjectType =
    {
        .base = nullptr,
        .name = "ОбєктКоду",
        .alloc = DEFAULT_ALLOC(CodeObject),
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
