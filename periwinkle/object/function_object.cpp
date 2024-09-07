#include <algorithm>

#include "function_object.hpp"
#include "string_object.hpp"
#include "int_object.hpp"
#include "list_object.hpp"
#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "vm.hpp"
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

static Object* fnStackCall(Object* callable, Object**& sp)
{
    auto fn = static_cast<FunctionObject*>(callable);
    auto frame = frameFromFunctionObject(fn);
    auto prevVm = VirtualMachine::currentVm;
    auto newVM = VirtualMachine(frame);
    auto result = newVM.execute();
    delete frame;
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
    for (const auto& o : func->callableInfo.defaults->parameters)
    {
        mark(o.second);
    }
}

namespace vm
{
    TypeObject functionObjectType =
    {
        .base = &objectObjectType,
        .name = "Функція",
        .size = sizeof(FunctionObject),
        .callableInfoOffset = offsetof(FunctionObject, callableInfo),
        .alloc = DEFAULT_ALLOC(FunctionObject),
        .dealloc = DEFAULT_DEALLOC(FunctionObject),
        .operators =
        {
            .stackCall = fnStackCall,
        },
        .traverse = (traverseFunction)traverse,
    };
}

FunctionObject* vm::FunctionObject::create(CodeObject* code)
{
    auto functionObject = (FunctionObject*)allocObject(&functionObjectType);
    functionObject->code = code;
    functionObject->callableInfo.arity = code->arity;
    functionObject->callableInfo.flags |= code->isVariadic ? CallableInfo::IS_VARIADIC : 0;
    functionObject->callableInfo.flags |= code->defaults.size() ? CallableInfo::HAS_DEFAULTS : 0;
    functionObject->callableInfo.name = code->name;
    return functionObject;
}
