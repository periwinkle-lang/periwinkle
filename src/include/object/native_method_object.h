#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"

#define OBJECT_METHOD(name, arity, method) \
    {name,                                 \
     new vm::NativeMethodObject{&vm::nativeMethodObjectType, arity + 1, name, method}}

namespace vm
{
    extern ObjectType nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>);

    struct NativeMethodObject : Object
    {
        WORD arity;
        std::string name;
        nativeMethod method;

        static NativeMethodObject* create(
            WORD arity, std::string name, nativeMethod method);
    };
}

#endif
