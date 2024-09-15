#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

namespace vm
{
    extern TypeObject nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(std::span<Object*>, TupleObject*, NamedArgs*);

    struct NativeFunctionObject : Object
    {
        CallableInfo callableInfo;
        nativeFunction function;

        NativeFunctionObject() = default;

        NativeFunctionObject(
            const std::string_view name, vm::nativeFunction function, WORD arity,
            bool variadic=false, DefaultParameters* defaults=nullptr) noexcept;

        static NativeFunctionObject* create(
            int arity, bool isVariadic, std::string name,
            nativeFunction function, DefaultParameters* defaults=nullptr);
    };
}

#endif
