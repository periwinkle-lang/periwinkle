#include <span>

#include "builtins.h"
#include "null_object.h"
#include "string_object.h"
#include "utils.h"

using namespace vm;

static Object* printNative(std::span<Object*> args)
{
    auto result = (StringObject*)Object::toString(args[0]);
    std::cout << result->value << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args)
{
    auto result = (StringObject*)Object::toString(args[0]);
    std::cout << result->value << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args)
{
    std::string line = utils::readline();
    return StringObject::create(line);
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
                {"зчитати", NativeFunctionObject::create(0, "зчитати", readLineNative)},
            }
        );
    }
    return builtin;
}
