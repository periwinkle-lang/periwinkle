#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "utils.hpp"
#include "periwinkle.hpp"

using namespace vm;

static Object* nativeMethodCall(
    NativeMethodObject* callable, std::span<Object*> argv, TupleObject* va, NamedArgs* na)
{
    auto instance = argv[0];
    if (instance->objectType != callable->classType)
    {
        getCurrentState()->setException(
            &TypeErrorObjectType,
            std::format("Метод \"{}\" виконує операції тільки над об'єктами типу \"{}\"",
                callable->callableInfo.name, callable->classType->name)
            );
        return nullptr;
    }
    return callable->method(instance, argv.subspan(1), va, na);
}

static void traverse(NativeMethodObject* method)
{
    if (method->classType != nullptr)
    {
        mark(method->classType);
    }
    if (method->callableInfo.defaults)
    {
        for (const auto& o : method->callableInfo.defaults->parameters)
        {
            mark(o.second);
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
        .callableInfoOffset = offsetof(NativeMethodObject, callableInfo),
        .alloc = DEFAULT_ALLOC(NativeMethodObject),
        .dealloc = DEFAULT_DEALLOC(NativeMethodObject),
        .operators =
        {
            .call = (callFunction)nativeMethodCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

vm::NativeMethodObject::NativeMethodObject(
    const std::string_view name, vm::nativeMethod method, TypeObject* classType, WORD arity,
    bool variadic, DefaultParameters* defaults) noexcept
{
    this->objectType = &vm::nativeMethodObjectType;
    this->callableInfo.arity = arity + 1 + (defaults ? defaults->parameters.size() : 0);
    this->callableInfo.flags |= variadic ? CallableInfo::IS_VARIADIC : 0;
    this->callableInfo.flags |= defaults ? CallableInfo::HAS_DEFAULTS : 0;
    this->callableInfo.name = name;
    this->method = method;
    this->classType = classType;
    this->callableInfo.defaults = defaults;
}

NativeMethodObject* vm::NativeMethodObject::create(
    WORD arity, bool isVariadic, std::string name,
    nativeMethod method, TypeObject* classType, DefaultParameters* defaults)
{
    auto nativeMethod =
        (NativeMethodObject*)allocObject(&nativeMethodObjectType);
    nativeMethod->callableInfo.arity = arity + 1 + (defaults ? defaults->parameters.size() : 0); // +1 для екземляра класу
    nativeMethod->callableInfo.flags |= isVariadic ? CallableInfo::IS_VARIADIC : 0;
    nativeMethod->callableInfo.flags |= defaults ? CallableInfo::HAS_DEFAULTS : 0;
    nativeMethod->callableInfo.name = name;
    nativeMethod->method = method;
    nativeMethod->classType = classType;
    nativeMethod->callableInfo.defaults = defaults;
    return nativeMethod;
}
