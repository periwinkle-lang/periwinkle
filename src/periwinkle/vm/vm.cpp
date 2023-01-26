﻿#include "vm.h"
#include "int_object.h"
#include "code_object.h"
#include "call.h"
#include "builtins.h"
#include "plogger.h"


using namespace vm;

#define READ() *ip++
#define GET_CONST() code->constants[READ()]
#define PUSH(object) *(++sp) = object
#define PEEK() *sp
#define POP() *sp--

#define BINARY_OP(name, op_name)                        \
case OpCode::name:                                      \
{                                                       \
    auto op = GET_OPERATOR(PEEK(), op_name);            \
    auto result = objectCall(op, sp, 2);                \
    PUSH(result);                                       \
    break;                                              \
}

#define UNARY_OP(name, op_name)                         \
case OpCode::name:                                      \
{                                                       \
    auto op = GET_OPERATOR(PEEK(), op_name);            \
    auto result = objectCall(op, sp, 2);                \
    PUSH(result);                                       \
    break;                                              \
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
        UNARY_OP(NEG, neg)
        BINARY_OP(ADD, add)
        BINARY_OP(SUB, sub)
        BINARY_OP(MUL, mul)
        BINARY_OP(DIV, div)
        BINARY_OP(MOD, mod)
        case JMP:
        {
            ip += *ip;
            break;
        }
        // TODO: доробити JMP_IF інструкції, щоб вони кастували будь який тип до bool
        //case JMP_IF_TRUE:
        //{
        //    auto condition = POP();
        //    if (((BoolObject*)condition)->value)
        //    {
        //        ip += *ip;
        //    }
        //    break;
        //}
        //case JMP_IF_FALSE:
        //{
        //    auto condition = POP();
        //    if (((BoolObject*)condition)->value == false)
        //    {
        //        ip += *ip;
        //    }
        //    break;
        //}
        case LOAD_CONST:
        {
            PUSH(GET_CONST());
            break;
        }
        case LOAD_GLOBAL:
        {
            auto& name = names[READ()];
            PUSH(frame->globals[name]);
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
            if (builtin.contains(name))
            {
                PUSH(builtin[name]);
            }
            else if (frame->globals.contains(name))
            {
                PUSH(frame->globals[name]);
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
                // TODO: викинути нормальну помилку для користувача
                plog::fatal << "Неможливо викликати об'єкт: " << functionObject;
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
