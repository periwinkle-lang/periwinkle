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
#include "argument_parser.h"

#define BUILTIN_FUNCTION(name, arity, isVariadic, func, defaults) \
    {name, NativeFunctionObject::create(arity, isVariadic, name, func, defaults)}

#define BUILTIN_TYPE(type) \
    {type.name, &type}

using namespace vm;

static DefaultParameters readLineDefaults = { {"підказка"}, {&P_emptyStr} };

static std::u32string joinObjectString(
    const std::u32string& sep, const std::vector<Object*>& objects)
{
    if (objects.size())
    {
        auto& str = ((StringObject*)Object::toString(objects[0]))->value;
        std::u32string result = std::accumulate(
            ++objects.begin(), objects.end(), str,
            [sep](const std::u32string& a, Object* o)
            {
                auto& str = ((StringObject*)Object::toString(o))->value;
                return a + sep + str;
            });
        return result;
    }
    return U"";
}

static Object* printNative(std::span<Object*> args, ArrayObject* va, NamedArgs* na)
{
    auto str = joinObjectString(U" ", va->items);
    std::cout << utils::utf32to8(str) << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args, ArrayObject* va, NamedArgs* na)
{
    auto str = joinObjectString(U" ", va->items);
    std::cout << utils::utf32to8(str) << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args, ArrayObject* va, NamedArgs* na)
{
    StringObject* a;
    static ArgParser argParser{
        {&a, stringObjectType, "а"},
    };
    argParser.parse(args, &readLineDefaults, na);
    std::cout << a->asUtf8();
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* getIterator(std::span<Object*> args, ArrayObject* va, NamedArgs* na)
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
            BUILTIN_FUNCTION("друк", 0, true, printNative, nullptr),
            BUILTIN_FUNCTION("друкр", 0, true, printLnNative, nullptr),
            BUILTIN_FUNCTION("зчитати", 0, false, readLineNative, &readLineDefaults),
            BUILTIN_FUNCTION("ітератор", 1, false, getIterator, nullptr),

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
