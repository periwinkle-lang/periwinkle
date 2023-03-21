#ifndef CODE_OBJECT_H
#define CODE_OBJECT_H

#include <string>
#include <vector>
#include <map>

#include "object.h"
#include "vm.h"

namespace vm
{
    extern TypeObject codeObjectType;

    struct CodeObject : Object
    {
        std::string name;
        WORD arity=0; // Не враховує варіативний аргумент
        bool isVariadic=false;
        std::vector<WORD> code;
        std::vector<Object*> constants;
        std::vector<std::string> names; // Імена змінних
        std::vector<std::string> locals;
        std::vector<std::string> cells;
        std::vector<std::string> freevars;
        std::vector<std::string> argsAsCells; // Імена аргументів, які є комірками
        // Ключ - номер опкода, значення - номер лінії в коді
        std::map<WORD, WORD> ipToLineno;

        static CodeObject* create(std::string name);
    };
}

#endif
