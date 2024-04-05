#ifndef CELL_OBJECT_H
#define CELL_OBJECT_H

#include "object.hpp"

namespace vm
{
    extern TypeObject cellObjectType;

    // Допоміжний об'єкт для зберігання вільних змінних в замиканнях
    struct CellObject : Object
    {
        Object* value;

        static CellObject* create(Object* value);
    };
}

#endif
