#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"
#include "array_object.h"

#define NATIVE_FUNCTION(name, arity, variadic, func)               \
    vm::NativeFunctionObject{&vm::nativeFunctionObjectType, arity, \
                           variadic, name, func}

#define OBJECT_STATIC_METHOD(name, arity, variadic, func) \
    {name, new NATIVE_FUNCTION(name, arity, variadic, (nativeFunction)func)}

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
