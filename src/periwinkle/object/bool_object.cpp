#include "bool_object.h"
#include "string_object.h"
#include "int_object.h"
#include "real_object.h"
#include "exception_object.h"
#include "native_method_object.h"

using namespace vm;

#define TO_BOOL(object, b)                             \
    if (object->objectType->type != ObjectTypes::BOOL) \
        return &P_NotImplemented;                      \
    b = object == &P_true;

static Object* boolComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    bool a, b;
    TO_BOOL(o1, a);
    TO_BOOL(o2, b);
    bool result;

    using enum ObjectCompOperator;
    switch (op)
    {
    case EQ: result = a == b; break;
    case NE: result = a != b; break;
    default:
        return &P_NotImplemented;
    }

    return P_BOOL(result);
}

static Object* boolInit(Object* o, std::span<Object*> args, ArrayObject* va)
{
    return Object::toBool(args[0]);
}

static Object* boolToString(Object* a)
{
    auto arg = (BoolObject*)a;
    return StringObject::create(arg->value ? U"істина" : U"хиба");
}

static Object* boolToInteger(Object* a)
{
    return IntObject::create(a == &P_true ? 1 : 0);
}

static Object* boolToReal(Object* a)
{
    return RealObject::create(a == &P_true ? 1.0 : 0.0);
}

static Object* boolToBool(Object* a)
{
    return a;
}

namespace vm
{
    TypeObject boolObjectType =
    {
        .base = &objectObjectType,
        .name = "Логічний",
        .type = ObjectTypes::BOOL,
        .alloc = DEFAULT_ALLOC(BoolObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, boolInit, boolObjectType),
        .operators =
        {
            .toString = boolToString,
            .toInteger = boolToInteger,
            .toReal = boolToReal,
            .toBool = boolToBool,
        },
        .comparison = boolComparison,
    };

    BoolObject P_true  { {.objectType = &boolObjectType}, true  };
    BoolObject P_false { {.objectType = &boolObjectType}, false };
}
