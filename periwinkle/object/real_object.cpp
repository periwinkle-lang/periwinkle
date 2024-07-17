#include <string>
#include <sstream>
#include <cmath>
#include <limits>

#include "object.hpp"
#include "real_object.hpp"
#include "native_function_object.hpp"
#include "string_object.hpp"
#include "int_object.hpp"
#include "exception_object.hpp"
#include "bool_object.hpp"
#include "native_method_object.hpp"
#include "periwinkle.hpp"

using namespace vm;

static bool tryConvertToDouble(Object* o, double& d)
{
    if (OBJECT_IS(o, &intObjectType) == false)
    {
        return false;
    }
    d = (double)((IntObject*)o)->value;
    return true;
}

#define TO_DOUBLE(object, d)                        \
    if (OBJECT_IS(object, &realObjectType))         \
        d = ((RealObject*)object)->value;           \
    else                                            \
    {                                               \
        if (tryConvertToDouble(object, d) == false) \
            return &P_NotImplemented;               \
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

static Object* realInit(Object* o, std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    return args[0]->toReal();
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

static Object* realDiv(Object* o1, Object* o2)
{
    double a, b;
    TO_DOUBLE(o1, a);
    TO_DOUBLE(o2, b);
    if (b == 0)
    {
        getCurrentState()->setException(
            &DivisionByZeroErrorObjectType, "Ділення на нуль");
        return nullptr;
    }
    double result = a / b;
    return RealObject::create(result);
}

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
        .size = sizeof(RealObject),
        .alloc = DEFAULT_ALLOC(RealObject),
        .dealloc = DEFAULT_DEALLOC(RealObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, realInit, realObjectType, nullptr),
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
