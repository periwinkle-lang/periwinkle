#include "bool_object.h"
#include "string_object.h"

using namespace vm;
extern ObjectType objectObjectType;

static Object* boolToString(Object* a)
{
    auto arg = (BoolObject*)a;
    return StringObject::create(arg->value ? "правда" : "брехня");
}

static Object* boolToBool(Object* a)
{
    return a;
}

Object* allocBoolObject();

namespace vm
{
    ObjectType boolObjectType =
    {
        .base = &objectObjectType,
        .name = "Bool",
        .type = ObjectTypes::BOOL,
        .alloc = &allocBoolObject,
        .operators = new ObjectOperators
        {
            .toString = boolToString,
            .toBool = boolToBool,
        }
    };

    BoolObject P_true  { {.objectType = &boolObjectType} };
    BoolObject P_false { {.objectType = &boolObjectType} };
}

Object* allocBoolObject()
{
    auto boolObject = new BoolObject;
    boolObject->objectType = &boolObjectType;
    return (Object*)boolObject;
}
