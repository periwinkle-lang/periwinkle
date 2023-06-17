#include "vm.h"
#include "int_object.h"
#include "code_object.h"
#include "bool_object.h"
#include "string_object.h"
#include "exception_object.h"
#include "function_object.h"
#include "null_object.h"
#include "cell_object.h"
#include "native_method_object.h"
#include "end_iteration_object.h"
#include "method_with_instance_object.h"
#include "string_vector_object.h"
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
        UNARY_OP(GET_ITER, getIter)
        BINARY_OP(ADD, add)
        BINARY_OP(SUB, sub)
        BINARY_OP(MUL, mul)
        BINARY_OP(DIV, div)
        BINARY_OP(MOD, mod)
        BINARY_OP(FLOOR_DIV, floorDiv)
        case IS:
        {
            auto o1 = POP();
            auto o2 = POP();
            PUSH(P_BOOL(o1 == o2));
            break;
        }
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
        case CALL_NA:
        {
            auto argc = READ();
            auto namedArgNames = (StringVectorObject*)GET_CONST();
            auto callable = *(sp - argc);
            auto namedArgs = new NamedArgs;
            auto namedArgCount = namedArgNames->value.size();

            namedArgs->names = &namedArgNames->value;
            namedArgs->count = namedArgCount;
            for (size_t i = 0; i < namedArgCount; ++i)
            {
                namedArgs->values.push_back(*(sp--));
            }

            auto result = Object::call(callable, sp, argc - namedArgCount, namedArgs);
            PUSH(result);
            delete namedArgs;
            break;
        }
        case RETURN:
        {
            auto returnValue = POP();
            return returnValue;
        }
        case FOR_EACH:
        {
            auto iterator = PEEK();
            auto nextMethod =
                (NativeMethodObject*)Object::getAttr(iterator, "наступний");
            if (nextMethod == nullptr)
            {
                throwException(
                    &TypeErrorObjectType,
                    utils::format("Тип \"%s\" не є ітератором",
                        iterator->objectType->name.c_str()));
            }

            auto nextElement = callNativeMethod(iterator, nextMethod, {});
            if (nextElement != &P_endIter)
            {
                PUSH(nextElement);
                ip++; // Пропуск аргументу опкоду
            }
            else
            {
                sp--; // Видалення зі стека ітератора
                JUMP(); // Завершення циклу
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
                PUSH(MethodWithInstanceObject::create(object, function));
            }
            else
            {
                PUSH(function);
            }

            break;
        }
        case CALL_METHOD:
        {
            auto argc = READ();
            auto callable = *(sp - argc);

            Object* result;
            if (callable->objectType->type == ObjectTypes::METHOD_WITH_INSTANCE)
            {
                auto methodWithInstance = (MethodWithInstanceObject*)callable;
                result = callNativeMethod(
                    methodWithInstance->instance,
                    (NativeMethodObject*)methodWithInstance->callable,
                    { sp - argc + 1, argc });
                sp -= argc + 1; // Метод
            }
            else
            {
                result = Object::call(callable, sp, argc);
            }
            PUSH(result);
            break;
        }
        case CALL_METHOD_NA:
        {
            plog::fatal << "CALL_METHOD_NA ще не реалізовано";
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

            for (WORD i = 0; i < codeObject->defaults.size(); ++i)
            {
                functionObject->defaultArguments.push_back(POP());
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
