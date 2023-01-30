#include "native_function_object.h"

using namespace vm;
extern ObjectType objectObjectType;

Object* allocNativeFunction();

namespace vm
{
    ObjectType nativeFunctionObjectType =
    {
        .base = &objectObjectType,
        .name = "NativeFunction",
        .type = ObjectTypes::NATIVE_FUNCTION,
        .alloc = &allocNativeFunction,
    };
}

Object* allocNativeFunction()
{
    auto nativeFunctionObject = new NativeFunctionObject;
    nativeFunctionObject->objectType = &nativeFunctionObjectType;
    return (Object*)nativeFunctionObject;
}

NativeFunctionObject* vm::NativeFunctionObject::create(int arity, std::string name, nativeFunction function)
{
    auto nativeFunction = (NativeFunctionObject*)allocObject(&nativeFunctionObjectType);
    nativeFunction->arity = arity;
    nativeFunction->name = name;
    nativeFunction->function = function;
    return nativeFunction;
}
