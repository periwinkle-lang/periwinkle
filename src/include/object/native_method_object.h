#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.h"
#include "vm.h"
#include "array_object.h"

#define NATIVE_METHOD(name, arity, variadic, method, classType)    \
    vm::NativeMethodObject{&vm::nativeMethodObjectType, arity + 1, \
                           variadic, name, method, &classType}

#define OBJECT_METHOD(name, arity, variadic, method, classType) \
    {name, new NATIVE_METHOD(name, arity, variadic, method, classType)}

namespace vm
{
    extern TypeObject nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, ArrayObject*);

    struct NativeMethodObject : Object
    {
        WORD arity;
        bool isVariadic;
        std::string name;
        nativeMethod method;
        TypeObject* classType;

        static NativeMethodObject* create(
            WORD arity, bool isVariadic, std::string name,
            nativeMethod method, TypeObject* classType);
    };

    Object* callNativeMethod(
        Object* instance, NativeMethodObject* method, std::span<Object*> args);
}

#endif
