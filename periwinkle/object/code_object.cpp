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
}

CodeObject* vm::CodeObject::create(std::string name)
{
    auto codeObject = (CodeObject*)allocObject(&codeObjectType);
    codeObject->name = name;
    return codeObject;
}
