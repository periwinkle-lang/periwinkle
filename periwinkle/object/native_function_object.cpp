#include "native_function_object.hpp"
#include "utils.hpp"
#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "vm.hpp"

using namespace vm;

static Object* nativeCall(
    NativeFunctionObject* nativeFunction, std::span<Object*> argv, ListObject* va, NamedArgs* na)
{
    return nativeFunction->function(argv, va, na);
}

static void traverse(NativeFunctionObject* func)
{
    if (func->callableInfo.defaults)
    {
        for (const auto& o : func->callableInfo.defaults->parameters)
        {
            mark(o.second);
        }
    }
}

namespace vm
{
    TypeObject nativeFunctionObjectType =
    {
        .base = &objectObjectType,
        .name = "НативнаФункція",
        .size = sizeof(NativeFunctionObject),
        .callableInfoOffset = offsetof(NativeFunctionObject, callableInfo),
        .alloc = DEFAULT_ALLOC(NativeFunctionObject),
        .dealloc = DEFAULT_DEALLOC(NativeFunctionObject),
        .operators =
        {
            .call = (callFunction)nativeCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

vm::NativeFunctionObject::NativeFunctionObject(
    const std::string_view name, vm::nativeFunction functions, WORD arity,
    bool variadic, DefaultParameters* defaults) noexcept
{
    this->objectType = &vm::nativeFunctionObjectType;
    this->callableInfo.arity = arity + (defaults ? defaults->parameters.size() : 0);
    this->callableInfo.flags |= variadic ? CallableInfo::IS_VARIADIC : 0;
    this->callableInfo.flags |= defaults ? CallableInfo::HAS_DEFAULTS : 0;
    this->callableInfo.name = name;
    this->callableInfo.defaults = defaults;
    this->function = function;
}

NativeFunctionObject* vm::NativeFunctionObject::create(
    int arity, bool isVariadic, std::string name,
    nativeFunction function, DefaultParameters* defaults)
{
    auto nativeFunction = (NativeFunctionObject*)allocObject(&nativeFunctionObjectType);
    nativeFunction->callableInfo.arity = arity + (defaults ? defaults->parameters.size() : 0);
    nativeFunction->callableInfo.flags |= isVariadic ? CallableInfo::IS_VARIADIC : 0;
    nativeFunction->callableInfo.flags |= defaults ? CallableInfo::HAS_DEFAULTS : 0;
    nativeFunction->callableInfo.name = name;
    nativeFunction->callableInfo.defaults = defaults;
    nativeFunction->function = function;
    return nativeFunction;
}
