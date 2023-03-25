#include "exception_object.h"
#include "string_object.h"

using namespace vm;

#define EXCEPTION_EXTEND(baseType, exc, excName, excOperators) \
    TypeObject exc##ObjectType =                        \
    {                                                   \
        .base = &baseType,                              \
        .name = excName,                                \
        .type = ObjectTypes::EXCEPTION,                 \
        .alloc = DEFAULT_ALLOC(exc##Object),            \
        .operators = excOperators,                      \
    };

static Object* exceptionToString(Object* a)
{
    auto exception = (ExceptionObject*)a;
    return StringObject::create(exception->message);
}

namespace vm
{
    TypeObject ExceptionObjectType =
    {
        .base = &objectObjectType,
        .name = "Виняток",
        .type = ObjectTypes::EXCEPTION,
        .alloc = DEFAULT_ALLOC(ExceptionObject),
        .operators =
        {
            .toString = exceptionToString,
        },
    };

    EXCEPTION_EXTEND(ExceptionObjectType, NameError, "ПомилкаІмені",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, TypeError, "ПомилкаТипу",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, NotImplementedError, "ПомилкаРеалізації",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, AttributeError, "ПомилкаАтрибута",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, IndexError, "ПомилкаІндексу",
        { .toString = exceptionToString });

    NotImplementedErrorObject P_NotImplemented{ {{.objectType = &NotImplementedErrorObjectType}} };
}
