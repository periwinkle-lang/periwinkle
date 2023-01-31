#include <string>
#include <sstream>
#include <cmath>

#include "real_object.h"
#include "native_function_object.h"
#include "string_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define BINARY_OP(name, op)                     \
static Object* name(Object* a, Object* b)       \
{                                               \
    auto arg1 = (RealObject*)a;                 \
    auto arg2 = (RealObject*)b;                 \
    double result = arg1->value op arg2->value; \
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

static Object* realFloorDiv(Object* a, Object* b)
{
    auto arg1 = (RealObject*)a;
    auto arg2 = (RealObject*)b;
    auto result = std::floor(arg1->value / arg2->value);
    return RealObject::create(result);
}

static Object* realInc(Object* a)
{
    auto arg = (RealObject*)a;
    return RealObject::create(arg->value + 1.);
}

static Object* realDec(Object* a)
{
    auto arg = (RealObject*)a;
    return RealObject::create(arg->value - 1.);
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
            .inc = realInc,
            .dec = realDec,
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
