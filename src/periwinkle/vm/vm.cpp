#include "vm.h"
#include "int_object.h"
#include "code_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "exception_object.h"
#include "call.h"
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
    (code->ipToLineno.contains(ip_ - &code->code[0]) ? \
    0 : code->ipToLineno[(ip_ - &code->code[0]) - 1])

#define BINARY_OP(name, op_name)                        \
case OpCode::name:                                      \
{                                                       \
    auto op = GET_OPERATOR(PEEK(), op_name);            \
    auto arg1 = POP();                                  \
    auto arg2 = POP();                                  \
    auto result = op(arg1, arg2);                       \
    PUSH(result);                                       \
    break;                                              \
}

#define UNARY_OP(name, op_name)                         \
case OpCode::name:                                      \
{                                                       \
    auto op = GET_OPERATOR(PEEK(), op_name);            \
    auto result = op(POP());                            \
    PUSH(result);                                       \
    break;                                              \
}

#define BOOLEAN_OP(name, op)                               \
case OpCode::name:                                         \
{                                                          \
    auto o1 = POP();                                       \
    auto o2 = POP();                                       \
    auto arg1 = (BoolObject*)GET_OPERATOR(o1, toBool)(o1); \
    auto arg2 = (BoolObject*)GET_OPERATOR(o2, toBool)(o2); \
    PUSH(BoolObject::create(arg1->value op arg2->value));  \
    break;                                                 \
}

void VirtualMachine::throwException(ObjectType* exception, std::string message, WORD lineno)
{
    if (lineno)
    {
        std::cerr << "На стрічці " << lineno << " знадено помилку" << std::endl;
    }
    std::cerr << exception->name << ": " << message << std::endl;
    exit(1);
}

void VirtualMachine::execute(Frame* frame)
{
    using enum OpCode;
    const auto code = frame->codeObject;
    const auto& names = code->names;

    WORD* ip = &code->code[0];
    Object** sp = &stack[0];

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
        UNARY_OP(INC, inc)
        UNARY_OP(DEC, dec)
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
            auto op = arg1->objectType->comparisonOperators[READ()];
            PUSH(op(arg1, arg2));
            break;
        }
        BOOLEAN_OP(AND, &&)
        BOOLEAN_OP(OR, ||)
        case NOT:
        {
            auto o = POP();
            auto arg = (BoolObject*)GET_OPERATOR(o, toBool)(o);
            PUSH(BoolObject::create(!(arg->value)));
            break;
        }
        case JMP:
        {
            JUMP();
            break;
        }
        case JMP_IF_TRUE:
        {
            auto op = GET_OPERATOR(PEEK(), toBool);
            auto condition = (BoolObject*)op(POP());
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
            auto op = GET_OPERATOR(PEEK(), toBool);
            auto condition = (BoolObject*)op(POP());
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
        case LOAD_CONST:
        {
            PUSH(GET_CONST());
            break;
        }
        case LOAD_GLOBAL:
        {
            auto& name = names[READ()];
            if (frame->globals.contains(name))
            {
                PUSH(frame->globals[name]);
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
            frame->globals[name] = POP();
            break;
        }
        case LOAD_NAME:
        {
            auto& name = names[READ()];
            if (frame->globals.contains(name))
            {
                PUSH(frame->globals[name]);
            }
            // TODO: Локальні змінні мають перевірятись після глобальних та перед builtin
            else if (builtin.contains(name))
            {
                PUSH(builtin[name]);
            }
            else
            {
                plog::fatal << "Локальні змінні ще не реалізовані";
            }
            break;
        }
        case CALL:
        {
            auto functionObject = POP();
            auto argc = READ();
            if (IS_CALLABLE(functionObject))
            {
                auto result = objectCall(functionObject, sp, argc);
                PUSH(result);
            }
            else
            {
                throwException(&TypeErrorObjectType,
                    utils::format("Об'єкт типу \"%s\" не може бути викликаний",
                        functionObject->objectType->name.c_str()),
                    GET_LINENO(ip-1)
                );
            }
            break;
        }
        //case MAKE_FUNCTION:
        //{
        //    auto codeObject = (CodeObject*)POP();
        //    auto cellCount = READ();
        //    auto functionObject = (FunctionObject*)allocObject(&functionObjectType);

        //    // Комірки передаються через стек
        //    for (WORD i = 0; i < cellCount; ++i)
        //    {
        //        auto cell = POP();
        //        functionObject->cells.push_back(cell);
        //    }
        //    PUSH(functionObject);
        //    break;
        //}
        case HALT:
        {
            return;
        }
        default:
            plog::fatal << "Опкод не реалізовано: \"" << stringEnum::enumToString((OpCode)a) << "\"";
        }
    }
}
