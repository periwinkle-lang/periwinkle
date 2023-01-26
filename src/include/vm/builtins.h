#ifndef BUILTINS_H
#define BUILTINS_H

#include <iostream>
#include <unordered_map>

#include "native_function_object.h"

namespace vm
{
    extern std::unordered_map<std::string, NativeFunctionObject*> builtin;
}

#endif
