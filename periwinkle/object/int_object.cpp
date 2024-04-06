#include <string>

#include "object.hpp"
#include "int_object.hpp"
#include "native_function_object.hpp"
#include "string_object.hpp"
#include "bool_object.hpp"
#include "real_object.hpp"
#include "exception_object.hpp"
#include "native_method_object.hpp"
#include "argument_parser.hpp"

using namespace vm;

#define CHECK_INT(object)                           \
    if (OBJECT_IS(object, &intObjectType) == false) \
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

static IntObject ten = { {.objectType = &intObjectType}, 10 };
static DefaultParameters intInitDefaults = { {"основа"}, { &ten } };

static Object* intInit(Object* o, std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    Object* x;
    IntObject* base;
    ArgParser argParser{
        {&x, objectObjectType, "значення"},
        {&base, intObjectType, "основа"}
    };
    argParser.parse(args, &intInitDefaults, na);

    if (OBJECT_IS(x, &stringObjectType))
    {
        auto value = stringObjectToInt((StringObject*)x, (base->value == -1 ? 10 : base->value));
        return IntObject::create(value);
    }
    else if (base->value != -1)
    {
        VirtualMachine::currentVm->throwException(
            &ValueErrorObjectType, "Основа може бути задана лише якщо перший аргумент є стрічкою");
    }

    return Object::toInteger(args[0]);
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
    if (((IntObject*)b)->value == 0)
    {
        VirtualMachine::currentVm->throwException(
            &DivisionByZeroErrorObjectType, "Ділення на нуль");
    }
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

namespace vm
{
    TypeObject intObjectType =
    {
        .base = &objectObjectType,
        .name = "Число",
        .alloc = DEFAULT_ALLOC(IntObject),
        .constructor = new NATIVE_METHOD("конструктор", 1, false, intInit, intObjectType, &intInitDefaults),
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

IntObject* vm::IntObject::create(i64 value)
{
    auto intObject = (IntObject*)allocObject(&intObjectType);
    intObject->value = value;
    return intObject;
}
