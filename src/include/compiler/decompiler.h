#ifndef DECOMPILER_H
#define DECOMPILER_H

#include "code_object.h"

namespace compiler
{
    class Decompiler
    {
    private:
        int opCodeLenArguments(vm::OpCode code);
        std::string getValueAsString(vm::Object* object);
    public:
        std::string decompile(vm::CodeObject* codeObject);
    };
}

#endif
