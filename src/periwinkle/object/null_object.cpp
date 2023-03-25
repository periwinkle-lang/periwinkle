#include "null_object.h"
#include "string_object.h"
#include "native_function_object.h"

using namespace vm;

static Object* nullToString(Object* a)
{
    return StringObject::create("нич");
}

namespace vm
{
    TypeObject nullObjectType =
    {
        .base = &objectObjectType,
        .name = "Нич",
        .type = ObjectTypes::NULL_,
        .alloc = DEFAULT_ALLOC(NullObject),
        .operators =
        {
            .toString = nullToString,
        },
    };

    NullObject P_null { {.objectType = &nullObjectType} };
}
