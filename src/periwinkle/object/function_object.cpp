#include "function_object.h"

using namespace vm;

Object* allocFunction();

namespace vm
{
    ObjectType functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Function",
        .type = ObjectTypes::FUNCTION,
        .alloc = &allocFunction,
    };
}

Object* allocFunction()
{
    auto functionObject = new FunctionObject;
    functionObject->objectType = &functionObjectType;
    return (Object*)functionObject;
}
