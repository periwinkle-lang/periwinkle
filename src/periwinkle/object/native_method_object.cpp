#include "native_method_object.h"
#include "utils.h"

using namespace vm;

Object* nativeMethodCall(Object* callable, Object**& sp, WORD argc)
{
    auto firstArg = sp - argc + 1;
    auto instance = *(firstArg - 2);

    // Якщо перед методом на стеці немає екземпляра,
    // то тоді він повинен передаватися через аргументи.
    WORD actualArgc = instance == nullptr ? argc - 1 : argc;

    auto result = callNativeMethod(
        instance == nullptr ? *firstArg : instance,
        (NativeMethodObject*)callable, { sp - actualArgc + 1, actualArgc });
    sp -= argc + 2; // +2, екземпляр класу та метод
    return result;
}

Object* allocNativeMethodObject();

namespace vm
{
    TypeObject nativeMethodObjectType =
    {
        .base = &objectObjectType,
        .name = "НативнийМетод",
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
    WORD arity, bool isVariadic, std::string name, nativeMethod method)
{
    auto nativeMethodFunction =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethodFunction->arity = arity + 1; // + 1 для екземляра класу
    nativeMethodFunction->isVariadic = isVariadic;
    nativeMethodFunction->name = name;
    nativeMethodFunction->method = method;
    return nativeMethodFunction;
}

Object* vm::callNativeMethod(
    Object* instance, NativeMethodObject* method, std::span<Object*> args)
{
    auto argc = args.size() + 1; // +1 для екземпляра

    if (method->isVariadic)
    {
        if (method->arity > argc)
        {
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                utils::format(
                    "Функція \"%s\" очікує мінімум %u аргументів,"
                    "натомість було передано %u",
                    method->name.c_str(), method->arity, argc));
        }
    }
    else if (method->arity != argc)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format(
                "Функція \"%s\" очікує %u аргументів, натомість було передано %u",
                method->name.c_str(), method->arity, argc));
    }

    auto variadicParameter = ArrayObject::create();
    if (auto variadicCount = argc - method->arity; variadicCount > 0)
    {
        static auto arrayPush =
            ((NativeMethodObject*)arrayObjectType.attributes["додати"])->method;
        for (WORD i = 0; i < variadicCount; ++i)
        {
            arrayPush(variadicParameter, { data(args) + i, 1}, nullptr);
        }
    }

    auto result = method->method(
        instance, { data(args), method->arity - 1 }, variadicParameter);
    delete variadicParameter;
    return result;
}
