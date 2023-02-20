#include "native_function_object.h"
#include "vm.h"
#include "utils.h"

using namespace vm;
extern ObjectType objectObjectType;

Object* nativeCall(Object* callable, Object** sp, WORD argc)
{
   auto nativeFunction = (NativeFunctionObject*)callable;
   if (nativeFunction->arity != argc)
   {
       VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
           utils::format("Функція \"%s\" очікує %u аргументів, натомість було передано %u",
               nativeFunction->name.c_str(), nativeFunction->arity, argc));
   }
   auto result = nativeFunction->function({sp - argc + 1, argc});
   return result;
}

Object* allocNativeFunction();

namespace vm
{
    ObjectType nativeFunctionObjectType =
    {
        .base = &objectObjectType,
        .name = "NativeFunction",
        .type = ObjectTypes::NATIVE_FUNCTION,
        .alloc = &allocNativeFunction,
        .operators =
        {
            .call = nativeCall,
        },
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
