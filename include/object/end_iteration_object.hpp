#ifndef END_ITERATION_OBJECT_H
#define END_ITERATION_OBJECT_H

#include "object.hpp"

namespace vm
{
    struct EndIterObject : Object
    {
    };

    extern TypeObject endIterObjectType;
    extern EndIterObject P_endIter;
}

#endif
