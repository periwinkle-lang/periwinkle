#include <string>

#include "int_object.h"
#include "native_function_object.h"
#include "string_object.h"
#include "bool_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define BINARY_OP(name, op)                  \
Object* name(Object* args[])                 \
{                                            \
    auto arg1 = (IntObject*)args[0];         \
    auto arg2 = (IntObject*)args[1];         \
    int result = arg1->value op arg2->value; \
    return IntObject::create(result);        \
}

Object* intToString(Object* args[])
{
    auto integer = (IntObject*)args[0];
    auto str = std::to_string(integer->value);
    return StringObject::create(str);
}

Object* intToBool(Object* args[])
{
    auto integer = (IntObject*)args[0];
    return BoolObject::create((bool)integer->value);
}

BINARY_OP(intAdd, +)
BINARY_OP(intSub, -)
BINARY_OP(intMul, *)
BINARY_OP(intDiv, /)
BINARY_OP(intMod, %)

Object* intInc(Object* args[])
{
    auto arg = (IntObject*)args[0];
    return IntObject::create(arg->value + 1);
}

Object* intDec(Object* args[])
{
    auto arg = (IntObject*)args[0];
    return IntObject::create(arg->value - 1);
}

Object* intNeg(Object* args[])
{
    auto arg = (IntObject*)args[0];
    return IntObject::create(-arg->value);
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
        .operators = new ObjectOperators
        {
            .toString = NativeFunctionObject::create(1, "toString", intToString),
            .toBool = NativeFunctionObject::create(1, "toBool", intToBool),
            .add = NativeFunctionObject::create(2, "add", intAdd),
            .sub = NativeFunctionObject::create(2, "sub", intSub),
            .mul = NativeFunctionObject::create(2, "mul", intMul),
            .div = NativeFunctionObject::create(2, "div", intDiv),
            .mod = NativeFunctionObject::create(2, "mod", intMod),
            .inc = NativeFunctionObject::create(1, "inc", intInc),
            .dec = NativeFunctionObject::create(1, "dec", intDec),
            .neg = NativeFunctionObject::create(1, "neg", intNeg),
        },
    };
}

Object* allocIntObject()
{
    auto intObject = new IntObject;
    intObject->objectType = &intObjectType;
    return (Object*)intObject;
}

IntObject* vm::IntObject::create(int value)
{
    auto intObject = (IntObject*)allocObject(&intObjectType);
    intObject->value = value;
    return intObject;
}
