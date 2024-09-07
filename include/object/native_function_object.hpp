#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

#define NATIVE_FUNCTION(name, arity, variadic, func, defaults)                   \
    vm::NativeFunctionObject{&vm::nativeFunctionObjectType, false,               \
                           arity, name, defaults,                                \
                           [](bool isVariadic, DefaultParameters* d) constexpr { \
                               return                                            \
                               (isVariadic ? vm::CallableInfo::IS_VARIADIC : 0)  \
                               | (d ? vm::CallableInfo::HAS_DEFAULTS : 0);       \
                           }(variadic, defaults),                                \
                           func}

#define OBJECT_STATIC_METHOD(name, arity, variadic, func, defaults) \
    {name, new NATIVE_FUNCTION(name, arity, variadic, (nativeFunction)func, defaults)}

namespace vm
{
    extern TypeObject nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(std::span<Object*>, ListObject*, NamedArgs*);

    struct NativeFunctionObject : Object
    {
        CallableInfo callableInfo;
        nativeFunction function;

        static NativeFunctionObject* create(
            int arity, bool isVariadic, std::string name,
            nativeFunction function, DefaultParameters* defaults=nullptr);
    };
}

#endif
