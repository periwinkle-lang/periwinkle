#include <string>

#include "int_object.h"
#include "native_function_object.h"
#include "string_object.h"
#include "bool_object.h"
#include "real_object.h"
#include "exception_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define CHECK_INT(object)                                 \
    if (object->objectType->type != ObjectTypes::INTEGER) \
        return &P_NotImplemented;

#define TO_INT(object, i)            \
    CHECK_INT(object)                \
    i = ((IntObject*)object)->value;

#define BINARY_OP(name, op)                   \
static Object* name(Object* a, Object* b)     \
{                                             \
    CHECK_INT(a);                             \
    CHECK_INT(b);                             \
    auto arg1 = (IntObject*)a;                \
    auto arg2 = (IntObject*)b;                \
    auto result = arg1->value op arg2->value; \
    return IntObject::create(result);         \
}

static Object* intComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    i64 a, b;
    TO_INT(o1, a);
    TO_INT(o2, b);
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

static Object* intToString(Object* a)
{
    auto integer = (IntObject*)a;
    auto str = std::to_string(integer->value);
    return StringObject::create(str);
}

static Object* intToInteger(Object* a)
{
    return a;
}

static Object* intToReal(Object* a)
{
    auto value = ((IntObject*)a)->value;
    return RealObject::create((double)value);
}

static Object* intToBool(Object* a)
{
    auto integer = (IntObject*)a;
    return P_BOOL(integer->value);
}

BINARY_OP(intAdd, +)
BINARY_OP(intSub, -)
BINARY_OP(intMul, *)
BINARY_OP(intMod, %)

static Object* intDiv(Object* a, Object* b)
{
    CHECK_INT(a);
    CHECK_INT(b);
    auto arg1 = (IntObject*)a;
    auto arg2 = (IntObject*)b;
    auto result = (double)arg1->value / (double)arg2->value;
    return RealObject::create(result);
}

static Object* intFloorDiv(Object* a, Object* b)
{
    CHECK_INT(a);
    CHECK_INT(b);
    auto arg1 = (IntObject*)a;
    auto arg2 = (IntObject*)b;
    auto result = arg1->value / arg2->value;
    return IntObject::create(result);
}

static Object* intNeg(Object* a)
{
    auto arg = (IntObject*)a;
    return IntObject::create(-arg->value);
}

static Object* intPos(Object* a)
{
    return (IntObject*)a;
}

Object* allocIntObject();

namespace vm
{
    ObjectType intObjectType =
    {
        .base = &objectObjectType,
        .name = "Integer",
        .type = ObjectTypes::INTEGER,
        .alloc = &allocIntObject,
        .operators =
        {
            .toString = intToString,
            .toInteger = intToInteger,
            .toReal = intToReal,
            .toBool = intToBool,
            .add = intAdd,
            .sub = intSub,
            .mul = intMul,
            .div = intDiv,
            .floorDiv = intFloorDiv,
            .mod = intMod,
            .pos = intPos,
            .neg = intNeg,
        },
        .comparison = intComparison,
    };
}

Object* allocIntObject()
{
    auto intObject = new IntObject;
    intObject->objectType = &intObjectType;
    return (Object*)intObject;
}

IntObject* vm::IntObject::create(i64 value)
{
    auto intObject = (IntObject*)allocObject(&intObjectType);
    intObject->value = value;
    return intObject;
}
