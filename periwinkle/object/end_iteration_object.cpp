﻿#include "end_iteration_object.hpp"
#include "string_object.hpp"
#include "native_function_object.hpp"

using namespace vm;

static Object* endIterToString(Object* a)
{
    return StringObject::create(endIterObjectType.name);
}

namespace vm
{
    TypeObject endIterObjectType =
    {
        .base = &objectObjectType,
        .name = "КінецьІтерації",
        .alloc = DEFAULT_ALLOC(EndIterObject),
        .operators =
        {
            .toString = endIterToString,
        },
    };

    EndIterObject P_endIter{ {.objectType = &endIterObjectType} };
}
