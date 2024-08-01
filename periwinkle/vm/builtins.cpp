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

#define BUILTIN_OBJECT(name, object) \
    {name, &object}

using namespace vm;

static StringObject strWithSpace = { {.objectType = &stringObjectType}, U" " };

static DefaultParameters readLineDefaults = { {"підказка"}, {&P_emptyStr} };
static DefaultParameters printDefaults = { {"роздільник"}, {&strWithSpace} };

static std::u32string joinObjectString(
    const std::u32string& sep, const std::vector<Object*>& objects)
{
    if (objects.size())
    {
        auto& str = ((StringObject*)objects[0]->toString())->value;
        std::u32string result = std::accumulate(
            ++objects.begin(), objects.end(), str,
            [sep](const std::u32string& a, Object* o)
            {
                auto& str = ((StringObject*)o->toString())->value;
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
    if (!argParser.parse(args, &printDefaults, na)) return nullptr;
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
    if (!argParser.parse(args, &printDefaults, na)) return nullptr;
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
    if (!argParser.parse(args, &readLineDefaults, na)) return nullptr;
    std::cout << prompt->asUtf8();
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* getIterator(std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    return args[0]->getIter();
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

            BUILTIN_TYPE(objectObjectType),
            BUILTIN_TYPE(intObjectType),
            BUILTIN_TYPE(boolObjectType),
            BUILTIN_TYPE(stringObjectType),
            BUILTIN_TYPE(realObjectType),
            BUILTIN_TYPE(listObjectType),

            BUILTIN_TYPE(ExceptionObjectType),
            BUILTIN_TYPE(NameErrorObjectType),
            BUILTIN_TYPE(TypeErrorObjectType),
            BUILTIN_TYPE(NotImplementedErrorObjectType),
            BUILTIN_TYPE(AttributeErrorObjectType),
            BUILTIN_TYPE(IndexErrorObjectType),
            BUILTIN_TYPE(DivisionByZeroErrorObjectType),
            BUILTIN_TYPE(ValueErrorObjectType),
            BUILTIN_TYPE(InternalErrorObjectType),

            BUILTIN_OBJECT("КінецьІтерації", P_endIter),
        }
        );
    }
    return builtin;
}
