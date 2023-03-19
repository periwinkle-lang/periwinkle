#include "native_function_object.h"
#include "vm.h"
#include "utils.h"
#include "native_method_object.h"

using namespace vm;
extern ObjectType objectObjectType;

Object* nativeCall(Object* callable, Object**& sp, WORD argc)
{
   auto nativeFunction = (NativeFunctionObject*)callable;
   if (nativeFunction->isVariadic)
   {
       if (nativeFunction->arity > argc)
       {
           VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
               utils::format("Функція \"%s\" очікує мінімум %u аргументів,"
                   "натомість було передано %u",
                   nativeFunction->name.c_str(), nativeFunction->arity, argc));
       }
   }
   else if (nativeFunction->arity != argc)
   {
       VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
           utils::format("Функція \"%s\" очікує мінімум %u аргументів,"
               "натомість було передано %u",
               nativeFunction->name.c_str(), nativeFunction->arity, argc));
   }

   auto variadicParameter = ArrayObject::create();
   if (auto variadicCount = argc - nativeFunction->arity; variadicCount > 0)
   {
       static auto arrayPush =
           ((NativeMethodObject*)arrayObjectType.attributes["додати"])->method;
       for (WORD i = variadicCount; i > 0; --i)
       {
           arrayPush(variadicParameter, { sp - i + 1, 1 }, nullptr);
       }
   }

   auto result = nativeFunction->function({sp - argc + 1,
       nativeFunction->arity}, variadicParameter);
   delete variadicParameter;
   sp -= argc + 1;
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

NativeFunctionObject* vm::NativeFunctionObject::create(
    int arity, bool isVariadic, std::string name, nativeFunction function)
{
    auto nativeFunction = (NativeFunctionObject*)allocObject(&nativeFunctionObjectType);
    nativeFunction->arity = arity;
    nativeFunction->isVariadic = isVariadic;
    nativeFunction->name = name;
    nativeFunction->function = function;
    return nativeFunction;
}
