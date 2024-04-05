#include "argument_parser.hpp"
#include "vm.hpp"
#include "exception_object.hpp"
#include "utils.hpp"

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
        VirtualMachine::currentVm->throwException(                                                  \
            &TypeErrorObjectType,                                                                   \
            utils::format(                                                                          \
                "Тип аргументу \"%s\" має бути \"%s\", натомість був переданий об'єкт типу \"%s\"", \
                desc.name.c_str(),                                                                  \
                desc.type.name.c_str(),                                                             \
                value->objectType->name.c_str())                                                    \
        );                                                                                          \
    }

void vm::ArgParser::parse(const std::span<Object*> args, DefaultParameters* defaults, NamedArgs* na)
{
    auto argc = args.size();
    auto defaultCount = defaults != nullptr ? defaults->names.size() : 0;
    auto arity = description.size();

    for (size_t i = 0; i < argc; ++i)
    {
        CHECK_SET_ARG(description[i], args[i]);
    }

    if (defaultCount)
    {
        if (na != nullptr)
        {
            std::vector<size_t> namedArgIndexes;

            for (size_t i = 0; i < na->count; ++i)
            {
                auto& argName = na->names->at(i);
                auto it = std::find(defaults->names.begin(), defaults->names.end(), argName);
                if (it != defaults->names.end())
                {
                    namedArgIndexes.push_back(it - defaults->names.begin());
                }
            }

            for (size_t i = arity - argc - 1, j = namedArgIndexes.size(); i < arity; ++i)
            {
                if (j)
                {
                    auto it = std::find(namedArgIndexes.begin(), namedArgIndexes.end(), i);
                    if (it != namedArgIndexes.end())
                    {
                        auto index = it - namedArgIndexes.begin();
                        CHECK_SET_ARG(description[i], na->values[index]);
                        j--;
                        continue;
                    }
                }

                SET_ARG(description[i], defaults->values[i - (arity - argc - 1)]);
            }
        }
        else
        {
            for (size_t i = arity - argc - 1; i < arity; ++i)
            {
                SET_ARG(description[i], defaults->values[i - (arity - argc - 1)]);
            }
        }
    }
}
