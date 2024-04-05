#include <span>
#include <numeric>

#include "builtins.hpp"
#include "null_object.hpp"
#include "string_object.hpp"
#include "utils.hpp"
#include "list_object.hpp"
#include "native_function_object.hpp"
#include "int_object.hpp"
#include "bool_object.hpp"
#include "real_object.hpp"
#include "end_iteration_object.hpp"
#include "argument_parser.hpp"

#define BUILTIN_FUNCTION(name, arity, isVariadic, func, defaults) \
    {name, NativeFunctionObject::create(arity, isVariadic, name, func, defaults)}

#define BUILTIN_TYPE(type) \
    {type.name, &type}

using namespace vm;

static StringObject strWithSpace = { {.objectType = &stringObjectType}, U" " };

static DefaultParameters readLineDefaults = { {"підказка"}, {&P_emptyStr} };
static DefaultParameters printDefaults = { {"роздільник"}, {&strWithSpace} };

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

static Object* printNative(std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    StringObject* separator;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"}
    };
    argParser.parse(args, &printDefaults, na);
    auto str = joinObjectString(separator->value, va->items);
    std::cout << utils::utf32to8(str) << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    StringObject* separator;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"}
    };
    argParser.parse(args, &printDefaults, na);
    auto str = joinObjectString(separator->value, va->items);
    std::cout << utils::utf32to8(str) << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    StringObject* prompt;
    ArgParser argParser{
        {&prompt, stringObjectType, "підказка"},
    };
    argParser.parse(args, &readLineDefaults, na);
    std::cout << prompt->asUtf8();
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* getIterator(std::span<Object*> args, ListObject* va, NamedArgs* na)
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
            BUILTIN_FUNCTION("друк", 0, true, printNative, &printDefaults),
            BUILTIN_FUNCTION("друкр", 0, true, printLnNative, &printDefaults),
            BUILTIN_FUNCTION("зчитати", 0, false, readLineNative, &readLineDefaults),
            BUILTIN_FUNCTION("ітератор", 1, false, getIterator, nullptr),

            BUILTIN_TYPE(intObjectType),
            BUILTIN_TYPE(boolObjectType),
            BUILTIN_TYPE(stringObjectType),
            BUILTIN_TYPE(realObjectType),
            BUILTIN_TYPE(listObjectType),

            {"КінецьІтерації", &P_endIter},
        }
        );
    }
    return builtin;
}
