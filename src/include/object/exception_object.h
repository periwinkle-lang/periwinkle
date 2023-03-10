#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType ExceptionObjectType;
    extern ObjectType NameErrorObjectType;
    extern ObjectType TypeErrorObjectType;
    extern ObjectType NotImplementedErrorObjectType;
    extern ObjectType AttributeErrorObjectType;

    struct ExceptionObject : Object
    {
        std::string message; // Повідомлення винятку
    };

    struct NameErrorObject : ExceptionObject
    {
    };

    struct TypeErrorObject : ExceptionObject
    {
    };

    struct NotImplementedErrorObject : ExceptionObject
    {
    };

    struct AttributeErrorObject : ExceptionObject
    {
    };

    extern NotImplementedErrorObject P_NotImplemented;
}

#endif
