#include "function_object.h"
#include "utils.h"
#include "vm.h"
#include "string_object.h"
#include "int_object.h"
#include "array_object.h"
#include "native_method_object.h"

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

Object* fnCall(Object* callable, Object**& sp, WORD argc)
{
    auto fn = (FunctionObject*)callable;
    ArrayObject* variadicParameter = nullptr;
    if (fn->code->isVariadic)
    {
        if (fn->code->arity > argc)
        {
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
                utils::format(
                    "Функція \"%s\" очікує мінімум %u аргументів, натомість передано %u",
                    fn->code->name.c_str(), fn->code->arity, argc)
            );
        }

        variadicParameter = ArrayObject::create();

        if (auto variadicCount = argc - fn->code->arity; variadicCount > 0)
        {
            static auto arrayPush =
                ((NativeMethodObject*)arrayObjectType.attributes["додати"])->method;
            for (WORD i = variadicCount; i > 0 ; --i)
            {
                arrayPush(variadicParameter, { sp - i + 1, 1 }, nullptr);
            }
            sp -= variadicCount;
        }
        *(++sp) = variadicParameter;
    }
    else if (fn->code->arity != argc)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format("Функція \"%s\" очікує %u аргументів, натомість передано %u",
                fn->code->name.c_str(), fn->code->arity, argc)
        );
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

Object* allocFunction();

namespace vm
{
    TypeObject functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Функція",
        .type = ObjectTypes::FUNCTION,
        .alloc = &allocFunction,
        .operators =
        {
            .call = fnCall,
        },
    };
}

Object* allocFunction()
{
    auto functionObject = new FunctionObject;
    functionObject->objectType = &functionObjectType;
    return (Object*)functionObject;
}

FunctionObject* vm::FunctionObject::create(CodeObject* code)
{
    auto functionObject = (FunctionObject*)allocObject(&functionObjectType);
    functionObject->code = code;
    return functionObject;
}
