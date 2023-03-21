#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"
#include "array_object.h"

namespace vm
{
    extern TypeObject nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(std::span<Object*>, ArrayObject*);

    struct NativeFunctionObject : Object
    {
        WORD arity;
        bool isVariadic;
        std::string name;
        nativeFunction function;

        static NativeFunctionObject* create(
            int arity, bool isVariadic, std::string name, nativeFunction function);
    };
}

#endif
