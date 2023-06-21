#include "null_object.h"
#include "string_object.h"
#include "native_function_object.h"

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
        .alloc = DEFAULT_ALLOC(NullObject),
        .operators =
        {
            .toString = nullToString,
        },
    };

    NullObject P_null { {.objectType = &nullObjectType} };
}
