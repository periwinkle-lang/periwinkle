#include "native_method_object.h"
#include "string_vector_object.h"
#include "utils.h"
#include "validate_args.h"

using namespace vm;

static Object* nativeMethodCall(NativeMethodObject* callable, Object**& sp, WORD argc,
    NamedArgs* namedArgs)
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
    auto result = callNativeMethod(*firstArg, callable, args, namedArgs);
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
    nativeMethod method, TypeObject* classType, DefaultParameters* defaults)
{
    auto nativeMethod =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethod->arity = arity + 1; // +1 для екземляра класу
    nativeMethod->isVariadic = isVariadic;
    nativeMethod->name = name;
    nativeMethod->method = method;
    nativeMethod->classType = classType;
    nativeMethod->defaults = defaults;
    return nativeMethod;
}

Object* vm::callNativeMethod(Object* instance, NativeMethodObject* method, std::span<Object*> args,
    NamedArgs* namedArgs)
{
    auto argc = args.size() + 1; // +1 для екземпляра

    std::vector<std::string>* defaultNames = nullptr;
    if (method->defaults)
    {
        defaultNames = &method->defaults->names;
    }
    std::vector<size_t> namedArgIndexes;

    validateCall(
        method->arity, defaultNames, method->isVariadic,
        method->name, true, argc, namedArgs, &namedArgIndexes
    );

    auto variadicParameter = ArrayObject::create();
    if (method->isVariadic)
    {
        auto variadicCount = argc + (namedArgs != nullptr ? namedArgs->count : 0) - method->arity;
        for (WORD i = 0; i < variadicCount; ++i)
        {
            variadicParameter->items.push_back(args[argc - variadicCount - 1 + i]);
        }
    }

    auto result = method->method(
        instance, { data(args), method->arity - 1 }, variadicParameter, namedArgs);
    delete variadicParameter;
    return result;
}
