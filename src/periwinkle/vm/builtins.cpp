#include "builtins.h"
#include "null_object.h"
#include "call.h"
#include "string_object.h"

using namespace vm;

Object* printNative(Object* args[])
{
    StringObject* result;
    if (args[0]->objectType->type != ObjectTypes::STRING)
    {
        result = (StringObject*)GET_OPERATOR(args[0], toString)(args[0]);
    }
    else
    {
        result = (StringObject*)args[0];
    }
    std::cout << result->value << std::flush;
    return &nullObject;
}


builtin_t* vm::getBuiltin()
{
    static builtin_t* builtin;
    if (builtin == nullptr)
    {
        builtin = new builtin_t();
        builtin->insert(
            {
                {"друк", NativeFunctionObject::create(1, "друк", printNative)},
            }
        );
    }
    return builtin;
}
