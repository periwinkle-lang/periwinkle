#ifndef NATIVE_METHOD_OBJECT_H
#define NATIVE_METHOD_OBJECT_H

#include <span>

#include "object.hpp"
#include "vm.hpp"
#include "list_object.hpp"

#define NATIVE_METHOD(name, arity, variadic, method, classType, defaults)         \
    vm::NativeMethodObject{&vm::nativeMethodObjectType, false,                    \
                           [](WORD a, DefaultParameters* d) constexpr {           \
                               return a + 1 +                                     \
                               (d ? static_cast<WORD>(d->parameters.size()) : 0); \
                           }(arity, defaults),                                    \
                           name, defaults,                                        \
                           [](bool isVariadic, DefaultParameters* d) constexpr {  \
                               return                                             \
                               (isVariadic ? vm::CallableInfo::IS_VARIADIC : 0)   \
                               | (d ? vm::CallableInfo::HAS_DEFAULTS : 0);        \
                           }(variadic, defaults),                                 \
                           method, &classType}

#define OBJECT_METHOD(name, arity, variadic, method, classType, defaults) \
    {name, new NATIVE_METHOD(name, arity, variadic, (nativeMethod)method, classType, defaults)}

namespace vm
{
    extern TypeObject nativeMethodObjectType;
    using nativeMethod = Object*(*)(Object*, std::span<Object*>, ListObject*, NamedArgs*);

    struct NativeMethodObject : Object
    {
        CallableInfo callableInfo;
        nativeMethod method;
        TypeObject* classType;

        static NativeMethodObject* create(
            WORD arity, bool isVariadic, std::string name,
            nativeMethod method, TypeObject* classType,
            DefaultParameters* defaults=nullptr);
    };
}

#endif
