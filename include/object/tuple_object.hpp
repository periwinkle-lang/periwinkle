#ifndef TUPLE_OBJECT_H
#define TUPLE_OBJECT_H

#include <vector>

#include "object.hpp"

namespace vm
{
    extern TypeObject tupleObjectType, tupleIterObjectType;

    struct TupleObject : Object
    {
        std::vector<Object*> items;

        static TupleObject* create();
    };

    struct TupleIterObject : Object
    {
        size_t position = 0;
        TupleObject* tuple;

        static TupleIterObject* create(TupleObject* tuple);
    };
}

#endif
