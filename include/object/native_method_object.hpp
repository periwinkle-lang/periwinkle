#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

namespace vm
{
    extern TypeObject nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, TupleObject*, NamedArgs*);

    struct NativeMethodObject : Object
    {
        CallableInfo callableInfo;
        nativeMethod method;
        TypeObject* classType;

        NativeMethodObject() = default;

        NativeMethodObject(
            const std::string_view name, vm::nativeMethod method, TypeObject* classType, WORD arity,
            bool variadic=false, DefaultParameters* defaults=nullptr) noexcept;

        static NativeMethodObject* create(
            WORD arity, bool isVariadic, std::string name,
            nativeMethod method, TypeObject* classType,
            DefaultParameters* defaults=nullptr);
    };
}

#endif
