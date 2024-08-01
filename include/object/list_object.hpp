#ifndef LIST_OBJECT_H
#define LIST_OBJECT_H

#include <vector>

#include "object.hpp"

namespace vm
{
    extern TypeObject listObjectType, listIterObjectType;

    struct ListObject : Object
    {
        std::vector<Object*> items;

        static ListObject* create();
    };

    struct ListIterObject : Object
    {
        size_t position;
        size_t length;
        std::vector<Object*> iterable;

        static ListIterObject* create(const std::vector<Object*> iterable);
    };
}

#endif
