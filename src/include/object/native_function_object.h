#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"

namespace vm
{
    extern ObjectType nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(std::span<Object*>);

    struct NativeFunctionObject : Object
    {
        WORD arity;
        std::string name;
        nativeFunction function;

        static NativeFunctionObject* create(int arity, std::string name, nativeFunction function);
    };
}

#endif
