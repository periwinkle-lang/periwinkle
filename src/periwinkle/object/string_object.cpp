#include "string_object.h"
#include "exception_object.h"
#include "null_object.h"
#include "bool_object.h"
#include "native_method_object.h"
#include "int_object.h"
#include "end_iteration_object.h"
#include "utils.h"

using namespace vm;

static bool tryConvertToString(Object * o, std::string& str)
{
    if (o->objectType->operators.toString != NULL)
    {
        str = ((StringObject*)Object::toString(o))->value;
        return true;
    }
    return false;
}

#define CHECK_STRING(object)                             \
    if (object->objectType->type != ObjectTypes::STRING) \
        return &P_NotImplemented;

// Конвертує об'єкт до StringObject, окрім випадку, коли об'єкт типу "нич"
#define TO_STRING(object, str)                           \
    if (object->objectType->type == ObjectTypes::STRING) \
        str = ((StringObject*)object)->value;            \
    else                                                 \
    {                                                    \
        if (object == &P_null                            \
            || tryConvertToString(object, str) == false) \
            return &P_NotImplemented;                    \
    }

static Object* strInit(Object* o, std::span<Object*> args, ArrayObject* va)
{
    return Object::toString(args[0]);
}

static Object* strComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    std::string a, b;
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

static Object* strAdd(Object* o1, Object* o2)
{
    std::string a, b;
    TO_STRING(o1, a);
    TO_STRING(o2, b);
    return StringObject::create(a + b);
}

static Object* strGetIter(StringObject* o)
{
    auto iterator = StringIterObject::create(o->value);
    return iterator;
}

static Object* stringSize(Object* s, std::span<Object*> args, ArrayObject* va)
{
    auto strObject = (StringObject*)s;
    return IntObject::create(utils::utf8Size(strObject->value));
}

static Object* strIterNext(StringIterObject* s, std::span<Object*> args, ArrayObject* va)
{
    if (s->position < s->length)
    {
        return StringObject::create(utils::utf8At(s->iterable, s->position++));
    }
    return &P_endIter;
}

namespace vm
{
    TypeObject stringObjectType =
    {
        .base = &objectObjectType,
        .name = "Стрічка",
        .type = ObjectTypes::STRING,
        .alloc = DEFAULT_ALLOC(StringObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, strInit),
        .operators =
        {
            .toString = strToString,
            .add = strAdd,
            .getIter = (unaryFunction)strGetIter,
        },
        .comparison = strComparison,
        .attributes =
        {
            OBJECT_METHOD("довжина", 0, false, stringSize),
        },
    };

    TypeObject stringIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторСтрічки",
        .type = ObjectTypes::STRING_ITERATOR,
        .alloc = DEFAULT_ALLOC(StringIterObject),
        .attributes =
        {
            OBJECT_METHOD("наступний", 0, false, (nativeMethod)strIterNext),
        },
    };
}

StringObject* vm::StringObject::create(std::string value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = value;
    return stringObject;
}

StringIterObject* vm::StringIterObject::create(const std::string& iterable)
{
    auto strIterObject = (StringIterObject*)allocObject(&stringIterObjectType);
    strIterObject->iterable = iterable;
    strIterObject->length = utils::utf8Size(iterable);
    return strIterObject;
}
