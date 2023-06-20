#include <algorithm>

#include "validate_args.h"
#include "vm.h"
#include "utils.h"

#define CALLABLE_NAME (isMethod ? "Метод" : "Функція")

using namespace vm;

void vm::validateCall(
    WORD arity, const std::vector<std::string>* defaultNames,
    bool isVariadic, std::string fnName, bool isMethod,
    WORD argc, vm::NamedArgs* namedArgs,
    std::vector<size_t>* namedArgIndexes)
{
    auto defaultCount = defaultNames != nullptr ? defaultNames->size() : 0;
    auto arityWithoutDefaults = arity - defaultCount;

    if (argc < arityWithoutDefaults)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format(
                "%s \"%s\" очікує %u аргументів, натомість передано %u",
                CALLABLE_NAME, fnName.c_str(), arityWithoutDefaults, argc)
        );
    }

    if (namedArgs && defaultCount == 0 && namedArgs->count != 0)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format("%s \"%s\" не має параметрів за замовчуванням", CALLABLE_NAME, fnName.c_str())
        );
    }

    if (isVariadic)
    {
        if (auto variadicCount = argc - arityWithoutDefaults; variadicCount > 0)
        {
            // Тепер змінна argc позначає кількість переданих змінних
            // без урахування варіативних аргументів
            argc -= variadicCount;
        }
    }
    else
    {
        if (argc > arity && defaultCount == 0)
        {
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                utils::format(
                    "%s \"%s\" очікує %u аргументів, натомість передано %u",
                    CALLABLE_NAME, fnName.c_str(), arityWithoutDefaults, argc)
            );
        }
    }

    if (defaultCount)
    {
        if (argc > arity)
        {
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                utils::format(
                    "%s \"%s\" очікує від %u до %u аргументів, натомість передано %u",
                    CALLABLE_NAME, fnName.c_str(), arityWithoutDefaults, arity, argc)
            );
        }

        if (namedArgs != nullptr)
        {
            for (size_t i = 0; i < namedArgs->count; ++i)
            {
                auto& argName = namedArgs->names->at(i);
                auto it = std::find(defaultNames->begin(), defaultNames->end(), argName);
                if (it != defaultNames->end())
                {
                    namedArgIndexes->push_back(it - defaultNames->begin());
                }
                else
                {
                    VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                        utils::format(
                            "%s \"%s\" не має параметра за замовчуванням з іменем \"%s\"",
                            CALLABLE_NAME, fnName.c_str(), argName.c_str())
                    );
                }
            }

            if (argc > arityWithoutDefaults)
            {
                auto overflow = argc - arityWithoutDefaults;
                auto max_it = std::max_element(namedArgIndexes->begin(), namedArgIndexes->end(),
                    [overflow](size_t a, size_t b) { return a < overflow && a > b; });

                if (max_it != namedArgIndexes->end())
                {
                    VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                        utils::format(
                            "%s \"%s\", аргумент \"%s\" приймає два значення",
                            CALLABLE_NAME, fnName.c_str(),
                            namedArgs->names->at(max_it - namedArgIndexes->begin()).c_str()
                        )
                    );
                }
            }
        }
    }
}
