#include "string_object.h"
#include "native_function_object.h"

using namespace vm;

Object* strAdd(Object* args[])
{
    auto str1 = (StringObject*)args[0];
    auto str2 = (StringObject*)args[1];
    return StringObject::create(str1->value + str2->value);
}

Object* allocStringObject();

namespace vm
{
    ObjectType stringObjectType =
    {
        .base = &objectObjectType,
        .name = "String",
        .type = ObjectTypes::STRING,
        .alloc = &allocStringObject,
        .operators = new ObjectOperators
        {
            .add = NativeFunctionObject::create(2, "add", strAdd),
        },
    };
}

Object* allocStringObject()
{
    auto stringObject = new StringObject;
    stringObject->objectType = &stringObjectType;
    return (Object*)stringObject;
}

StringObject* vm::StringObject::create(std::string value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = value;
    return stringObject;
}
