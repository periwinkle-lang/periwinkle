#ifndef STRING_OBJECT_H
#define STRING_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType stringObjectType;

    struct StringObject : Object
    {
        std::string value;

        static StringObject* create(std::string value);
    };
}

#endif
