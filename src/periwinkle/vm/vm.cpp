#include "vm.h"
#include "int_object.h"
#include "code_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "exception_object.h"
#include "function_object.h"
#include "null_object.h"
#include "cell_object.h"
#include "builtins.h"
#include "plogger.h"
#include "utils.h"

using namespace vm;

#define READ() *ip++
#define GET_CONST() code->constants[READ()]
#define PUSH(object) *(++sp) = object
#define PEEK() *sp
#define POP() *sp--
#define JUMP() ip = &code->code[*ip]
#define GET_LINENO(ip_) \
    (frame->codeObject->ipToLineno.contains(ip_ - &frame->codeObject->code[0]) ? \
    0 : frame->codeObject->ipToLineno[(ip_ - &frame->codeObject->code[0]) - 1])

#define BINARY_OP(name, op_name)                        \
case OpCode::name:                                      \
{                                                       \
    auto arg1 = POP();                                  \
    auto arg2 = POP();                                  \
    auto result = Object::op_name(arg1, arg2);          \
    PUSH(result);                                       \
    break;                                              \
}

#define UNARY_OP(name, op_name)                         \
case OpCode::name:                                      \
{                                                       \
    auto arg = POP();                                   \
    auto result = Object::op_name(arg);                 \
    PUSH(result);                                       \
    break;                                              \
}

#define BOOLEAN_OP(name, op)                            \
case OpCode::name:                                      \
{                                                       \
    auto o1 = POP();                                    \
    auto o2 = POP();                                    \
    auto arg1 = (BoolObject*)Object::toBool(o1);        \
    auto arg2 = (BoolObject*)Object::toBool(o2);        \
    PUSH(P_BOOL(arg1->value op arg2->value));           \
    break;                                              \
}

void VirtualMachine::throwException(
    TypeObject * exception, std::string message, WORD lineno)
{
    if (!lineno)
    {
        lineno = GET_LINENO(ip - 1);
    }
    std::cerr << "На стрічці " << lineno << " знайдено помилку" << std::endl;
    std::cerr << exception->name << ": " << message << std::endl;
    exit(1);
}

Object* VirtualMachine::execute()
{
    using enum OpCode;
    const auto code = frame->codeObject;
    const auto& names = code->names;
    auto builtin = getBuiltin();

    for (;;)
    {
        auto a = READ();
        switch ((OpCode)a)
        {
        case POP:
        {
            --sp;
            break;
        }
        case DUP:
        {
            auto object = PEEK();
            PUSH(object);
            break;
        }
        UNARY_OP(POS, pos)
        UNARY_OP(NEG, neg)
        BINARY_OP(ADD, add)
        BINARY_OP(SUB, sub)
        BINARY_OP(MUL, mul)
        BINARY_OP(DIV, div)
        BINARY_OP(MOD, mod)
        BINARY_OP(FLOOR_DIV, floorDiv)
        case COMPARE:
        {
            auto arg1 = POP();
            auto arg2 = POP();
            auto result = Object::compare(arg1, arg2, (ObjectCompOperator)READ());
            PUSH(result);
            break;
        }
        BOOLEAN_OP(AND, &&)
        BOOLEAN_OP(OR, ||)
        case NOT:
        {
            auto o = POP();
            auto arg = (BoolObject*)Object::toBool(o);
            PUSH(P_BOOL(!arg->value));
            break;
        }
        case JMP:
        {
            JUMP();
            break;
        }
        case JMP_IF_TRUE:
        {
            auto o = POP();
            auto condition = (BoolObject*)Object::toBool(o);
            if ((condition)->value)
            {
                JUMP();
            }
            else
            {
                ip++;
            }
            break;
        }
        case JMP_IF_FALSE:
        {
            auto o = POP();
            auto condition = (BoolObject*)Object::toBool(o);
            if (((BoolObject*)condition)->value == false)
            {
                JUMP();
            }
            else
            {
                ip++;
            }
            break;
        }
        case CALL:
        {
            auto argc = READ();
            auto callable = *(sp - argc);

            auto result = Object::call(callable, sp, argc);
            PUSH(result);
            break;
        }
        case RETURN:
        {
            auto returnValue = POP();
            return returnValue;
        }
        case LOAD_CONST:
        {
            PUSH(GET_CONST());
            break;
        }
        case LOAD_GLOBAL:
        {
            auto& name = names[READ()];
            if (frame->globals->contains(name))
            {
                PUSH((*frame->globals)[name]);
            }
            else if (builtin->contains(name))
            {
                PUSH(builtin->at(name));
            }
            else
            {
                throwException(&NameErrorObjectType,
                    utils::format("Імені \"%s\" не існує", name.c_str()), GET_LINENO(ip-1));
            }
            break;
        }
        case STORE_GLOBAL:
        {
            auto& name = names[READ()];
            (*frame->globals)[name] = POP();
            break;
        }
        case LOAD_LOCAL:
        {
            PUSH(bp[READ()]);
            break;
        }
        case STORE_LOCAL:
        {
            bp[READ()] = POP();
            break;
        }
        case GET_CELL:
        {
            PUSH(freevars[READ()]);
            break;
        }
        case LOAD_CELL:
        {
            auto cell = (CellObject*)freevars[READ()];
            PUSH(cell->value);
            break;
        }
        case STORE_CELL:
        {
            auto value = POP();
            auto cell = (CellObject*)freevars[READ()];
            cell->value = value;
            break;
        }
        case GET_ATTR:
        {
            auto object = POP();
            auto& name = names[READ()];
            auto value = Object::getAttr(object, name);
            if (value == nullptr)
            {
                throwException(&AttributeErrorObjectType,
                    utils::format("Об'єкт \"%s\" не має атрибута \"%s\"",
                        object->objectType->name.c_str(), name.c_str()));
            }
            PUSH(value);
            break;
        }
        case LOAD_METHOD:
        {
            auto object = POP();
            auto& name = names[READ()];
            auto function = Object::getAttr(object, name);
            if (function == nullptr)
            {
                throwException(&AttributeErrorObjectType,
                    utils::format("Об'єкт \"%s\" не має атрибута \"%s\"",
                        object->objectType->name.c_str(), name.c_str()));
            }

            if (function->objectType->type == ObjectTypes::NATIVE_METHOD)
            {
                PUSH(object);
            }
            else
            {
                PUSH(nullptr);
            }

            PUSH(function);
            break;
        }
        case CALL_METHOD:
        {
            auto argc = READ();
            auto callable = *(sp - argc);

            auto result = Object::call(callable, sp, argc);
            PUSH(result);
            break;
        }
        case MAKE_FUNCTION:
        {
            auto codeObject = (CodeObject*)POP();
            auto functionObject = FunctionObject::create(codeObject);

            for (WORD i = 0; i < codeObject->freevars.size(); ++i)
            {
                functionObject->closure.push_back((CellObject*)POP());
            }

            PUSH(functionObject);
            break;
        }
        case HALT:
        {
            return &P_null;
        }
        default:
            plog::fatal << "Опкод не реалізовано: \"" << stringEnum::enumToString((OpCode)a) << "\"";
        }
    }
}

Frame* vm::VirtualMachine::getFrame() const
{
    return frame;
}

VirtualMachine* vm::VirtualMachine::currentVm = nullptr;

vm::VirtualMachine::VirtualMachine(Frame* frame)
    :
    frame(frame),
    ip(&frame->codeObject->code[0]),
    sp(frame->sp),
    bp(frame->bp),
    freevars(frame->freevars)
{
    currentVm = this;
}
