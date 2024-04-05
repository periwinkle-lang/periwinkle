#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

#define NATIVE_FUNCTION(name, arity, variadic, func, defaults)     \
    vm::NativeFunctionObject{&vm::nativeFunctionObjectType, arity, \
                           variadic, name, func, defaults}

#define OBJECT_STATIC_METHOD(name, arity, variadic, func, defaults) \
    {name, new NATIVE_FUNCTION(name, arity, variadic, (nativeFunction)func, defaults)}

namespace vm
{
    extern TypeObject nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(std::span<Object*>, ListObject*, NamedArgs*);

    struct NativeFunctionObject : Object
    {
        WORD arity;
        bool isVariadic;
        std::string name;
        nativeFunction function;
        DefaultParameters* defaults;

        static NativeFunctionObject* create(
            int arity, bool isVariadic, std::string name,
            nativeFunction function, DefaultParameters* defaults=nullptr);
    };
}

#endif
