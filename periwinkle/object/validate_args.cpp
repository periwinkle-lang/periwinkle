#include <algorithm>
#include <format>

#include "validate_args.hpp"
#include "vm.hpp"
#include "utils.hpp"
#include "periwinkle.hpp"

#define CALLABLE_NAME (isMethod ? "Метод" : "Функція")

using namespace vm;

bool vm::validateCall(
    WORD arity, const std::vector<std::string>* defaultNames,
    bool isVariadic, std::string fnName, bool isMethod,
    WORD argc, vm::NamedArgs* namedArgs,
    std::vector<size_t>* namedArgIndexes)
{
    auto defaultCount = defaultNames != nullptr ? defaultNames->size() : 0;
    if (namedArgs && defaultCount == 0 && namedArgs->count != 0)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format("{} \"{}\" не має іменованих параметрів", CALLABLE_NAME, fnName)
        );
        return false;
    }

    auto arityWithoutDefaults = arity - defaultCount;
    if (argc < arityWithoutDefaults)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format(
                "{} \"{}\" очікує {} {}, натомість передано {}",
                CALLABLE_NAME, fnName, arityWithoutDefaults,
                utils::wordDeclension(arityWithoutDefaults, "аргумент"), argc)
        );
        return false;
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
            getCurrentState()->setException(&TypeErrorObjectType,
                std::format(
                    "{} \"{}\" очікує {} {}, натомість передано {}",
                    CALLABLE_NAME, fnName, arityWithoutDefaults,
                    utils::wordDeclension(arityWithoutDefaults, "аргумент"), argc)
            );
            return false;
        }
    }

    if (defaultCount)
    {
        if (argc > arity)
        {
            getCurrentState()->setException(&TypeErrorObjectType,
                std::format(
                    "{} \"{}\" очікує від {} до {} аргументів, натомість передано {}",
                    CALLABLE_NAME, fnName, arityWithoutDefaults, arity, argc)
            );
            return false;
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
                    getCurrentState()->setException(&TypeErrorObjectType,
                        std::format(
                            "{} \"{}\" не має параметра за замовчуванням з іменем \"{}\"",
                            CALLABLE_NAME, fnName, argName)
                    );
                    return false;
                }
            }

            if (argc > arityWithoutDefaults)
            {
                auto overflow = argc - arityWithoutDefaults;
                auto max_it = std::max_element(namedArgIndexes->begin(), namedArgIndexes->end(),
                    [overflow](size_t a, size_t b) { return a < overflow && a > b; });

                if (max_it != namedArgIndexes->end())
                {
                    getCurrentState()->setException(&TypeErrorObjectType,
                        std::format(
                            "{} \"{}\", аргумент \"{}\" приймає два значення",
                            CALLABLE_NAME, fnName,
                            namedArgs->names->at(max_it - namedArgIndexes->begin())
                        )
                    );
                    return false;
                }
            }
        }
    }
    return true;
}
