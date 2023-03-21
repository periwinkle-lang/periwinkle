#ifndef NULL_OBJECT_H
#define NULL_OBJECT_H

#include "object.h"

namespace vm
{
    struct NullObject : Object
    {
    };

    extern TypeObject nullObjectType;
    extern NullObject P_null;
}

#endif
