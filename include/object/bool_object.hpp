#ifndef BOOL_OBJECT_H
#define BOOL_OBJECT_H

#include "object.hpp"

// Конвертує логічний тип С++ в логічний тип Барвінку
#define P_BOOL(boolValue) (boolValue) ? &vm::P_true : &vm::P_false

namespace vm
{
    extern TypeObject boolObjectType;

    struct BoolObject : Object
    {
        bool value;
    };

    extern BoolObject P_true;
    extern BoolObject P_false;
}

#endif
