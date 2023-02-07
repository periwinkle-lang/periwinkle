#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "decompiler.h"
#include "types.h"
#include "utils.h"
#include "call.h"
#include "native_function_object.h"
#include "int_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "real_object.h"
#include "plogger.h"

using namespace compiler;
using vm::OpCode;
using enum vm::OpCode;

int compiler::Decompiler::opCodeLenArguments(OpCode code)
{
    switch (code)
    {
    case JMP:
    case JMP_IF_FALSE:
    case JMP_IF_TRUE:
    case LOAD_CONST:
    case LOAD_GLOBAL:
    case STORE_GLOBAL:
    case LOAD_NAME:
    case CALL:
    case COMPARE:
        return 1;
    default:
        return 0;
    }
}

std::string compiler::Decompiler::getValueAsString(vm::Object* object)
{
    using enum vm::ObjectTypes;
    switch (object->objectType->type)
    {
    case INTEGER:
    {
        return std::to_string(((vm::IntObject*)object)->value);
    }
    case BOOL:
    {
        return ((vm::BoolObject*)object)->value ? "істина" : "хиба";
    }
    case STRING:
    {
        return "\"" + utils::escapeString(((vm::StringObject*)object)->value) + "\"";
    }
    case REAL:
    {
        std::stringstream ss;
        ss << (((vm::RealObject*)object)->value);
        return ss.str();
    }
    case NULL_:
    {
        return "нич";
    }
    default:
        plog::fatal << "Нереалізовано для типу: \"" << object->objectType->name << "\"";
    }
}

std::string compiler::Decompiler::decompile()
{
    std::stringstream out;
    vm::WORD lineno = 0;

    for (vm::WORD ip = 0; ip < codeObject->code.size(); ++ip)
    {
        OpCode op = (OpCode)codeObject->code[ip];
        if (codeObject->ipToLineno.contains(ip))
        {
            if (auto newLineno = codeObject->ipToLineno.at(ip); newLineno != lineno)
            {
                lineno = newLineno;
                out << lineno << std::endl;
            }
        }

        out << std::right << std::setw(4) << ip << " ";
        out << std::left << std::setw(16);
        out << vm::stringEnum::enumToString(op);

        int argumentLen = opCodeLenArguments(op);
        if (argumentLen == 1)
        {
            vm::WORD argument = codeObject->code[++ip];
            out << argument;
            if (op == LOAD_CONST)
            {
                auto argumentAsObject = codeObject->constants[argument];
                out << "(" << getValueAsString(argumentAsObject) << ")";
            }
            else if (op == LOAD_NAME || op == STORE_GLOBAL || op == LOAD_GLOBAL)
            {
                auto& name = codeObject->names[argument];
                out << "(" << name << ")";
            }
            else if (op == COMPARE)
            {
                out << "(";
                switch (argument)
                {
                case 0: out << "=="; break;
                case 1: out << "!="; break;
                case 2: out << "більше"; break;
                case 3: out << "більше="; break;
                case 4: out << "менше"; break;
                case 5: out << "менше="; break;
                }
                out << ")";
            }
        }


        out << std::endl;
    }

    return out.str();
}

compiler::Decompiler::Decompiler(vm::CodeObject* codeObject) : codeObject(codeObject)
{
}
