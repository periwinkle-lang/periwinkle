#include "null_object.h"
#include "string_object.h"
#include "native_function_object.h"

using namespace vm;
extern ObjectType objectObjectType;

static Object* nullToString(Object* a)
{
    return StringObject::create("нич");
}

Object* allocNullObject();

namespace vm
{
    ObjectType nullObjectType =
    {
        .base = &objectObjectType,
        .name = "Null",
        .type = ObjectTypes::NULL_,
        .alloc = &allocNullObject,
        .operators =
        {
            .toString = nullToString,
        },
    };

    NullObject P_null { {.objectType = &nullObjectType} };
}

Object* allocNullObject()
{
    auto nullObject = new NullObject;
    nullObject->objectType = &nullObjectType;
    return (Object*)nullObject;
}
