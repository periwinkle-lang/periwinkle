﻿#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.hpp"

namespace vm
{
    extern TypeObject ExceptionObjectType;
    extern TypeObject NameErrorObjectType;
    extern TypeObject TypeErrorObjectType;
    extern TypeObject NotImplementedErrorObjectType;
    extern TypeObject AttributeErrorObjectType;
    extern TypeObject IndexErrorObjectType;
    extern TypeObject DivisionByZeroErrorObjectType;
    extern TypeObject ValueErrorObjectType;

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

    struct ValueErrorObject : ExceptionObject
    {
    };

    extern NotImplementedErrorObject P_NotImplemented;
}

#endif