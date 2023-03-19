#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"
#include "array_object.h"

#define OBJECT_METHOD(name, arity, variadic, method)                    \
    {name,                                                              \
     new vm::NativeMethodObject{&vm::nativeMethodObjectType, arity + 1, \
                                variadic, name, method}}

namespace vm
{
    extern ObjectType nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, ArrayObject*);

    struct NativeMethodObject : Object
    {
        WORD arity;
        bool isVariadic;
        std::string name;
        nativeMethod method;

        static NativeMethodObject* create(
            WORD arity, bool isVariadic, std::string name, nativeMethod method);
    };
}

#endif
