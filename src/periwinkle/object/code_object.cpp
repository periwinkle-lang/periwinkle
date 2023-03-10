#include "code_object.h"

using namespace vm;

Object* allocCodeObject();

namespace vm
{
    ObjectType codeObjectType =
    {
        .base = nullptr,
        .name = "CodeObject",
        .type = ObjectTypes::CODE,
        .alloc = &allocCodeObject,
    };
}

Object* allocCodeObject()
{
    auto codeObject = new CodeObject;
    codeObject->objectType = &codeObjectType;
    return codeObject;
}

CodeObject* vm::CodeObject::create(std::string name)
{
    auto codeObject = (CodeObject*)allocObject(&codeObjectType);
    codeObject->name = name;
    return codeObject;
}
