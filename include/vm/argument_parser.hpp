#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <vector>
#include <span>
#include <initializer_list>

#include "object.hpp"

namespace vm
{
    struct ArgParser
    {
        struct Arg
        {
            void* pointer; // Куди буде збережений результат
            TypeObject& type;
            std::string name;
        };

        std::vector<Arg> description;

        bool parse(const std::span<Object*> args,
            DefaultParameters* defaults=nullptr, NamedArgs* na=nullptr);

        ArgParser(std::initializer_list<Arg> args) : description{args}
        {
        }
    };
}

#endif
