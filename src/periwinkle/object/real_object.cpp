#include <string>
#include <sstream>
#include <cmath>
#include <limits>

#include "real_object.h"
#include "native_function_object.h"
#include "string_object.h"
#include "int_object.h"
#include "exception_object.h"
#include "bool_object.h"
#include "native_method_object.h"

using namespace vm;

static bool tryConvertToDouble(Object* o, double& d)
{
    if (o->objectType->type != ObjectTypes::INTEGER)
    {
        return false;
    }
    d = (double)((IntObject*)o)->value;
    return true;
}

#define TO_DOUBLE(object, d)                           \
    if (object->objectType->type == ObjectTypes::REAL) \
        d = ((RealObject*)object)->value;              \
    else                                               \
    {                                                  \
        if (tryConvertToDouble(object, d) == false)    \
            return &P_NotImplemented;                  \
    }

#define BINARY_OP(name, op)                     \
static Object* name(Object* o1, Object* o2)     \
{                                               \
    double a, b;                                \
    TO_DOUBLE(o1, a);                           \
    TO_DOUBLE(o2, b);                           \
    double result = a op b;                     \
    return RealObject::create(result);          \
}

static Object* realInit(Object* o, std::span<Object*> args, ArrayObject* va)
{
    return Object::toReal(args[0]);
}

static Object* realComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    double a, b;
    TO_DOUBLE(o1, a);
    TO_DOUBLE(o2, b);
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

static Object* realToString(Object* a)
{
    auto real = (RealObject*)a;
    std::stringstream ss;
    ss.precision(std::numeric_limits<double>::max_digits10);
    ss << real->value;
    auto s = ss.str();
    if (s.find('.') == std::string::npos)
    {
        s.append(".0");
    }
    return StringObject::create(s);
}

static Object* realToInteger(Object* a)
{
    double value = ((RealObject*)a)->value;
    return IntObject::create((i64)value);
}

static Object* realToReal(Object* a)
{
    return a;
}

static Object* realToBool(Object* a)
{
    double value = ((RealObject*)a)->value;
    return P_BOOL(value == 0.0);
}


BINARY_OP(realAdd, +)
BINARY_OP(realSub, -)
BINARY_OP(realMul, *)
BINARY_OP(realDiv, / )

static Object* realFloorDiv(Object* o1, Object* o2)
{
    double a, b;
    TO_DOUBLE(o1, a);
    TO_DOUBLE(o2, b);
    auto result = std::floor(a / b);
    return RealObject::create(result);
}

static Object* realPos(Object* a)
{
    return (RealObject*)a;
}

static Object* realNeg(Object* a)
{
    auto arg = (RealObject*)a;
    return RealObject::create(-arg->value);
}

namespace vm
{
    TypeObject realObjectType =
    {
        .base = &objectObjectType,
        .name = "Дійсний",
        .type = ObjectTypes::REAL,
        .alloc = DEFAULT_ALLOC(RealObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, realInit, realObjectType),
        .operators =
        {
            .toString = realToString,
            .toInteger = realToInteger,
            .toReal = realToReal,
            .toBool = realToBool,
            .add = realAdd,
            .sub = realSub,
            .mul = realMul,
            .div = realDiv,
            .floorDiv = realFloorDiv,
            .pos = realPos,
            .neg = realNeg,
        },
        .comparison = realComparison,
    };
}

RealObject* vm::RealObject::create(double value)
{
    auto realObject = (RealObject*)allocObject(&realObjectType);
    realObject->value = value;
    return realObject;
}
