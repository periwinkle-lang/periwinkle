#include "code_object.h"

using namespace vm;

namespace vm
{
    TypeObject codeObjectType =
    {
        .base = nullptr,
        .name = "ОбєктКоду",
        .type = ObjectTypes::CODE,
        .alloc = DEFAULT_ALLOC(CodeObject),
    };
}

CodeObject* vm::CodeObject::create(std::string name)
{
    auto codeObject = (CodeObject*)allocObject(&codeObjectType);
    codeObject->name = name;
    return codeObject;
}
