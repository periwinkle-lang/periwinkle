#ifndef DECOMPILER_H
#define DECOMPILER_H

#include "code_object.h"

namespace compiler
{
    class Decompiler
    {
    private:
        const vm::CodeObject* codeObject;

        int opCodeLenArguments(vm::OpCode code);
        std::string getValueAsString(vm::Object* object);
    public:
        std::string decompile();

        Decompiler(vm::CodeObject* codeObject);
    };
}

#endif
