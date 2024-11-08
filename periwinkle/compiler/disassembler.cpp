#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "disassembler.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "object.hpp"
#include "native_function_object.hpp"
#include "int_object.hpp"
#include "bool_object.hpp"
#include "string_object.hpp"
#include "real_object.hpp"
#include "string_vector_object.hpp"
#include "code_object.hpp"
#include "null_object.hpp"
#include "plogger.hpp"
#include "keyword.hpp"

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
    case JMP_IF_FALSE_OR_POP:
    case JMP_IF_TRUE_OR_POP:
    case LOAD_CONST:
    case LOAD_GLOBAL:
    case STORE_GLOBAL:
    case DELETE_GLOBAL:
    case LOAD_LOCAL:
    case STORE_LOCAL:
    case DELETE_LOCAL:
    case LOAD_CELL:
    case STORE_CELL:
    case GET_CELL:
    case GET_ATTR:
    case CALL:
    case COMPARE:
    case LOAD_METHOD:
    case CALL_METHOD:
    case FOR_EACH:
    case TRY:
    case CATCH:
    case UNARY_OP:
    case BINARY_OP:
    case IS:
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
        return std::string{ ((vm::BoolObject*)object)->value ? Keyword::K_TRUE : Keyword::K_FALSE };
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
        return std::string{ Keyword::K_NULL };
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
        OpCode op = (OpCode)(codeObject->code[ip] & vm::OPCODE_MASK);
        if (codeObject->ipToLineno.contains(ip))
        {
            if (auto newLineno = codeObject->ipToLineno.at(ip); newLineno != lineno)
            {
                lineno = newLineno;
                out << lineno << std::endl;
            }
        }

        out << std::right << std::setw(4) << ip << " ";
        out << std::left << std::setw(20);
        out << vm::stringEnum::enumToString(op);

        switch (opCodeLenArguments(op))
        {
        case 1:
        {
            vm::WORD argument = codeObject->code[ip] >> 8;
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
            else if (op == STORE_GLOBAL || op == LOAD_GLOBAL || op == DELETE_GLOBAL
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
                case EQ: out << Keyword::EQUAL_EQUAL; break;
                case NE: out << Keyword::NOT_EQUAL; break;
                case GT: out << Keyword::GREATER; break;
                case GE: out << Keyword::GREATER_EQUAL; break;
                case LT: out << Keyword::LESS; break;
                case LE: out << Keyword::LESS_EQUAL; break;
                }
                out << ")";
            }
            else if (op == LOAD_LOCAL || op == STORE_LOCAL || op == DELETE_LOCAL)
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
            else if (op == BINARY_OP || op == UNARY_OP)
            {
                out << "(";
                switch (static_cast<vm::ObjectOperatorOffset>(argument))
                {
                case vm::ObjectOperatorOffset::ADD: out << Keyword::ADD; break;
                case vm::ObjectOperatorOffset::SUB: out << Keyword::SUB; break;
                case vm::ObjectOperatorOffset::DIV: out << Keyword::DIV; break;
                case vm::ObjectOperatorOffset::MUL: out << Keyword::MUL; break;
                case vm::ObjectOperatorOffset::MOD: out << Keyword::MOD; break;
                case vm::ObjectOperatorOffset::FLOOR_DIV: out << Keyword::FLOOR_DIV; break;
                case vm::ObjectOperatorOffset::POS: out << Keyword::POS; break;
                case vm::ObjectOperatorOffset::NEG: out << Keyword::NEG; break;
                case vm::ObjectOperatorOffset::GET_ITER: out << "getIter"; break;
                }
                out << ")";
            }
            break;
        }
        case 2:
        {
            vm::WORD argument1 = codeObject->code[ip] >> 8;
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
