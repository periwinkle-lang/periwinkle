#include "exception_object.h"
#include "string_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define EXCEPTION_EXTEND(baseType, exc, excName, excOperators) \
    Object* alloc##exc##Object();                       \
                                                        \
    ObjectType exc##ObjectType =                        \
    {                                                   \
        .base = &baseType,                              \
        .name = excName,                                \
        .type = ObjectTypes::EXCEPTION,                 \
        .alloc = &alloc##exc##Object,                   \
        .operators = excOperators,                      \
    };                                                  \
                                                        \
    Object* alloc##exc##Object()                        \
    {                                                   \
        auto exceptionObject = new exc##Object;         \
        exceptionObject->objectType = &exc##ObjectType; \
        return (Object*)exceptionObject;                \
    }

static Object* exceptionToString(Object* a)
{
    auto exception = (ExceptionObject*)a;
    return StringObject::create(exception->message);
}

Object* allocExceptionObject();

namespace vm
{
    ObjectType ExceptionObjectType =
    {
        .base = &objectObjectType,
        .name = "Виняток",
        .type = ObjectTypes::EXCEPTION,
        .alloc = &allocExceptionObject,
        .operators = new ObjectOperators
        {
            .toString = exceptionToString,
        },
    };

    EXCEPTION_EXTEND(ExceptionObjectType, NameError, "ПомилкаІмені",
        new ObjectOperators{ .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, TypeError, "ПомилкаТипу",
        new ObjectOperators{ .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, NotImplementedError, "ПомилкаРеалізації",
        new ObjectOperators{ .toString = exceptionToString });

    NotImplementedErrorObject P_NotImplemented{ {{.objectType = &NotImplementedErrorObjectType}} };
}

Object* allocExceptionObject()
{
    auto exceptionObject = new ExceptionObject;
    exceptionObject->objectType = &ExceptionObjectType;
    return (Object*)exceptionObject;
}
