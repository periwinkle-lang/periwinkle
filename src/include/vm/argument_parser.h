﻿#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <vector>
#include <span>
#include <initializer_list>

#include "object.h"

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

        void parse(const std::span<Object*> args);

        ArgParser(std::initializer_list<Arg> args) : description{args}
        {
        }
    };

}

#endif