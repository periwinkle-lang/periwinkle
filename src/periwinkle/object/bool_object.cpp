#include "bool_object.h"

using namespace vm;
extern ObjectType objectObjectType;

Object* allocBoolObject();

namespace vm
{
    ObjectType boolObjectType =
    {
        .base = &objectObjectType,
        .name = "Bool",
        .type = ObjectTypes::BOOL,
        .alloc = &allocBoolObject,
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
