#include "end_iteration_object.h"
#include "string_object.h"
#include "native_function_object.h"

using namespace vm;

static Object* endIterToString(Object* a)
{
    return StringObject::create(endIterObjectType.name);
}

Object* allocendIterObject();

namespace vm
{
    TypeObject endIterObjectType =
    {
        .base = &objectObjectType,
        .name = "КінецьІтерації",
        .type = ObjectTypes::END_ITERATION,
        .alloc = &allocendIterObject,
        .operators =
        {
            .toString = endIterToString,
        },
    };

    EndIterObject P_endIter{ {.objectType = &endIterObjectType} };
}

Object* allocendIterObject()
{
    auto endIterObject = new EndIterObject;
    endIterObject->objectType = &endIterObjectType;
    return (Object*)endIterObject;
}
