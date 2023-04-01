#include "native_method_object.h"
#include "utils.h"

using namespace vm;

static Object* nativeMethodCall(NativeMethodObject* callable, Object**& sp, WORD argc)
{
    auto firstArg = sp - argc + 1;

    if ((*firstArg)->objectType != callable->classType)
    {
        VirtualMachine::currentVm->throwException(
            &TypeErrorObjectType,
            "Першим аргументом має бути переданий екземпляр класу,"
            " до якого відноситься метод");
    }

    std::span<Object*> args{ firstArg + 1, argc - 1 };
    auto result = callNativeMethod(*firstArg, callable, args);
    sp -= argc + 1; // +1 для методу
    return result;
}

namespace vm
{
    TypeObject nativeMethodObjectType =
    {
        .base = &objectObjectType,
        .name = "НативнийМетод",
        .type = ObjectTypes::NATIVE_METHOD,
        .alloc = DEFAULT_ALLOC(NativeMethodObject),
        .operators =
        {
            .call = (callFunction)nativeMethodCall,
        },
    };
}

NativeMethodObject* vm::NativeMethodObject::create(
    WORD arity, bool isVariadic, std::string name,
    nativeMethod method, TypeObject* classType)
{
    auto nativeMethod =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethod->arity = arity + 1; // + 1 для екземляра класу
    nativeMethod->isVariadic = isVariadic;
    nativeMethod->name = name;
    nativeMethod->method = method;
    nativeMethod->classType = classType;
    return nativeMethod;
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
