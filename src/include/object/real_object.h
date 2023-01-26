#ifndef REAL_OBJECT_H
#define REAL_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType realObjectType;

    struct RealObject : Object
    {
        double value;
        
        static RealObject* create(double value);
    };
}

#endif
