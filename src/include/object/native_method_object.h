#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"
#include "array_object.h"

#define NATIVE_METHOD(name, arity, variadic, method, classType, defaults)  \
    vm::NativeMethodObject{&vm::nativeMethodObjectType, arity + 1,         \
                           variadic, name, method, &classType, defaults}

#define OBJECT_METHOD(name, arity, variadic, method, classType, defaults) \
    {name, new NATIVE_METHOD(name, arity, variadic, (nativeMethod)method, classType, defaults)}

namespace vm
{
    extern TypeObject nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, ArrayObject*, NamedArgs*);

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
