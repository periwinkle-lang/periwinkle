#ifndef FUNCTION_OBJECT_H
#define FUNCTION_OBJECT_H

#include <vector>
#include "object.h"
#include "code_object.h"

namespace vm
{
    extern ObjectType functionObjectType;

    struct FunctionObject : Object
    {
        CodeObject* code;
        std::vector<Object*> cells;
    };
}

#endif
