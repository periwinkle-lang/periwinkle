#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "utils.hpp"
#include "validate_args.hpp"
#include "periwinkle.hpp"

using namespace vm;

static Object* nativeMethodCall(NativeMethodObject* callable, Object**& sp, WORD argc,
    NamedArgs* namedArgs)
{
    auto firstArg = sp - argc + 1;

    if ((*firstArg)->objectType != callable->classType)
    {
        getCurrentState()->setException(
            &TypeErrorObjectType,
            "Першим аргументом має бути переданий екземпляр класу,"
            " до якого відноситься метод");
        return nullptr;
    }

    std::span<Object*> args{ firstArg + 1, argc - 1 };
    auto result = callNativeMethod(*firstArg, callable, args, namedArgs);
    if (!result) return nullptr;
    sp -= argc + 1; // +1 для методу
    return result;
}

static void traverse(NativeMethodObject* method)
{
    if (method->classType != nullptr)
    {
        mark(method->classType);
    }
    if (method->defaults)
    {
        for (auto o : method->defaults->values)
        {
            mark(o);
        }
    }
}

namespace vm
{
    TypeObject nativeMethodObjectType =
    {
        .base = &objectObjectType,
        .name = "НативнийМетод",
        .size = sizeof(NativeMethodObject),
        .alloc = DEFAULT_ALLOC(NativeMethodObject),
        .operators =
        {
            .call = (callFunction)nativeMethodCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

NativeMethodObject* vm::NativeMethodObject::create(
    WORD arity, bool isVariadic, std::string name,
    nativeMethod method, TypeObject* classType, DefaultParameters* defaults)
{
    auto nativeMethod =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethod->arity = arity + 1 + (defaults ? defaults->names.size() : 0); // +1 для екземляра класу
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
    auto arityWithoutDefaults = method->arity;
    auto argc = args.size() + 1; // +1 для екземпляра

    std::vector<std::string>* defaultNames = nullptr;
    if (method->defaults)
    {
        defaultNames = &method->defaults->names;
        arityWithoutDefaults -= defaultNames->size();
    }
    std::vector<size_t> namedArgIndexes;

    if (!validateCall(
        method->arity, defaultNames, method->isVariadic,
        method->name, true, argc, namedArgs, &namedArgIndexes
    )) return nullptr;

    auto variadicParameter = new ListObject{ {&listObjectType} };
    if (method->isVariadic)
    {
        auto variadicCount = argc - arityWithoutDefaults;
        for (WORD i = 0; i < variadicCount; ++i)
        {
            variadicParameter->items.push_back(args[argc - variadicCount - 1 + i]);
        }
    }

    auto result = method->method(
        instance, args, variadicParameter, namedArgs);
    delete variadicParameter;
    return result;
}
