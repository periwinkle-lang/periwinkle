#include <algorithm>

#include "function_object.hpp"
#include "string_object.hpp"
#include "int_object.hpp"
#include "list_object.hpp"
#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "vm.hpp"
#include "validate_args.hpp"
#include "plogger.hpp"

using namespace vm;

static Frame* frameFromFunctionObject(FunctionObject* fn)
{
    auto currentFrame = VirtualMachine::currentVm->getFrame();
    auto newFrame = new Frame;
    newFrame->previous = currentFrame;
    newFrame->codeObject = fn->code;
    newFrame->globals = currentFrame->globals;
    newFrame->bp = currentFrame->sp - fn->code->arity - (int)fn->code->isVariadic;
    newFrame->freevars = newFrame->bp + fn->code->locals.size();
    newFrame->sp = newFrame->freevars + fn->code->cells.size() + fn->code->freevars.size();

    for (size_t i = 0; i < fn->code->cells.size() + fn->code->freevars.size(); ++i)
    {
        newFrame->freevars[i] = CellObject::create(nullptr);
    }

    for (size_t i = 0; i < fn->code->argsAsCells.size(); ++i)
    {
        auto idx = std::find(fn->code->locals.begin(), fn->code->locals.end(),
            fn->code->argsAsCells[i]);
        auto cell = (CellObject*)newFrame->freevars[i];
        auto object = newFrame->bp[idx - fn->code->locals.begin()];
        cell->value = object;
    }

    for (size_t i = 0; i < fn->code->freevars.size(); ++i)
    {
        newFrame->freevars[fn->code->cells.size() + i] = fn->closure[i];
    }
    return newFrame;
}

static Object* fnCall(Object* callable, Object**& sp, WORD argc, NamedArgs* namedArgs)
{
    auto fn = (FunctionObject*)callable;
    ListObject* variadicParameter = nullptr;
    auto defaultCount = fn->code->defaults.size();
    std::vector<size_t> namedArgIndexes;

    if (!validateCall(
        fn->code->arity, &fn->code->defaults, fn->code->isVariadic, fn->code->name,
        false, argc, namedArgs, &namedArgIndexes
    )) return nullptr;

    if (fn->code->isVariadic)
    {
        variadicParameter = new ListObject{ {&listObjectType} };

        if (auto variadicCount = argc - (fn->code->arity - defaultCount); variadicCount > 0)
        {
            for (WORD i = variadicCount; i > 0; --i)
            {
                variadicParameter->items.push_back(*(sp - i + 1));
            }
            sp -= variadicCount;
            argc -= variadicCount; // Змінюється значення вхідного аргументу
        }
        *(++sp) = variadicParameter;
    }

    if (defaultCount)
    {
        if (namedArgs != nullptr)
        {
            for (size_t i = 0, j = namedArgIndexes.size(); i < defaultCount; ++i)
            {
                if (j)
                {
                    auto it = std::find(namedArgIndexes.begin(), namedArgIndexes.end(), i);
                    if (it != namedArgIndexes.end())
                    {
                        auto index = it - namedArgIndexes.begin();
                        *(++sp) = namedArgs->values[namedArgs->count - index - 1];
                        j--;
                        continue;
                    }
                }

                *(++sp) = fn->defaultArguments[defaultCount - i - 1];
            }
        }
        else
        {
            for (size_t i = 0, argLack = fn->code->arity - argc; i < argLack; ++i)
            {
                *(++sp) = fn->defaultArguments[argLack - i - 1];
            }
        }
    }

    auto frame = frameFromFunctionObject(fn);
    auto prevVm = VirtualMachine::currentVm;
    auto newVM = VirtualMachine(frame);
    auto result = newVM.execute();
    delete frame;
    if (variadicParameter != nullptr) delete variadicParameter;
    sp -= fn->code->arity + 1 + (int)fn->code->isVariadic;
    VirtualMachine::currentVm = prevVm;
    return result;
}

static void traverse(FunctionObject* func)
{
    mark(func->code);
    for (auto o : func->closure)
    {
        mark(o);
    }
    for (auto o : func->defaultArguments)
    {
        mark(o);
    }
}

namespace vm
{
    TypeObject functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Функція",
        .size = sizeof(FunctionObject),
        .alloc = DEFAULT_ALLOC(FunctionObject),
        .dealloc = DEFAULT_DEALLOC(FunctionObject),
        .operators =
        {
            .call = fnCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

FunctionObject* vm::FunctionObject::create(CodeObject* code)
{
    auto functionObject = (FunctionObject*)allocObject(&functionObjectType);
    functionObject->code = code;
    return functionObject;
}
