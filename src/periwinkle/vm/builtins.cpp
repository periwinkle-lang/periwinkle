#include <span>

#include "builtins.h"
#include "null_object.h"
#include "string_object.h"
#include "utils.h"
#include "array_object.h"

using namespace vm;

static Object* printNative(std::span<Object*> args, ArrayObject* va)
{
    auto result = (StringObject*)Object::toString(args[0]);
    std::cout << result->value << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args, ArrayObject* va)
{
    auto result = (StringObject*)Object::toString(args[0]);
    std::cout << result->value << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args, ArrayObject* va)
{
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* createArray(std::span<Object*> args, ArrayObject* va)
{
    return ArrayObject::create();
}

builtin_t* vm::getBuiltin()
{
    static builtin_t* builtin;
    if (builtin == nullptr)
    {
        builtin = new builtin_t();
        builtin->insert(
        {
            {"друк", NativeFunctionObject::create(1, false, "друк", printNative)},
            {"друклн", NativeFunctionObject::create(1, false, "друклн", printLnNative)},
            {"зчитати", NativeFunctionObject::create(0, false, "зчитати", readLineNative)},
            {"Масив", NativeFunctionObject::create(0, false, "Масив", createArray)},
        }
        );
    }
    return builtin;
}
