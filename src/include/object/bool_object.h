#ifndef BOOL_OBJECT_H
#define BOOL_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType boolObjectType;

    struct BoolObject : Object
    {
        bool value;

        static BoolObject* create(bool value);
    };
}

#endif
