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
        result = (StringObject*)nativeFunctionCall(
            args[0]->objectType->operators->toString, args);
    }
    else
    {
        result = (StringObject*)args[0];
    }
    std::cout << result->value << std::flush;
    return &nullObject;
}

namespace vm
{
    std::unordered_map<std::string, NativeFunctionObject*> builtin
    {
        {"друк", NativeFunctionObject::create(1, "друк", printNative)},
    };
}
