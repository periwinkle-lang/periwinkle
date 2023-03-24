#ifndef STRING_OBJECT_H
#define STRING_OBJECT_H

#include "object.h"

namespace vm
{
    extern TypeObject stringObjectType, stringIterObjectType;

    struct StringObject : Object
    {
        std::string value;

        static StringObject* create(std::string value);
    };

    struct StringIterObject : Object
    {
        size_t position;
        size_t length;
        std::string iterable;

        static StringIterObject* create(const std::string& iterable);
    };
}

#endif
