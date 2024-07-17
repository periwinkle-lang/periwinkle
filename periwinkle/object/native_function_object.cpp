#include "native_function_object.hpp"
#include "utils.hpp"
#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "vm.hpp"
#include "validate_args.hpp"

using namespace vm;

Object* nativeCall(NativeFunctionObject* nativeFunction, Object**& sp, WORD argc, NamedArgs* namedArgs)
{
    auto arityWithoutDefaults = nativeFunction->arity;
    std::vector<std::string>* defaultNames = nullptr;
    if (nativeFunction->defaults)
    {
        defaultNames = &nativeFunction->defaults->names;
        arityWithoutDefaults -= defaultNames->size();
    }
    std::vector<size_t> namedArgIndexes;

    if (!validateCall(
        nativeFunction->arity, defaultNames, nativeFunction->isVariadic,
        nativeFunction->name, false, argc, namedArgs, &namedArgIndexes
    )) return nullptr;

    auto variadicParameter = new ListObject{ {&listObjectType} };
    if (nativeFunction->isVariadic)
    {
        auto variadicCount = argc - arityWithoutDefaults;
        for (WORD i = variadicCount; i > 0; --i)
        {
            variadicParameter->items.push_back(*(sp - i + 1));
        }
        sp -= variadicCount;
        argc -= variadicCount; // Змінюється значення вхідного аргументу
    }
    auto result = nativeFunction->function(
        {sp - (argc != 0 ? argc - 1 : 0), argc},
        variadicParameter, namedArgs
    );
    delete variadicParameter;
    sp -= argc + 1;
    return result;
}

static void traverse(NativeFunctionObject* func)
{
    if (func->defaults)
    {
        for (auto o : func->defaults->values)
        {
            mark(o);
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
        .alloc = DEFAULT_ALLOC(NativeFunctionObject),
        .dealloc = DEFAULT_DEALLOC(NativeFunctionObject),
        .operators =
        {
            .call = (callFunction)nativeCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

NativeFunctionObject* vm::NativeFunctionObject::create(
    int arity, bool isVariadic, std::string name,
    nativeFunction function, DefaultParameters* defaults)
{
    auto nativeFunction = (NativeFunctionObject*)allocObject(&nativeFunctionObjectType);
    nativeFunction->arity = arity + (defaults ? defaults->names.size() : 0);
    nativeFunction->isVariadic = isVariadic;
    nativeFunction->name = name;
    nativeFunction->function = function;
    nativeFunction->defaults = defaults;
    return nativeFunction;
}
