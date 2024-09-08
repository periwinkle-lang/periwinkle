#include <algorithm>
#include <cstring>

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

static inline Object* _call(FunctionObject* fn)
{
    auto frame = frameFromFunctionObject(fn);
    auto prevVm = VirtualMachine::currentVm;
    auto newVM = VirtualMachine(frame);
    auto result = newVM.execute();
    delete frame;
    VirtualMachine::currentVm = prevVm;
    return result;
}

static Object* fnCall(FunctionObject* fn, std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    auto vm = VirtualMachine::currentVm;
    auto& sp = vm->getFrame()->sp;
    if (args.size() > 0)
    {
        std::memcpy(++sp, args.data(), args.size() * sizeof(Object*));
        sp += args.size() - 1;
    }
    if (fn->callableInfo.flags & CallableInfo::IS_VARIADIC)
    {
        *(++sp) = va;
    }

    auto defaultCount = fn->callableInfo.flags & CallableInfo::HAS_DEFAULTS ?
        fn->callableInfo.defaults->parameters.size() : 0;
    if (defaultCount)
    {
        if (na != nullptr)
        {
            for (size_t i = 0, j = na->count; i < defaultCount; ++i)
            {
                if (j)
                {
                    auto it = std::find(na->indexes.begin(), na->indexes.end(), i);
                    if (it != na->indexes.end())
                    {
                        auto index = it - na->indexes.begin();
                        *(++sp) = na->values[na->count - index - 1];
                        j--;
                        continue;
                    }
                }
                *(++sp) = fn->callableInfo.defaults->parameters[defaultCount - i - 1].second;
            }
        }
        else
        {
            for (size_t i = 0, argLack = fn->callableInfo.arity - args.size(); i < argLack; ++i)
                *(++sp) = fn->callableInfo.defaults->parameters[argLack - i - 1].second;
        }
    }

    Object* result = _call(fn);
    sp -= fn->callableInfo.arity
        + fn->callableInfo.flags & CallableInfo::IS_VARIADIC;
    return result;
}

static Object* fnStackCall(FunctionObject* fn, Object**& sp)
{
    return _call(fn);
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
            .call = (callFunction)fnCall,
            .stackCall = (stackCallFunction)fnStackCall,
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
