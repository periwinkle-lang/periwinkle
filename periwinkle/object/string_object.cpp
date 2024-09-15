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

#define CHECK_STRING(object)                           \
    if (OBJECT_IS(object, &stringObjectType) == false) \
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
            &IndexErrorObjectType, "Індекс виходить за межі рядка"); \
        return nullptr;                                                \
    }

static Object* strInit(Object* o, std::span<Object*> args, TupleObject* va, NamedArgs* na)
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
    case EQ: result = a == b; break;
    case NE: result = a != b; break;
    case GT: result = a > b; break;
    case GE: result = a >= b; break;
    case LT: result = a < b; break;
    case LE: result = a <= b; break;
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
                "Неможливо перетворити рядок \"{}\" в дійсне число",
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

#define X_OBJECT_STRUCT StringObject
#define X_OBJECT_TYPE vm::stringObjectType

METHOD_TEMPLATE(removeEnd)
{
    OBJECT_CAST();
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
OBJECT_METHOD(removeEnd, "видалитиЗакінчення", 1, false, nullptr)


METHOD_TEMPLATE(removePrefix)
{
    OBJECT_CAST();
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
OBJECT_METHOD(removePrefix, "видалитиПрефікс", 1, false, nullptr)


METHOD_TEMPLATE(strInsert)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strInsert, "вставити", 2, false, nullptr)


METHOD_TEMPLATE(strSet)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strSet, "встановити", 2, false, nullptr)


METHOD_TEMPLATE(strSize)
{
    OBJECT_CAST();
    return IntObject::create(o->value.size());
}
OBJECT_METHOD(strSize, "розмір", 0, false, nullptr)


METHOD_TEMPLATE(strEndsWith)
{
    OBJECT_CAST();
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    return P_BOOL(value->value.size() && o->value.ends_with(value->value));
}
OBJECT_METHOD(strEndsWith, "закінчуєтьсяНа", 1, false, nullptr)


METHOD_TEMPLATE(strReplace)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strReplace, "замінити", 2, false, nullptr)


Object* strJoin(std::span<Object*> args, TupleObject* va, NamedArgs* na)
{
    StringObject* separator;
    ListObject* objects;
    ArgParser argParser{
        {&separator, stringObjectType, "роздільник"},
        {&objects, listObjectType, "об'єкти"},
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
OBJECT_STATIC_METHOD(strJoin, "зліпити", 2, false, nullptr)


METHOD_TEMPLATE(strFind)
{
    OBJECT_CAST();
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto pos = o->value.find(value->value);
    return IntObject::create(pos == std::string::npos ? -1 : pos);
}
OBJECT_METHOD(strFind, "знайти", 1, false, nullptr)


METHOD_TEMPLATE(strCount)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strCount, "кількість", 1, false, nullptr)


METHOD_TEMPLATE(strContains)
{
    OBJECT_CAST();
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    auto pos = o->value.find(value->value);
    return P_BOOL(pos != std::string::npos);
}
OBJECT_METHOD(strContains, "містить", 1, false, nullptr)


METHOD_TEMPLATE(strGet)
{
    OBJECT_CAST();
    IntObject* index;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    return StringObject::create(std::u32string{ o->value[index->value] });
}
OBJECT_METHOD(strGet, "отримати", 1, false, nullptr)


METHOD_TEMPLATE(strStartsWith)
{
    OBJECT_CAST();
    StringObject* value;
    ArgParser argParser{
        {&value, stringObjectType, "значення"},
    };
    if (!argParser.parse(args)) return nullptr;

    return P_BOOL(o->value.size() && o->value.starts_with(value->value));
}
OBJECT_METHOD(strStartsWith, "починаєтьсяНа", 1, false, nullptr)


METHOD_TEMPLATE(strTrim)
{
    OBJECT_CAST();
    auto start = o->value.begin();
    auto end = o->value.end();

    while (start != end && unicode::isSpace(*start))
    {
        start++;
    }

    while (end != start && unicode::isSpace(*(end - 1)))
    {
        end--;
    }

    return StringObject::create(std::u32string{ start, end });
}
OBJECT_METHOD(strTrim, "причепурити", 0, false, nullptr)


METHOD_TEMPLATE(strLeftTrim)
{
    OBJECT_CAST();
    auto start = o->value.begin();
    auto end = o->value.end();

    while (start != end && unicode::isSpace(*start))
    {
        start++;
    }

    return StringObject::create(std::u32string{ start, end });
}
OBJECT_METHOD(strLeftTrim, "причепуритиЗліва", 0, false, nullptr)


METHOD_TEMPLATE(strRightTrim)
{
    OBJECT_CAST();
    auto start = o->value.begin();
    auto end = o->value.end();

    while (end != start && unicode::isSpace(*(end - 1)))
    {
        end--;
    }

    return StringObject::create(std::u32string{ start, end });
}
OBJECT_METHOD(strRightTrim, "причепуритиСправа", 0, false, nullptr)


METHOD_TEMPLATE(strSubstr)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strSubstr, "підрядок", 2, false, nullptr)


METHOD_TEMPLATE(strSplit)
{
    OBJECT_CAST();
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
OBJECT_METHOD(strSplit, "розділити", 1, false, nullptr)


static IntObject int10 = IntObject{ {.objectType = &intObjectType}, 10 };
static DefaultParameters toIntDefaults = {{
    {"основа", &int10}} };

METHOD_TEMPLATE(toIntMethod)
{
    OBJECT_CAST();
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

    auto value = stringObjectToInt(o, static_cast<int>(base->value));
    if (!value) return nullptr;
    return IntObject::create(value.value());
}
OBJECT_METHOD(toIntMethod, "доЧисла", 0, false, &toIntDefaults)


METHOD_TEMPLATE(strIsAlpha)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if (unicode::isLetter(ch) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsAlpha, "цеБуквенне", 0, false, nullptr)


METHOD_TEMPLATE(strIsAlnum)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if ((unicode::isLetter(ch) && unicode::isDigit(ch)) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsAlnum, "цеБуквенноЦифрове", 0, false, nullptr)


METHOD_TEMPLATE(strIsDecimal)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if (unicode::isDecimal(ch) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsDecimal, "цеДесятковоЦифрове", 0, false, nullptr)


METHOD_TEMPLATE(strIsDigit)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if (unicode::isDigit(ch) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsDigit, "цеЦифрове", 0, false, nullptr)


METHOD_TEMPLATE(strIsNumeric)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if (unicode::isNumeric(ch) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsNumeric, "цеЧислове", 0, false, nullptr)


METHOD_TEMPLATE(strIsSpace)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_false;
    }

    for (char32_t ch : o->value)
    {
        if (unicode::isSpace(ch) == false)
        {
            return &P_false;
        }
    }
    return &P_true;
}
OBJECT_METHOD(strIsSpace, "цеПробіл", 0, false, nullptr)


METHOD_TEMPLATE(strToLowercase)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_emptyStr;
    }

    auto newStr = StringObject::create(U"");
    newStr->value.reserve(o->value.size());
    for (char32_t ch : o->value)
    {
        newStr->value += unicode::toLowercase(ch);
    }

    return newStr;
}
OBJECT_METHOD(strToLowercase, "доНижнього", 0, false, nullptr)


METHOD_TEMPLATE(strToUppercase)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_emptyStr;
    }

    auto newStr = StringObject::create(U"");
    newStr->value.reserve(o->value.size());
    for (char32_t ch : o->value)
    {
        newStr->value += unicode::toUppercase(ch);
    }

    return newStr;
}
OBJECT_METHOD(strToUppercase, "доВерхнього", 0, false, nullptr)


METHOD_TEMPLATE(strTitle)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_emptyStr;
    }

    auto newStr = StringObject::create(U"");
    newStr->value.reserve(o->value.size());
    bool prevHasCase = false;
    for (char32_t ch : o->value)
    {
        if (unicode::hasCase(ch) == false)
        {
            newStr->value += ch;
            prevHasCase = false;
            continue;
        }

        if (prevHasCase == false)
        {
            if (char32_t titleCh; (titleCh = unicode::toTitlecase(ch)) != ch)
                newStr->value += titleCh;
            else
                newStr->value += unicode::toUppercase(ch);
            prevHasCase = true;
        }
        else
            newStr->value += unicode::toLowercase(ch);
    }

    return newStr;
}
OBJECT_METHOD(strTitle, "доЗаголовку", 0, false, nullptr)


METHOD_TEMPLATE(strCapitalize)
{
    OBJECT_CAST();
    if (o->value.size() == 0)
    {
        return &P_emptyStr;
    }

    auto newStr = StringObject::create(U"");
    newStr->value.reserve(o->value.size());
    for (char32_t ch : o->value)
    {
        newStr->value += unicode::toLowercase(ch);
    }

    if (char32_t titleCh; (titleCh = unicode::toTitlecase(o->value[0])) != o->value[0])
        newStr->value[0] = titleCh;
    else
        newStr->value[0] = unicode::toUppercase(o->value[0]);

    return newStr;
}
OBJECT_METHOD(strCapitalize, "буквиця", 0, false, nullptr)

#undef X_OBJECT_STRUCT
#undef X_OBJECT_TYPE
#define X_OBJECT_STRUCT StringIterObject
#define X_OBJECT_TYPE vm::stringIterObjectType

METHOD_TEMPLATE(strIterNext)
{
    OBJECT_CAST();
    if (o->position < o->length)
    {
        return StringObject::create(std::u32string{ o->iterable[o->position++] });
    }
    return &P_endIter;
}
OBJECT_METHOD(strIterNext, "наступний", 0, false, nullptr)


namespace vm
{
    TypeObject stringObjectType =
    {
        .base = &objectObjectType,
        .name = "Рядок",
        .size = sizeof(StringObject),
        .alloc = DEFAULT_ALLOC(StringObject),
        .dealloc = DEFAULT_DEALLOC(StringObject),
        .callableInfo =
        {
            .arity = 1,
            .name = "конструктор",
            .flags = CallableInfo::IS_METHOD,
        },
        .constructor = strInit,
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
            METHOD_ATTRIBUTE(removeEnd),
            METHOD_ATTRIBUTE(removePrefix),
            METHOD_ATTRIBUTE(strInsert),
            METHOD_ATTRIBUTE(strSet),
            METHOD_ATTRIBUTE(strSize),
            METHOD_ATTRIBUTE(strEndsWith),
            METHOD_ATTRIBUTE(strReplace),
            METHOD_ATTRIBUTE(strFind),
            METHOD_ATTRIBUTE(strCount),
            METHOD_ATTRIBUTE(strContains),
            METHOD_ATTRIBUTE(strGet),
            METHOD_ATTRIBUTE(strStartsWith),
            METHOD_ATTRIBUTE(strTrim),
            METHOD_ATTRIBUTE(strLeftTrim),
            METHOD_ATTRIBUTE(strRightTrim),
            METHOD_ATTRIBUTE(strSubstr),
            METHOD_ATTRIBUTE(strSplit),
            METHOD_ATTRIBUTE(toIntMethod),
            METHOD_ATTRIBUTE(strIsAlpha),
            METHOD_ATTRIBUTE(strIsAlnum),
            METHOD_ATTRIBUTE(strIsDecimal),
            METHOD_ATTRIBUTE(strIsDigit),
            METHOD_ATTRIBUTE(strIsNumeric),
            METHOD_ATTRIBUTE(strIsSpace),
            METHOD_ATTRIBUTE(strToLowercase),
            METHOD_ATTRIBUTE(strToUppercase),
            METHOD_ATTRIBUTE(strTitle),
            METHOD_ATTRIBUTE(strCapitalize),
            STATIC_METHOD_ATTRIBUTE(strJoin),
        },
    };

    TypeObject stringIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторРядка",
        .size = sizeof(StringIterObject),
        .alloc = DEFAULT_ALLOC(StringIterObject),
        .attributes =
        {
            METHOD_ATTRIBUTE(strIterNext),
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
                    "Неможливо перетворити рядок \"{}\" в число",
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
    return unicode::toUtf8(value);
}

StringObject* vm::StringObject::create(std::string_view value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = unicode::toUtf32(value);
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
