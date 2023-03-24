#ifndef ARRAY_OBJECT_H
#define ARRAY_OBJECT_H

#include <vector>

#include "object.h"

namespace vm
{
    extern TypeObject arrayObjectType, arrayIterObjectType;

    struct ArrayObject : Object
    {
        std::vector<Object*> items;

        static ArrayObject* create();
    };

    struct ArrayIterObject : Object
    {
        size_t position;
        size_t length;
        std::vector<Object*> iterable;

        static ArrayIterObject* create(const std::vector<Object*> iterable);
    };
}

#endif
