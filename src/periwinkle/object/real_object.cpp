#include <string>
#include <sstream>

#include "real_object.h"
#include "native_function_object.h"
#include "string_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define BINARY_OP(name, op)                     \
Object* name(Object* args[])                    \
{                                               \
    auto arg1 = (RealObject*)args[0];           \
    auto arg2 = (RealObject*)args[1];           \
    double result = arg1->value op arg2->value; \
    return RealObject::create(result);          \
}

Object* realToString(Object* args[])
{
    auto real = (RealObject*)args[0];
    std::stringstream ss; 
    ss.precision(10);
    ss << real->value;
    return StringObject::create(ss.str());
}

BINARY_OP(realAdd, +)
BINARY_OP(realSub, -)
BINARY_OP(realMul, *)
BINARY_OP(realDiv, / )

Object* realInc(Object* args[])
{
    auto arg = (RealObject*)args[0];
    return RealObject::create(arg->value + 1.);
}

Object* realDec(Object* args[])
{
    auto arg = (RealObject*)args[0];
    return RealObject::create(arg->value - 1.);
}

Object* realNeg(Object* args[])
{
    auto arg = (RealObject*)args[0];
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
            .toString = NativeFunctionObject::create(1, "toString", realToString),
            .add = NativeFunctionObject::create(2, "add", realAdd),
            .sub = NativeFunctionObject::create(2, "sub", realSub),
            .mul = NativeFunctionObject::create(2, "mul", realMul),
            .div = NativeFunctionObject::create(2, "div", realDiv),
            .inc = NativeFunctionObject::create(1, "inc", realInc),
            .dec = NativeFunctionObject::create(1, "dec", realDec),
            .neg = NativeFunctionObject::create(1, "neg", realNeg),
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
