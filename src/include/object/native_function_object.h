#ifndef NATIVE_FUNCTION_OBJECT_H
#define NATIVE_FUNCTION_OBJECT_H

#include "object.h"

namespace vm
{
    extern ObjectType nativeFunctionObjectType;
    // Функція приймає масив об'єктів та повертає посилання на результат
    using nativeFunction = Object*(*)(Object*[]);

    struct NativeFunctionObject : Object
    {
        int arity;
        std::string name;
        nativeFunction function;

        static NativeFunctionObject* create(int arity, std::string name, nativeFunction function);
    };
}

#endif
