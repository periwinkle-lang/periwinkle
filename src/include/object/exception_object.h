#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType exceptionObjectType;
    extern ObjectType nameExceptionObjectType;
    extern ObjectType typeErrorObjectType;

    struct ExceptionObject : Object
    {
        std::string message; // Повідомлення винятку
    };

    struct NameErrorObject : ExceptionObject
    {
        std::string name; // Ім'я, через яке був викликаний виняток

        static NameErrorObject* create(std::string message, std::string name);
    };

    struct TypeErrorObject : ExceptionObject
    {
        std::string name; // Назва типу, через який був викликаний виняток

        static TypeErrorObject* create(std::string message, std::string name);
    };
}

#endif
