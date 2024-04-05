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

    validateCall(
        nativeFunction->arity, defaultNames, nativeFunction->isVariadic,
        nativeFunction->name, false, argc, namedArgs, &namedArgIndexes
    );

    auto variadicParameter = ListObject::create();
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

namespace vm
{
    TypeObject nativeFunctionObjectType =
    {
        .base = &objectObjectType,
        .name = "НативнаФункція",
        .alloc = DEFAULT_ALLOC(NativeFunctionObject),
        .operators =
        {
            .call = (callFunction)nativeCall,
        },
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
