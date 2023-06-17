#include <algorithm>

#include "function_object.h"
#include "string_object.h"
#include "int_object.h"
#include "array_object.h"
#include "native_method_object.h"
#include "string_vector_object.h"
#include "vm.h"
#include "validate_args.h"

#include "plogger.h"

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

Object* fnCall(Object* callable, Object**& sp, WORD argc, NamedArgs* namedArgs)
{
    auto fn = (FunctionObject*)callable;
    ArrayObject* variadicParameter = nullptr;
    auto defaultCount = fn->code->defaults.size();
    std::vector<size_t> namedArgIndexes;

    validateCall(
        fn->code->arity, &fn->code->defaults, fn->code->isVariadic, fn->code->name,
        false, argc, namedArgs, &namedArgIndexes
    );

    if (fn->code->isVariadic)
    {
        variadicParameter = ArrayObject::create();

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

namespace vm
{
    TypeObject functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Функція",
        .type = ObjectTypes::FUNCTION,
        .alloc = DEFAULT_ALLOC(FunctionObject),
        .operators =
        {
            .call = fnCall,
        },
    };
}

FunctionObject* vm::FunctionObject::create(CodeObject* code)
{
    auto functionObject = (FunctionObject*)allocObject(&functionObjectType);
    functionObject->code = code;
    return functionObject;
}
