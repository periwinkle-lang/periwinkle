#ifndef ARRAY_OBJECT_H
#define ARRAY_OBJECT_H

#include <vector>

#include "object.h"

namespace vm
{
    extern TypeObject arrayObjectType;

    struct ArrayObject : Object
    {
        std::vector<Object*> items;
        static ArrayObject* create();
    };
}

#endif
