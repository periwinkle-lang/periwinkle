#include "string_object.h"
#include "native_function_object.h"

using namespace vm;

static Object* strAdd(Object* a, Object* b)
{
    auto str1 = (StringObject*)a;
    auto str2 = (StringObject*)b;
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
            .add = strAdd,
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
