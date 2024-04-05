#ifndef VALIDATE_ARGS_H
#define VALIDATE_ARGS_H

#include <span>

#include "vm.hpp"
#include "object.hpp"

namespace vm
{
    // Перевіряє чи передана правильна кількість аргументів для виклику функції
    void validateCall(
        WORD arity, const std::vector<std::string>* defaultNames,
        bool isVariadic, std::string fnName, bool isMethod,
        WORD argc, vm::NamedArgs* namedArgs,
        std::vector<size_t>* namedArgIndexes);
}

#endif
