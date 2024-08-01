#ifndef INT_OBJECT_H
#define INT_OBJECT_H

#include "object.hpp"
#include "types.hpp"

namespace vm
{
    extern TypeObject intObjectType;

    struct IntObject : Object
    {
        i64 value;

        static IntObject* create(i64 value);
    };
}

#endif
