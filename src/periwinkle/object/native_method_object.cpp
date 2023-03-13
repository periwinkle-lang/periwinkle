#include "native_method_object.h"
#include "utils.h"

using namespace vm;
extern ObjectType objectObjectType;

Object* nativeMethodCall(Object* callable, Object** sp, WORD argc)
{
    auto nativeMethod = (NativeMethodObject*)callable;
    auto firstArg = sp - argc + 1;
    auto instance = *(firstArg - 2); // Екземпляр об'єкта, в якого викликається метод.

    // Якщо перед методом на стеці немає екземпляра,
    // то тоді він повинен передаватися через аргументи.
    WORD actualArgc = instance == nullptr ? argc : argc + 1;

    if (nativeMethod->arity != actualArgc)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format(
                "Функція \"%s\" очікує %u аргументів, натомість було передано %u",
                nativeMethod->name.c_str(), nativeMethod->arity, argc));
    }

    if (instance == nullptr)
    {
        instance = *firstArg; // Береться екземпляр об'єта з аргументів.
        firstArg++; // Пропускається екземпляр.
    }

    auto result = nativeMethod->method(instance, {firstArg, nativeMethod->arity - 1});
    return result;
}

Object* allocNativeMethodObject();

namespace vm
{
    ObjectType nativeMethodObjectType =
    {
        .base = &objectObjectType,
        .name = "Нативний метод",
        .type = ObjectTypes::NATIVE_METHOD,
        .alloc = &allocNativeMethodObject,
        .operators =
        {
            .call = nativeMethodCall,
        },
    };
}

Object* allocNativeMethodObject()
{
    auto nativeMethodObject = new NativeMethodObject;
    nativeMethodObject->objectType = &nativeMethodObjectType;
    return (Object*)nativeMethodObject;
}

NativeMethodObject* vm::NativeMethodObject::create(
    WORD arity, std::string name, nativeMethod method)
{
    auto nativeMethodFunction =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethodFunction->arity = arity + 1; // + 1 для екземляра класу
    nativeMethodFunction->name = name;
    nativeMethodFunction->method = method;
    return nativeMethodFunction;
}
