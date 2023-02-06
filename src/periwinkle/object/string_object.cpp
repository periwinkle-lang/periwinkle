#include "string_object.h"
#include "native_function_object.h"
#include "exception_object.h"

using namespace vm;

#define CHECK_STRING(object)                             \
    if (object->objectType->type != ObjectTypes::STRING) \
       return &P_NotImplemented;

static Object* strAdd(Object* a, Object* b)
{
    CHECK_STRING(a);
    CHECK_STRING(b);
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
