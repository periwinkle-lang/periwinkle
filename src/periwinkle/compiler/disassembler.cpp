﻿#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "disassembler.h"
#include "types.h"
#include "utils.h"
#include "object.h"
#include "native_function_object.h"
#include "int_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "real_object.h"
#include "string_vector_object.h"
#include "code_object.h"
#include "null_object.h"
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
    case CALL_NA:
    case CALL_METHOD_NA:
        return 2;
    default:
        return 0;
    }
}

std::string compiler::Disassembler::getValueAsString(vm::Object* object)
{
    if (OBJECT_IS(object, &vm::intObjectType))
    {
        return std::to_string(((vm::IntObject*)object)->value);
    }
    else if (OBJECT_IS(object, &vm::boolObjectType))
    {
        return ((vm::BoolObject*)object)->value ? "істина" : "хиба";
    }
    else if (OBJECT_IS(object, &vm::stringObjectType))
    {
        return "\"" + utils::escapeString(((vm::StringObject*)object)->asUtf8()) + "\"";
    }
    else if (OBJECT_IS(object, &vm::realObjectType))
    {
        std::stringstream ss;
        ss << (((vm::RealObject*)object)->value);
        return ss.str();
    }
    else if (OBJECT_IS(object, &vm::nullObjectType))
    {
        return "ніц";
    }
    else if (OBJECT_IS(object, &vm::codeObjectType))
    {
        std::stringstream ss;
        ss << "<CodeObject " << ((vm::CodeObject*)object)->name << ": " << object << ">";
        return ss.str();
    }
    else if (OBJECT_IS(object, &vm::stringVectorObjectType))
    {
        std::stringstream ss;
        const auto& args = ((vm::StringVectorObject*)object)->value;
        ss << args[0];
        for (size_t i = 1; i < args.size(); ++i)
        {
            ss << ", " << args[i];
        }
        return ss.str();
    }
    else
    {
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

        switch (opCodeLenArguments(op))
        {
        case 1:
        {
            vm::WORD argument = codeObject->code[++ip];
            out << argument;
            if (op == LOAD_CONST)
            {
                auto argumentAsObject = codeObject->constants[argument];
                out << "(" << getValueAsString(argumentAsObject) << ")";
                if (OBJECT_IS(argumentAsObject, &vm::codeObjectType))
                {
                    codeObjects.push_back((vm::CodeObject*)argumentAsObject);
                }
            }
            else if (op == STORE_GLOBAL || op == LOAD_GLOBAL
                || op == GET_ATTR || op == LOAD_METHOD)
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
            break;
        }
        case 2:
        {
            vm::WORD argument1 = codeObject->code[++ip];
            vm::WORD argument2 = codeObject->code[++ip];
            out << argument1;

            out << ", " << argument2;

            if (op == CALL_NA || op == CALL_METHOD_NA)
            {
                out << "(" << getValueAsString(codeObject->constants[argument2]) << ")";
            }
            break;
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
