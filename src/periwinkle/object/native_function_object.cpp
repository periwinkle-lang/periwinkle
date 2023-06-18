#include "native_function_object.h"
#include "utils.h"
#include "native_method_object.h"
#include "string_vector_object.h"
#include "vm.h"
#include "validate_args.h"

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

    auto variadicParameter = ArrayObject::create();
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
        .type = ObjectTypes::NATIVE_FUNCTION,
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
