#include "call.h"
#include "native_function_object.h"
#include "plogger.h"

using namespace vm;

Object* vm::objectCall(Object* callable, Object** stack, WORD argc)
{
    if (callable->objectType->type == ObjectTypes::FUNCTION)
    {
        // TODO
        plog::fatal << "Виклик FunctionObject ще не реалізовано";
    }
    else if (callable->objectType->type == ObjectTypes::NATIVE_FUNCTION)
    {
        auto nativeFunction = (NativeFunctionObject*)callable;
        
        if ((int)argc != nativeFunction->arity)
        {
            // TODO: викинути нормальну помилку для користувача
            plog::fatal << "Передана неправильна кількість аргументів";
        }
        
        Object** args = new Object*[nativeFunction->arity];
        for (int i = 0; i < nativeFunction->arity; ++i)
        {
            args[i] = *stack--;
        }
        auto result = nativeFunctionCall(callable, args);
        delete[] args;
        return result;
    }
    else
    {
        plog::fatal << "Об'єкт неможливо викликати";
    }
}

Object* vm::nativeFunctionCall(Object* callable, Object* args[])
{
    if (callable->objectType->type == ObjectTypes::NATIVE_FUNCTION)
    {
        auto nativeFunction = (NativeFunctionObject*)callable;
        auto result = nativeFunction->function(args);
        return result;
    }
    else
    {
        plog::fatal << "Об'єкт не є нативною функцією!";
    }
}
