﻿#include <string>

#include "int_object.h"
#include "native_function_object.h"
#include "string_object.h"
#include "bool_object.h"
#include "real_object.h"

using namespace vm;
extern ObjectType objectObjectType;

#define BINARY_OP(name, op)                  \
static Object* name(Object* a, Object* b)    \
{                                            \
    auto arg1 = (IntObject*)a;               \
    auto arg2 = (IntObject*)b;               \
    int result = arg1->value op arg2->value; \
    return IntObject::create(result);        \
}

static Object* intToString(Object* a)
{
    auto integer = (IntObject*)a;
    auto str = std::to_string(integer->value);
    return StringObject::create(str);
}

static Object* intToBool(Object* a)
{
    auto integer = (IntObject*)a;
    return BoolObject::create((bool)integer->value);
}

BINARY_OP(intAdd, +)
BINARY_OP(intSub, -)
BINARY_OP(intMul, *)
BINARY_OP(intMod, %)

static Object* intDiv(Object* a, Object* b)
{
    auto arg1 = (IntObject*)a;
    auto arg2 = (IntObject*)b;
    auto result = (double)arg1->value / (double)arg2->value;
    return RealObject::create(result);
}

static Object* intInc(Object* a)
{
    auto arg = (IntObject*)a;
    return IntObject::create(arg->value + 1);
}

static Object* intDec(Object* a)
{
    auto arg = (IntObject*)a;
    return IntObject::create(arg->value - 1);
}

static Object* intNeg(Object* a)
{
    auto arg = (IntObject*)a;
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
            .toString = intToString,
            .toBool = intToBool,
            .add = intAdd,
            .sub = intSub,
            .mul = intMul,
            .div = intDiv,
            .mod = intMod,
            .inc = intInc,
            .dec = intDec,
            .neg = intNeg,
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
