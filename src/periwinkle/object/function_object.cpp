#include "function_object.h"
#include "utils.h"
#include "vm.h"

#include "string_object.h"
#include "int_object.h"
using namespace vm;

static Frame* frameFromFunctionObject(FunctionObject* fn)
{
    auto currentFrame = VirtualMachine::currentVm->getFrame();
    auto newFrame = new Frame;
    newFrame->previous = currentFrame;
    newFrame->codeObject = fn->code;
    newFrame->globals = currentFrame->globals;
    newFrame->bp = currentFrame->sp - fn->code->arity;
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

Object* fnCall(Object* callable, Object** sp, WORD argc)
{
    auto fn = (FunctionObject*)callable;
    if (fn->code->arity != argc)
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
    VirtualMachine::currentVm = prevVm;
    return result;
}

Object* allocFunction();

namespace vm
{
    ObjectType functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Function",
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
