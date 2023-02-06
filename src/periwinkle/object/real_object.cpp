#include <string>
#include <sstream>
#include <cmath>

#include "real_object.h"
#include "native_function_object.h"
#include "string_object.h"
#include "int_object.h"
#include "exception_object.h"

using namespace vm;
extern ObjectType objectObjectType;

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

static Object* realToString(Object* a)
{
    auto real = (RealObject*)a;
    std::stringstream ss;
    ss.precision(10);
    ss << real->value;
    return StringObject::create(ss.str());
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

Object* allocRealObject();

namespace vm
{
    ObjectType realObjectType =
    {
        .base = &objectObjectType,
        .name = "Real",
        .type = ObjectTypes::REAL,
        .alloc = &allocRealObject,
        .operators = new ObjectOperators
        {
            .toString = realToString,
            .add = realAdd,
            .sub = realSub,
            .mul = realMul,
            .div = realDiv,
            .floorDiv = realFloorDiv,
            .pos = realPos,
            .neg = realNeg,
        },
    };
}

Object* allocRealObject()
{
    auto realObject = new RealObject;
    realObject->objectType = &realObjectType;
    return (Object*)realObject;
}

RealObject* vm::RealObject::create(double value)
{
    auto realObject = (RealObject*)allocObject(&realObjectType);
    realObject->value = value;
    return realObject;
}
