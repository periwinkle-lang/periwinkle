#ifndef CODE_OBJECT_H
#define CODE_OBJECT_H

#include <string>
#include <vector>
#include "object.h"
#include "vm.h"

namespace vm
{
    extern ObjectType codeObjectType;

    struct CodeObject : Object
    {
        std::string name;
        std::vector<WORD> code;
        std::vector<Object*> constants;
        std::vector<std::string> names; // Імена змінних

        static CodeObject* create(std::string name);
    };
}

#endif
