#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include "code_object.hpp"

namespace compiler
{
    class Disassembler
    {
    private:
        int opCodeLenArguments(vm::OpCode code);
        std::string getValueAsString(vm::Object* object);
    public:
        std::string disassemble(vm::CodeObject* codeObject);
    };
}

#endif
