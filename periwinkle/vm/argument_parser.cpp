#include <format>

#include "argument_parser.hpp"
#include "vm.hpp"
#include "exception_object.hpp"
#include "utils.hpp"
#include "periwinkle.hpp"

using namespace vm;

#define SET_ARG(desc, value) \
    *((Object**)desc.pointer) = value;

// Перевіряє чи тип параметра співпадає з типом переданого значення
#define CHECK_SET_ARG(desc, value)                                                                  \
    if (isInstance(value, desc.type))                                                               \
    {                                                                                               \
        SET_ARG(desc, value);                                                                       \
    }                                                                                               \
    else                                                                                            \
    {                                                                                               \
        getCurrentState()->setException(                                                            \
            &TypeErrorObjectType,                                                                   \
            std::format(                                                                            \
                "Тип аргументу \"{}\" має бути \"{}\", натомість був переданий об'єкт типу \"{}\"", \
                desc.name,                                                                          \
                desc.type.name,                                                                     \
                value->objectType->name)                                                            \
        );                                                                                          \
        return false;                                                                               \
    }

bool vm::ArgParser::parse(const std::span<Object*> args, DefaultParameters* defaults, NamedArgs* na)
{
    auto argc = args.size();
    auto defaultCount = defaults != nullptr ? defaults->parameters.size() : 0;
    auto arity = description.size();

    for (size_t i = 0; i < argc; ++i)
    {
        CHECK_SET_ARG(description[i], args[i]);
    }

    if (defaultCount)
    {
        if (na != nullptr)
        {
            for (size_t i = 0, j = na->count; i < arity - argc; ++i)
            {
                if (j)
                {
                    auto it = std::find(na->indexes.begin(), na->indexes.end(), i);
                    if (it != na->indexes.end())
                    {
                        auto index = it - na->indexes.begin();
                        CHECK_SET_ARG(description[i + argc], na->values[index]);
                        j--;
                        continue;
                    }
                }

                SET_ARG(description[i + argc], defaults->parameters[i].second);
            }
        }
        else
        {
            for (size_t i = 0; i < arity - argc; ++i)
            {
                SET_ARG(description[i + argc], defaults->parameters[i].second);
            }
        }
    }
    return true;
}
