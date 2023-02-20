#include <span>

#include "builtins.h"
#include "null_object.h"
#include "string_object.h"

using namespace vm;

static Object* printNative(std::span<Object*> args)
{
    StringObject* result;
    if (args[0]->objectType->type != ObjectTypes::STRING)
    {
        result = (StringObject*)Object::toString(args[0]);
    }
    else
    {
        result = (StringObject*)args[0];
    }
    std::cout << result->value << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args)
{
    StringObject* result;
    if (args[0]->objectType->type != ObjectTypes::STRING)
    {
        result = (StringObject*)Object::toString(args[0]);
    }
    else
    {
        result = (StringObject*)args[0];
    }
    std::cout << result->value << std::endl;
    return &P_null;
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
                {"друклн", NativeFunctionObject::create(1, "друклн", printLnNative)},
            }
        );
    }
    return builtin;
}
