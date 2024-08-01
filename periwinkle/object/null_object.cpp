#include "null_object.hpp"
#include "string_object.hpp"
#include "native_function_object.hpp"

using namespace vm;

static StringObject strNull = { {.objectType = &stringObjectType}, U"ніц" };

static Object* nullToString(Object* a)
{
    return &strNull;
}

namespace vm
{
    TypeObject nullObjectType =
    {
        .base = &objectObjectType,
        .name = "Ніц",
        .size = sizeof(NullObject),
        .alloc = DEFAULT_ALLOC(NullObject),
        .dealloc = DEFAULT_DEALLOC(NullObject),
        .operators =
        {
            .toString = nullToString,
        },
    };

    NullObject P_null { {.objectType = &nullObjectType} };
}
