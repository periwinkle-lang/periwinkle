#include <span>
#include <numeric>

#include "builtins.h"
#include "null_object.h"
#include "string_object.h"
#include "utils.h"
#include "array_object.h"
#include "native_function_object.h"
#include "int_object.h"
#include "bool_object.h"
#include "real_object.h"
#include "end_iteration_object.h"

#define BUILTIN_FUNCTION(name, arity, isVariadic, func) \
    {name, NativeFunctionObject::create(arity, isVariadic, name, func)}

#define BUILTIN_TYPE(type) \
    {type.name, &type}

using namespace vm;

static std::string joinObjectString(
    const std::string& sep, const std::vector<Object*>& objects)
{
    if (objects.size())
    {
        auto& str = ((StringObject*)Object::toString(objects[0]))->value;
        std::string result = std::accumulate(
            ++objects.begin(), objects.end(), str,
            [sep](const std::string& a, Object* o)
            {
                auto& str = ((StringObject*)Object::toString(o))->value;
                return a + sep + str;
            });
        return result;
    }
    return "";
}

static Object* printNative(std::span<Object*> args, ArrayObject* va)
{
    auto str = joinObjectString(" ", va->items);
    std::cout << str << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args, ArrayObject* va)
{
    auto str = joinObjectString(" ", va->items);
    std::cout << str << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args, ArrayObject* va)
{
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* getIterator(std::span<Object*> args, ArrayObject* va)
{
    return Object::getIter(args[0]);
}

builtin_t* vm::getBuiltin()
{
    static builtin_t* builtin;
    if (builtin == nullptr)
    {
        builtin = new builtin_t();
        builtin->insert(
        {
            BUILTIN_FUNCTION("друк", 0, true, printNative),
            BUILTIN_FUNCTION("друкр", 0, true, printLnNative),
            BUILTIN_FUNCTION("зчитати", 0, false, readLineNative),
            BUILTIN_FUNCTION("ітератор", 1, false, getIterator),

            BUILTIN_TYPE(intObjectType),
            BUILTIN_TYPE(boolObjectType),
            BUILTIN_TYPE(stringObjectType),
            BUILTIN_TYPE(realObjectType),
            BUILTIN_TYPE(arrayObjectType),

            {"КінецьІтерації", &P_endIter},
        }
        );
    }
    return builtin;
}
