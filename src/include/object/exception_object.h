#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType ExceptionObjectType;
    extern ObjectType NameErrorObjectType;
    extern ObjectType TypeErrorObjectType;

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
}

#endif
