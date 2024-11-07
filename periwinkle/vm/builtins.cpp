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
#include "tuple_object.hpp"
#include "argument_parser.hpp"
#include "unicode.hpp"
#include "platform.hpp"

#define BUILTIN_FUNCTION_IMPLEMENTATION(func, name, arity, variadic, defaults) \
    static const char* func##__functionName = name;                            \
    static NativeFunctionObject func##__functionImpl {                         \
        func##__functionName, func, arity, variadic, defaults                  \
    };

#define BUILTIN_FUNCTION_TEMPLATE(name) \
    static Object* name(std::span<Object*> args, TupleObject* va, NamedArgs* na)

#define BUILTIN_FUNCTION(func) { func##__functionName, &func##__functionImpl }

#define BUILTIN_TYPE(type) \
    {type.name, &type}

#define BUILTIN_OBJECT(name, object) \
    {name, &object}

using namespace vm;

static StringObject strWithSpace = { {.objectType = &stringObjectType}, U" " };

static DefaultParameters readLineDefaults = {{ {"підказка", &P_emptyStr} }};
static DefaultParameters printDefaults = {{ {"роздільник", &strWithSpace} }};

static std::u32string joinObjectString(
    const std::u32string& sep, std::span<Object*> objects)
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

BUILTIN_FUNCTION_TEMPLATE(printNative)
{
    StringObject* separator;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"}
    };
    if (!argParser.parse(args, &printDefaults, na)) return nullptr;
    auto str = joinObjectString(separator->value, va->items);
    std::cout << unicode::toUtf8(str) << std::flush;
    return &P_null;
}
BUILTIN_FUNCTION_IMPLEMENTATION(printNative, "друк", 0, true, &printDefaults)


BUILTIN_FUNCTION_TEMPLATE(printLnNative)
{
    StringObject* separator;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"}
    };
    if (!argParser.parse(args, &printDefaults, na)) return nullptr;
    auto str = joinObjectString(separator->value, va->items);
    std::cout << unicode::toUtf8(str) << std::endl;
    return &P_null;
}
BUILTIN_FUNCTION_IMPLEMENTATION(printLnNative, "друкр", 0, true, &printDefaults)


BUILTIN_FUNCTION_TEMPLATE(readLineNative)
{
    StringObject* prompt;
    ArgParser argParser{
        {&prompt, stringObjectType, "підказка"},
    };
    if (!argParser.parse(args, &readLineDefaults, na)) return nullptr;
    std::cout << prompt->asUtf8();
    std::string line = platform::readline();
    return StringObject::create(line);
}
BUILTIN_FUNCTION_IMPLEMENTATION(readLineNative, "зчитати", 0, false, &readLineDefaults)


BUILTIN_FUNCTION_TEMPLATE(getIteratorNative)
{
    return args[0]->getIter();
}
BUILTIN_FUNCTION_IMPLEMENTATION(getIteratorNative, "ітератор", 1, false, nullptr)


static builtin_t builtin;

void vm::initBuiltins()
{
    if (builtin.empty())
    {
        builtin.insert({
            BUILTIN_FUNCTION(printNative),
            BUILTIN_FUNCTION(printLnNative),
            BUILTIN_FUNCTION(readLineNative),
            BUILTIN_FUNCTION(getIteratorNative),

            BUILTIN_TYPE(objectObjectType),
            BUILTIN_TYPE(intObjectType),
            BUILTIN_TYPE(boolObjectType),
            BUILTIN_TYPE(stringObjectType),
            BUILTIN_TYPE(realObjectType),
            BUILTIN_TYPE(listObjectType),
            BUILTIN_TYPE(tupleObjectType),

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
        });
    }
}

void vm::deinitBuiltins()
{
    builtin.clear();
}

builtin_t* vm::getBuiltin()
{
    return &builtin;
}
