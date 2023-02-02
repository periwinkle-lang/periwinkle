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
}

Object* allocBoolObject()
{
    auto boolObject = new BoolObject;
    boolObject->objectType = &boolObjectType;
    return (Object*)boolObject;
}

BoolObject* vm::BoolObject::create(bool value)
{
    auto boolObject = (BoolObject*)allocObject(&boolObjectType);
    boolObject->value = value;
    return boolObject;
}
