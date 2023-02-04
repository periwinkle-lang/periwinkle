#ifndef BUILTINS_H
#define BUILTINS_H

#include <iostream>
#include <unordered_map>

#include "native_function_object.h"

namespace vm
{
    using builtin_t = std::unordered_map<std::string, NativeFunctionObject*>;
    builtin_t* getBuiltin();
}

#endif
