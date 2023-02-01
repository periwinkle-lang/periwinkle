#include "utils.h"
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

static Object* nameErrorToString(Object* a)
{
    auto nameError = (NameErrorObject*)a;
    auto str = utils::format(nameError->message, nameError->name.c_str());
    return StringObject::create(str);
}

static Object* typeErrorToString(Object* a)
{
    auto typeError = (TypeErrorObject*)a;
    auto str = utils::format(typeError->message, typeError->name.c_str());
    return StringObject::create(str);
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
        new ObjectOperators{ .toString = nameErrorToString });

    EXCEPTION_EXTEND(ExceptionObjectType, TypeError, "ПомилкаТипу",
        new ObjectOperators{ .toString = typeErrorToString });
}

Object* allocExceptionObject()
{
    auto exceptionObject = new ExceptionObject;
    exceptionObject->objectType = &ExceptionObjectType;
    return (Object*)exceptionObject;
}

NameErrorObject* NameErrorObject::create(std::string message, std::string name)
{
    auto nameError = (NameErrorObject*)allocObject(&NameErrorObjectType);
    nameError->message = message;
    nameError->name = name;
    return nameError;
}

TypeErrorObject* TypeErrorObject::create(std::string message, std::string name)
{
    auto typeError = (TypeErrorObject*)allocObject(&TypeErrorObjectType);
    typeError->message = message;
    typeError->name = name;
    return typeError;
}
