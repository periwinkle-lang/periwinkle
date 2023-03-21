#ifndef FUNCTION_OBJECT_H
#define FUNCTION_OBJECT_H

#include <vector>

#include "object.h"
#include "code_object.h"
#include "cell_object.h"

namespace vm
{
    extern TypeObject functionObjectType;

    struct FunctionObject : Object
    {
        CodeObject* code;
        std::vector<CellObject*> closure; // Масив комірок

        static FunctionObject* create(CodeObject* code);
    };
}

#endif
