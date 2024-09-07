#ifndef FUNCTION_OBJECT_H
#define FUNCTION_OBJECT_H

#include <vector>

#include "object.hpp"
#include "code_object.hpp"
#include "cell_object.hpp"

namespace vm
{
    extern TypeObject functionObjectType;

    struct FunctionObject : Object
    {
        CallableInfo callableInfo;
        CodeObject* code;
        std::vector<CellObject*> closure; // Масив комірок

        static FunctionObject* create(CodeObject* code);
    };
}

#endif
