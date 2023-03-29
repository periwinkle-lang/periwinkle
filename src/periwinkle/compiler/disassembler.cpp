#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "disassembler.h"
#include "types.h"
#include "utils.h"
#include "native_function_object.h"
#include "int_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "real_object.h"
#include "plogger.h"

using namespace compiler;
using vm::OpCode;
using enum vm::OpCode;

int compiler::Disassembler::opCodeLenArguments(OpCode code)
{
    switch (code)
    {
    case JMP:
    case JMP_IF_FALSE:
    case JMP_IF_TRUE:
    case LOAD_CONST:
    case LOAD_GLOBAL:
    case STORE_GLOBAL:
    case LOAD_LOCAL:
    case STORE_LOCAL:
    case LOAD_CELL:
    case STORE_CELL:
    case GET_CELL:
    case GET_ATTR:
    case CALL:
    case COMPARE:
    case LOAD_METHOD:
    case CALL_METHOD:
    case FOR_EACH:
        return 1;
    default:
        return 0;
    }
}

std::string compiler::Disassembler::getValueAsString(vm::Object* object)
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
    case CODE:
    {
        std::stringstream ss;
        ss << "<CodeObject " << ((vm::CodeObject*)object)->name << ": " << object << ">";
        return ss.str();

    }
    default:
        plog::fatal << "Не реалізовано для типу: \"" << object->objectType->name << "\"";
    }
}

std::string compiler::Disassembler::disassemble(vm::CodeObject* codeObject)
{
    std::stringstream out;
    vm::WORD lineno = 0;
    std::vector<vm::CodeObject*> codeObjects;

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
                if (argumentAsObject->objectType->type == vm::ObjectTypes::CODE)
                {
                    codeObjects.push_back((vm::CodeObject*)argumentAsObject);
                }
            }
            else if (op == STORE_GLOBAL || op == LOAD_GLOBAL
                || op == GET_ATTR  || op == LOAD_METHOD)
            {
                auto& name = codeObject->names[argument];
                out << "(" << name << ")";
            }
            else if (op == COMPARE)
            {
                out << "(";
                using enum vm::ObjectCompOperator;
                switch ((vm::ObjectCompOperator)argument)
                {
                case EQ: out << "=="; break;
                case NE: out << "!="; break;
                case GT: out << "більше"; break;
                case GE: out << "більше="; break;
                case LT: out << "менше"; break;
                case LE: out << "менше="; break;
                }
                out << ")";
            }
            else if (op == LOAD_LOCAL || op == STORE_LOCAL)
            {
                auto& name = codeObject->locals[argument];
                out << "(" << name << ")";
            }
            else if (op == LOAD_CELL || op == STORE_CELL || op == GET_CELL)
            {
                std::string name;
                if (argument < codeObject->cells.size())
                {
                    name = codeObject->cells[argument];
                }
                else
                {
                    name = codeObject->freevars[argument - codeObject->cells.size()];
                }
                out << "(" << name << ")";
            }
        }

        out << std::endl;

    }

    for (auto value : codeObjects)
    {
        out << std::endl;
        out << "Disassemble " << getValueAsString(value) << ":" << std::endl;
        out << disassemble(value);
    }

    return out.str();
}
