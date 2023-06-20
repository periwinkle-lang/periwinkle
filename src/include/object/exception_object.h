#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.h"

namespace vm
{
    extern TypeObject ExceptionObjectType;
    extern TypeObject NameErrorObjectType;
    extern TypeObject TypeErrorObjectType;
    extern TypeObject NotImplementedErrorObjectType;
    extern TypeObject AttributeErrorObjectType;
    extern TypeObject IndexErrorObjectType;
    extern TypeObject DivisionByZeroErrorObjectType;

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

    struct IndexErrorObject : ExceptionObject
    {
    };

    struct DivisionByZeroErrorObject : ExceptionObject
    {
    };

    extern NotImplementedErrorObject P_NotImplemented;
}

#endif
