#include <format>

#include "vm.hpp"
#include "int_object.hpp"
#include "code_object.hpp"
#include "bool_object.hpp"
#include "string_object.hpp"
#include "exception_object.hpp"
#include "function_object.hpp"
#include "null_object.hpp"
#include "cell_object.hpp"
#include "native_method_object.hpp"
#include "end_iteration_object.hpp"
#include "method_with_instance_object.hpp"
#include "string_vector_object.hpp"
#include "builtins.hpp"
#include "plogger.hpp"
#include "utils.hpp"
#include "periwinkle.hpp"

using namespace vm;

#define READ() *ip++
#define GET_CONST() code->constants[operand]
#define PUSH(object) *(++sp) = object
#define PEEK() *sp
#define POP() *sp--
#define JUMP() ip = &code->code[operand]
#define IP_OFFSET() (ip - &code->code[0] - 1)
#define SET_IP(new_ip) ip = &code->code[(new_ip)]
#define NEXT_OPCODE()         \
    opcode = READ();          \
    a = opcode & OPCODE_MASK; \
    operand = opcode >> 8;

#define BINARY_OP(name, op_name)                        \
case OpCode::name:                                      \
{                                                       \
    auto arg1 = POP();                                  \
    auto arg2 = POP();                                  \
    auto result = arg1->op_name(arg2);                  \
    if (!result) goto error;                            \
    PUSH(result);                                       \
    break;                                              \
}

#define UNARY_OP(name, op_name)                         \
case OpCode::name:                                      \
{                                                       \
    auto arg = POP();                                   \
    auto result = arg->op_name();                       \
    if (!result) goto error;                            \
    PUSH(result);                                       \
    break;                                              \
}

constexpr auto NAME_NOT_DEFINED = "Ім'я \"{}\" не знайдено";

i64 VirtualMachine::getLineno(WORD* ip) const
{
    while (!frame->codeObject->ipToLineno.contains(ip - frame->codeObject->code.data()))
    {
        ip--;
    }
    return static_cast<i64>(frame->codeObject->ipToLineno[ip - frame->codeObject->code.data()]);
}

Object* VirtualMachine::execute()
{
    using enum OpCode;
    const auto code = frame->codeObject;
    const auto& names = code->names;
    auto builtin = getBuiltin();
    auto gc = getCurrentState()->getGC();
    WORD opcode, a, operand;

    for (;;)
    {
    loop:
        NEXT_OPCODE();
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
            auto result = arg1->compare(arg2, (ObjectCompOperator)operand);
            if (!result) goto error;
            PUSH(result);
            break;
        }
        case NOT:
        {
            auto o = POP();
            auto arg = (BoolObject*)o->toBool();
            if (!arg) goto error;
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
            bool condition = objectToBool(o);
            if (getCurrentState()->exceptionOccurred()) goto error;
            if (condition)
                JUMP();
            else
                ip++;
            break;
        }
        case JMP_IF_FALSE:
        {
            auto o = POP();
            bool condition = objectToBool(o);
            if (getCurrentState()->exceptionOccurred()) goto error;
            if (condition == false)
                JUMP();
            else
                ip++;
            break;
        }
        case JMP_IF_TRUE_OR_POP:
        {
            auto o = PEEK();
            bool condition = objectToBool(o);
            if (getCurrentState()->exceptionOccurred()) goto error;
            if (condition)
                JUMP();
            else
            {
                ip++;
                (void)POP();
            }
            break;
        }
        case JMP_IF_FALSE_OR_POP:
        {
            auto o = PEEK();
            bool condition = objectToBool(o);
            if (getCurrentState()->exceptionOccurred()) goto error;
            if (condition == false)
                JUMP();
            else
            {
                ip++;
                (void)POP();
            }
            break;
        }
        case CALL:
        {
            auto argc = operand;
            auto callable = *(sp - argc);

            auto result = callable->stackCall(sp, argc);
            if (!result) goto error;
            PUSH(result);
            break;
        }
        case CALL_NA:
        {
            auto argc = operand;
            auto namedArgNames = (StringVectorObject*)code->constants[READ()];
            auto callable = *(sp - argc);
            auto namedArgs = new NamedArgs;
            auto namedArgCount = namedArgNames->value.size();

            namedArgs->names = namedArgNames->value;
            namedArgs->count = namedArgCount;
            namedArgs->values.reserve(namedArgCount);
            for (size_t i = 0; i < namedArgCount; ++i)
            {
                namedArgs->values.push_back(*(sp--));
            }

            auto result = callable->stackCall(sp, argc - namedArgCount, namedArgs);
            if (!result) goto error;
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
                (NativeMethodObject*)iterator->getAttr("наступний");
            if (nextMethod == nullptr)
            {
                getCurrentState()->setException(
                    &TypeErrorObjectType,
                    std::format("Тип \"{}\" не є ітератором",
                        iterator->objectType->name));
                goto error;
            }

            auto nextElement = nextMethod->call({sp, 1});
            if (!nextElement) goto error;
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
            auto& name = names[operand];
            if (frame->globals->contains(name))
            {
                if (Object* v; (v = (*frame->globals)[name]) != nullptr)
                {
                    PUSH(v);
                    break;
                }
            }

            if (builtin->contains(name))
            {
                PUSH(builtin->at(name));
            }
            else
            {
                getCurrentState()->setException(&NameErrorObjectType,
                    std::format(NAME_NOT_DEFINED, name));
                goto error;
            }
            break;
        }
        case STORE_GLOBAL:
        {
            auto& name = names[operand];
            (*frame->globals)[name] = POP();
            break;
        }
        case DELETE_GLOBAL:
        {
            auto& name = names[operand];
            if ((*frame->globals)[name] != nullptr)
            {
                (*frame->globals)[name] = nullptr;
            }
            else
            {
                getCurrentState()->setException(&NameErrorObjectType,
                    std::format(NAME_NOT_DEFINED, name));
                goto error;
            }
            break;
        }
        case LOAD_LOCAL:
        {
            auto localIdx = operand;
            if (Object* v; (v = bp[localIdx]) != nullptr)
            {
                PUSH(v);
            }
            else
            {
                getCurrentState()->setException(&NameErrorObjectType,
                    std::format(NAME_NOT_DEFINED, frame->codeObject->locals[localIdx]));
                goto error;
            }
            break;
        }
        case STORE_LOCAL:
        {
            bp[operand] = POP();
            break;
        }
        case DELETE_LOCAL:
        {
            auto localIdx = operand;
            if (bp[localIdx] != nullptr)
            {
                bp[localIdx] = nullptr;
            }
            else
            {
                getCurrentState()->setException(&NameErrorObjectType,
                    std::format(NAME_NOT_DEFINED, frame->codeObject->locals[localIdx]));
                goto error;
            }
            break;
        }
        case GET_CELL:
        {
            PUSH(freevars[operand]);
            break;
        }
        case LOAD_CELL:
        {
            auto cell = (CellObject*)freevars[operand];
            PUSH(cell->value);
            break;
        }
        case STORE_CELL:
        {
            auto value = POP();
            auto cell = (CellObject*)freevars[operand];
            cell->value = value;
            break;
        }
        case GET_ATTR:
        {
            auto object = POP();
            auto& name = names[operand];
            auto value = object->getAttr(name);
            if (value == nullptr)
            {
                getCurrentState()->setException(&AttributeErrorObjectType,
                    std::format("Об'єкт \"{}\" не має атрибута \"\"",
                        object->objectType->name, name));
                goto error;
            }
            PUSH(value);
            break;
        }
        case LOAD_METHOD:
        {
            auto object = POP();
            auto& name = names[operand];
            auto function = object->getAttr(name);
            if (function == nullptr)
            {
                getCurrentState()->setException(&AttributeErrorObjectType,
                    std::format("Об'єкт \"{}\" не має атрибута \"{}\"",
                        object->objectType->name, name));
                goto error;
            }

            if (OBJECT_IS(function, &nativeMethodObjectType))
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
            auto argc = operand;
            auto callable = *(sp - argc);

            Object* result;
            if (OBJECT_IS(callable, &methodWithInstanceObjectType))
            {
                auto methodWithInstance = (MethodWithInstanceObject*)callable;
                std::vector<Object*> argv;
                argv.reserve(argc + 1);
                argv.push_back(methodWithInstance->instance);
                argv.insert(argv.begin(), sp - argc + 1, sp);
                result = methodWithInstance->callable->call(argv);
                if (!result) goto error;
                sp -= argc + 1; // Метод
            }
            else
            {
                result = callable->stackCall(sp, argc);
                if (!result) goto error;
            }
            PUSH(result);
            break;
        }
        case CALL_METHOD_NA:
        {
            auto argc = operand;
            auto namedArgNames = (StringVectorObject*)code->constants[READ()];
            auto callable = *(sp - argc);
            auto namedArgs = new NamedArgs;
            auto namedArgCount = namedArgNames->value.size();

            namedArgs->names = namedArgNames->value;
            namedArgs->count = namedArgCount;
            namedArgs->values.resize(namedArgCount);
            for (size_t i = 0; i < namedArgCount; ++i)
            {
                namedArgs->values[namedArgCount - 1 - i ] = (*(sp--));
            }

            Object* result;
            if (OBJECT_IS(callable, &methodWithInstanceObjectType))
            {
                argc -= namedArgCount;
                auto methodWithInstance = (MethodWithInstanceObject*)callable;
                std::vector<Object*> argv;
                argv.reserve(argc + 1);
                argv.push_back(methodWithInstance->instance);
                argv.insert(argv.end(), sp - argc, sp);
                result = methodWithInstance->callable->call(argv, namedArgs);
                if (!result) goto error;
                sp -= argc + 1; // Метод
            }
            else
            {
                result = callable->stackCall(sp, argc - namedArgCount, namedArgs);
                if (!result) goto error;
            }
            PUSH(result);

            delete namedArgs;
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

            if (code->defaults.empty() == false)
            {
                functionObject->callableInfo.defaults = new DefaultParameters;
                functionObject->callableInfo.defaults->parameters.reserve(code->defaults.size());
                for (std::string_view parameterName : codeObject->defaults)
                    functionObject->callableInfo.defaults->parameters.emplace_back(parameterName, POP());
            }

            PUSH(functionObject);
            break;
        }
        case TRY:
        {
            code->getHandlerByStartIp(IP_OFFSET())->stackTop = sp;
            break;
        }
        case CATCH:
        {
            auto exceptionType = static_cast<TypeObject*>(*sp);
            auto endIp = operand;
            auto currentException = getCurrentState()->exceptionOccurred();
            if (isInstance(currentException, *exceptionType))
            {
                PUSH(currentException);
                getCurrentState()->exceptionClear();
            }
            else
            {
                ip = &code->code[endIp];
            }
            break;
        }
        case END_TRY:
        {
        OP_END_TRY:
            auto handler = code->getHandlerByEndIp(IP_OFFSET());
            sp = handler->stackTop;
            handler->stackTop = nullptr;
            if (getCurrentState()->exceptionOccurred()) goto error;
            break;
        }
        case RAISE:
        {
            auto exception = POP();
            if (!isException(exception->objectType))
            {
                getCurrentState()->setException(&TypeErrorObjectType,
                    std::format("Об'єкт \"{}\" не є підкласом типу \"Виняток\"",
                        exception->objectType->name));
                goto error;
            }
            getCurrentState()->setException(exception);
            goto error;
        }
        default:
            plog::fatal << "Опкод не реалізовано: \"" << stringEnum::enumToString((OpCode)a) << "\"";
        }
        gc->gc(frame);
    }

    error:
        auto exception = getCurrentState()->exceptionOccurred();
        plog::passert(exception) << "Віртуальна машина перейшла в блок обробки помилок без викинутої помилки.";
        i64 lineno = getLineno(ip - 1);
        WORD offset = IP_OFFSET();
        if (auto excHandler = code->getExceptionHandler(offset))
        {
            if (!exception->lineno) exception->lineno = lineno;
            auto handler = excHandler.value();
            // Якщо в блоці "спробувати"
            if (offset < handler->firstHandlerAddress)
            {
                SET_IP(handler->firstHandlerAddress);
                goto loop;
            }
            // Якщо в блоці "наприкінці"
            if (handler->finallyAddress && offset >= handler->finallyAddress)
            {
                SET_IP(handler->endAddress);
                NEXT_OPCODE();
                goto OP_END_TRY;
            }
            else // В блоках обробників
            {
                if (handler->finallyAddress)
                {
                    SET_IP(handler->finallyAddress);
                    goto loop;
                }
                SET_IP(handler->endAddress);
                NEXT_OPCODE();
                goto OP_END_TRY;
            }
        }

        exception->addStackTraceItem(frame, lineno);
        return nullptr;
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
