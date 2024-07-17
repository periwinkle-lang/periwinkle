#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

#define NATIVE_METHOD(name, arity, variadic, method, classType, defaults) \
    vm::NativeMethodObject{&vm::nativeMethodObjectType, false,            \
                           [](size_t a, DefaultParameters* d) constexpr { \
                                return a + 1 + (d ? d->names.size() : 0); \
                           }(arity, defaults),                            \
                           variadic, name, method, &classType, defaults}

#define OBJECT_METHOD(name, arity, variadic, method, classType, defaults) \
    {name, new NATIVE_METHOD(name, arity, variadic, (nativeMethod)method, classType, defaults)}

namespace vm
{
    extern TypeObject nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, ListObject*, NamedArgs*);

    struct NativeMethodObject : Object
    {
        WORD arity;
        bool isVariadic;
        std::string name;
        nativeMethod method;
        TypeObject* classType;
        DefaultParameters* defaults;

        static NativeMethodObject* create(
            WORD arity, bool isVariadic, std::string name,
            nativeMethod method, TypeObject* classType, DefaultParameters* defaults=nullptr);
    };

    Object* callNativeMethod(
        Object* instance, NativeMethodObject* method, std::span<Object*> args,
        NamedArgs* namedArgs=nullptr);
}

#endif
