#ifndef BUILTINS_H
#define BUILTINS_H

#include <iostream>
#include <unordered_map>

#include "object.hpp"

namespace vm
{
    void initBuiltins();
    void deinitBuiltins();
    using builtin_t = std::unordered_map<std::string, Object*>;
    builtin_t* getBuiltin();
}

#endif
