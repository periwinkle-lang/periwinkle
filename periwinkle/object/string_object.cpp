#include <numeric>
#include <format>
#include <utility>

#include "object.hpp"
#include "string_object.hpp"
#include "exception_object.hpp"
#include "null_object.hpp"
#include "bool_object.hpp"
#include "native_method_object.hpp"
#include "int_object.hpp"
#include "end_iteration_object.hpp"
#include "utils.hpp"
#include "argument_parser.hpp"
#include "native_function_object.hpp"
#include "real_object.hpp"
#include "periwinkle.hpp"
#include "unicode.hpp"

using namespace vm;

static bool tryConvertToString(Object* o, std::u32string& str)
{
    if (o->objectType->operators.toString != NULL)
    {
        str = ((StringObject*)o->toString())->value;
        return true;
    }
    return false;
}

#define CHECK_STRING(object)                  \
    if (OBJECT_IS(object, &stringObjectType)) \
        return &P_NotImplemented;

// Конвертує об'єкт до StringObject, окрім випадку, коли об'єкт типу "ніц"
#define TO_STRING(object, str)                           \
    if (OBJECT_IS(object, &stringObjectType))            \
        str = ((StringObject*)object)->value;            \
    else                                                 \
    {                                                    \
        if (object == &P_null                            \
            || tryConvertToString(object, str) == false) \
            return &P_NotImplemented;                    \
    }

#define CHECK_INDEX(index, strObject)                                  \
    if (std::cmp_greater_equal(index, strObject->value.size())         \
        || index < 0)                                                  \
    {                                                                  \
        getCurrentState()->setException(                               \
            &IndexErrorObjectType, "Індекс виходить за межі стрічки"); \
        return nullptr;                                                \
    }

static Object* strInit(Object* o, std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    return args[0]->toString();
}

static Object* strComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    std::u32string a, b;
    CHECK_STRING(o1);
    CHECK_STRING(o2);
    a = ((StringObject*)o1)->value;
    b = ((StringObject*)o2)->value;
    bool result;

    using enum ObjectCompOperator;
    switch (op)
    {
    case EQ: result = a.compare(b) == 0; break;
    case NE: result = a.compare(b) != 0; break;
    default:
        return &P_NotImplemented;
    }

    return P_BOOL(result);
}

static Object* strToString(Object* o)
{
    return o;
}

static Object* strToInteger(StringObject* o)
{
    if (auto value = stringObjectToInt(o))
    {
        return IntObject::create(value.value());
    }
    return nullptr;
}

static Object* strToReal(StringObject* o)
{
    double value;
    try
    {
        value = std::stod(o->asUtf8());
    }
    catch (const std::invalid_argument& e)
    {
        getCurrentState()->setException(
            &ValueErrorObjectType, std::format(
                "Неможливо перетворити стрічку \"{}\" в дійсне число",
                utils::escapeString(o->asUtf8())));
        return nullptr;
    }
    catch (const std::out_of_range& e)
    {
        getCurrentState()->setException(
            &ValueErrorObjectType, std::format(
                "Число \"{}\" не входить в діапазон можливих значень дійсного числа",
                o->asUtf8()));
        return nullptr;
    }
    return RealObject::create(value);
}

static Object* strToBool(StringObject* o)
{
    return P_BOOL(o->value.size());
}

static Object* strAdd(Object* o1, Object* o2)
{
    std::u32string a, b;
    TO_STRING(o1, a);
    TO_STRING(o2, b);
    return StringObject::create(a + b);
}

static Object* strGetIter(StringObject* o)
{
    auto iterator = StringIterObject::create(o->value);
    return iterator;
}

METHOD_TEMPLATE(removeEnd, StringObject)
{
    StringObject* end;
    ArgParser argParser{
        {&end, stringObjectType, "закінчення"}
    };
    if (!argParser.parse(args)) return nullptr;

    auto strSize = o->value.size(), endSize = end->value.size();
    if (endSize && o->value.ends_with(end->value))
    {
        return StringObject::create(o->value.substr(0, strSize - endSize));
    }

    return StringObject::create(o->value);
}

METHOD_TEMPLATE(removePrefix, StringObject)
{
    StringObject* prefix;
    ArgParser argParser{
        {&prefix, stringObjectType, "префікс"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto prefixSize = prefix->value.size();
    if (prefixSize && o->value.starts_with(prefix->value))
    {
        return StringObject::create(o->value.substr(prefixSize));
    }

    return StringObject::create(o->value);
}

METHOD_TEMPLATE(strInsert, StringObject)
{
    IntObject* index;
    StringObject* str;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&str, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, str);
    auto newStr = o->value;
    return StringObject::create(newStr.insert(index->value, str->value));
}

METHOD_TEMPLATE(strSet, StringObject)
{
    IntObject* index;
    StringObject* str;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&str, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, str);
    auto newStr = o->value;
    return StringObject::create(newStr.replace(
        index->value, 1, str->value));
}

METHOD_TEMPLATE(strSize, StringObject)
{
    return IntObject::create(o->value.size());
}

METHOD_TEMPLATE(strEndsWith, StringObject)
{
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    return P_BOOL(value->value.size() && o->value.ends_with(value->value));
}

METHOD_TEMPLATE(strReplace, StringObject)
{
    StringObject *what, *with;
    ArgParser argParser{
        {&what, stringObjectType, "що"},
        {&with, stringObjectType, "чим"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto newStr = o->value;
    auto whatSize = what->value.size(), withSize = with->value.size();
    size_t pos = 0;

    while ((pos = newStr.find(what->value, pos)) != std::string::npos)
    {
        newStr.replace(pos, whatSize, with->value);
        pos += withSize;
    }

    return StringObject::create(newStr);
}

Object* strJoin(std::span<Object*> args, ListObject* va)
{
    StringObject* separator;
    ListObject* objects;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"},
        {&objects, listObjectType, "обєкти"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto& items = objects->items;

    if (items.size())
    {
        auto& str = ((StringObject*)objects->items[0]->toString())->value;
        std::u32string result = std::accumulate(
            ++items.begin(), items.end(), str,
            [separator](const std::u32string& a, Object* o)
            {
                auto& str = ((StringObject*)o->toString())->value;
                return a + separator->value + str;
            });
        return StringObject::create(result);
    }

    return StringObject::create(U"");
}

METHOD_TEMPLATE(strFind, StringObject)
{
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto pos = o->value.find(value->value);
    return IntObject::create(pos == std::string::npos ? -1 : pos);
}

METHOD_TEMPLATE(strCopy, StringObject)
{
    return StringObject::create(o->value);
}

METHOD_TEMPLATE(strCount, StringObject)
{
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    size_t pos = 0, count = 0;

    while ((pos = o->value.find(value->value, pos)) != std::string::npos)
    {
        pos += value->value.size();
        ++count;
    }

    return IntObject::create(count);
}

METHOD_TEMPLATE(strContains, StringObject)
{
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto pos = o->value.find(value->value);
    return P_BOOL(pos != std::string::npos);
}

METHOD_TEMPLATE(strGet, StringObject)
{
    IntObject* index;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    return StringObject::create(std::u32string{ o->value[index->value] });
}

METHOD_TEMPLATE(strStartsWith, StringObject)
{
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    return P_BOOL(o->value.size() && o->value.starts_with(value->value));
}

METHOD_TEMPLATE(strTrim, StringObject)
{
    auto start = o->value.begin();
    auto end = o->value.end();

    while (start != end && *start >= 0 && *start <= 255 && std::isspace(*start))
    {
        start++;
    }

    while (end != start && *start >= 0 && *start <= 255 && std::isspace(*(end - 1)))
    {
        end--;
    }

    return StringObject::create(std::u32string{ start, end });
}

METHOD_TEMPLATE(strLeftTrim, StringObject)
{
    auto start = o->value.begin();
    auto end = o->value.end();

    while (start != end && *start >= 0 && *start <= 255 && std::isspace(*start))
    {
        start++;
    }

    return StringObject::create(std::u32string{ start, end });
}

METHOD_TEMPLATE(strRightTrim, StringObject)
{
    auto start = o->value.begin();
    auto end = o->value.end();

    while (end != start && *start >= 0 && *start <= 255 && std::isspace(*(end - 1)))
    {
        end--;
    }

    return StringObject::create(std::u32string{ start, end });
}

METHOD_TEMPLATE(strSubstr, StringObject)
{
    IntObject *start, *count;
    ArgParser argParser{
        {&start, intObjectType, "початок"},
        {&count, intObjectType, "кількість"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(start->value, o);
    CHECK_INDEX(start->value + count->value - (count->value == 0 ? 0 : 1), o);
    return StringObject::create(o->value.substr(start->value, count->value));
}

METHOD_TEMPLATE(strSplit, StringObject)
{
    StringObject* delimiter;
    ArgParser argParser{
        {&delimiter, stringObjectType, "роздільник"},
    };
    if (!argParser.parse(args)) return nullptr;

   auto strs = ListObject::create();
    size_t start = 0, end = o->value.find(delimiter->value);

    while (end != std::string::npos) {
        auto s = o->value.substr(start, end - start);
        strs->items.push_back(StringObject::create(s));
        start = end + delimiter->value.size();
        end = o->value.find(delimiter->value, start);
    }

    strs->items.push_back(StringObject::create(o->value.substr(start)));

    return strs;
}

static DefaultParameters toIntDefaults = {
    {"основа"}, { new IntObject{ {.objectType = &intObjectType}, 10 } } };

METHOD_TEMPLATE(toIntMethod, StringObject)
{
    IntObject* base;
    ArgParser argParser{
        {&base, intObjectType, "основа"}
    };
    if (!argParser.parse(args, &toIntDefaults, na)) return nullptr;

    if (!((base->value >= 2 && base->value <= 36) || base->value == 0))
    {
        getCurrentState()->setException(
            &ValueErrorObjectType, "Основа повинна бути в діапазоні 2-36(включно) або 0");
        return nullptr;
    }

    auto value = stringObjectToInt(o, base->value);
    if (!value) return nullptr;
    return IntObject::create(value.value());
}

METHOD_TEMPLATE(strIterNext, StringIterObject)
{
    if (o->position < o->length)
    {
        return StringObject::create(std::u32string{ o->iterable[o->position++] });
    }
    return &P_endIter;
}

namespace vm
{
    TypeObject stringObjectType =
    {
        .base = &objectObjectType,
        .name = "Стрічка",
        .size = sizeof(StringObject),
        .alloc = DEFAULT_ALLOC(StringObject),
        .dealloc = DEFAULT_DEALLOC(StringObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, strInit, stringObjectType, nullptr),
        .operators =
        {
            .toString = strToString,
            .toInteger = (unaryFunction)strToInteger,
            .toReal = (unaryFunction)strToReal,
            .toBool = (unaryFunction)strToBool,
            .add = strAdd,
            .getIter = (unaryFunction)strGetIter,
        },
        .comparison = strComparison,
        .attributes =
        {
            OBJECT_METHOD("видалитиЗакінчення", 1, false, removeEnd,     stringObjectType, nullptr),
            OBJECT_METHOD("видалитиПрефікс",    1, false, removePrefix,  stringObjectType, nullptr),
            OBJECT_METHOD("вставити",           2, false, strInsert,     stringObjectType, nullptr),
            OBJECT_METHOD("встановити",         2, false, strSet,        stringObjectType, nullptr),
            OBJECT_METHOD("довжина",            0, false, strSize,       stringObjectType, nullptr),
            OBJECT_METHOD("закінчуєтьсяНа",     1, false, strEndsWith,   stringObjectType, nullptr),
            OBJECT_METHOD("замінити",           2, false, strReplace,    stringObjectType, nullptr),
            OBJECT_METHOD("знайти",             1, false, strFind,       stringObjectType, nullptr),
            OBJECT_METHOD("копія",              0, false, strCopy,       stringObjectType, nullptr),
            OBJECT_METHOD("кількість",          1, false, strCount,      stringObjectType, nullptr),
            OBJECT_METHOD("містить",            1, false, strContains,   stringObjectType, nullptr),
            OBJECT_METHOD("отримати",           1, false, strGet,        stringObjectType, nullptr),
            OBJECT_METHOD("починаєтьсяНа",      1, false, strStartsWith, stringObjectType, nullptr),
            OBJECT_METHOD("причепурити",        0, false, strTrim,       stringObjectType, nullptr),
            OBJECT_METHOD("причепуритиЗліва",   0, false, strLeftTrim,   stringObjectType, nullptr),
            OBJECT_METHOD("причепуритиСправа",  0, false, strRightTrim,  stringObjectType, nullptr),
            OBJECT_METHOD("підстрічка",         2, false, strSubstr,     stringObjectType, nullptr),
            OBJECT_METHOD("розділити",          1, false, strSplit,      stringObjectType, nullptr),
            OBJECT_METHOD("доЧисла",            0, false, toIntMethod,   stringObjectType, &toIntDefaults),
            OBJECT_STATIC_METHOD("зліпити",     2, false, strJoin, nullptr),
        },
    };

    TypeObject stringIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторСтрічки",
        .size = sizeof(StringIterObject),
        .alloc = DEFAULT_ALLOC(StringIterObject),
        .attributes =
        {
            OBJECT_METHOD("наступний", 0, false, strIterNext, stringIterObjectType, nullptr),
        },
    };

    StringObject P_emptyStr = { {.objectType = &stringObjectType}, U"" };

    std::optional<i64> stringObjectToInt(StringObject* str, int base)
    {
        i64 value;
        try
        {
            value = std::stoll(str->asUtf8(), nullptr, base);
        }
        catch (const std::invalid_argument& e)
        {
            getCurrentState()->setException(
                &ValueErrorObjectType, std::format(
                    "Неможливо перетворити стрічку \"{}\" в число",
                    utils::escapeString(str->asUtf8())));
            return std::nullopt;
        }
        catch (const std::out_of_range& e)
        {
            getCurrentState()->setException(
                &ValueErrorObjectType, std::format(
                    "Число \"{}\" не входить в діапазон можливих значень числа",
                    str->asUtf8()));
            return std::nullopt;
        }
        return value;
    }
}

std::string vm::StringObject::asUtf8() const
{
    return unicode::utf32to8(value);
}

StringObject* vm::StringObject::create(const std::string& value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = unicode::utf8to32(value);
    return stringObject;
}

StringObject* vm::StringObject::create(const std::u32string& value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = value;
    return stringObject;
}

StringIterObject* vm::StringIterObject::create(const std::u32string& iterable)
{
    auto strIterObject = (StringIterObject*)allocObject(&stringIterObjectType);
    strIterObject->iterable = iterable;
    strIterObject->length = iterable.size();
    return strIterObject;
}
