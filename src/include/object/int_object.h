#ifndef INT_OBJECT_H
#define INT_OBJECT_H

#include "object.h"
#include "types.h"

namespace vm
{
    extern ObjectType intObjectType;

    struct IntObject : Object
    {
        i64 value;

        static IntObject* create(i64 value);
    };
}

#endif
