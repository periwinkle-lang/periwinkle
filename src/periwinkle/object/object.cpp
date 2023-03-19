#include <cstddef>

#include "object.h"
#include "bool_object.h"
#include "exception_object.h"
#include "vm.h"
#include "utils.h"

using namespace vm;

namespace vm
{
    ObjectType objectObjectType =
    {
        .base = nullptr, // Object - базовий тип для всіх типів, і тому ні від кого не наслідується
        .name = "Object",
        .type = ObjectTypes::OBJECT,
    };
}

Object* vm::allocObject(ObjectType const *objectType)
{
    return objectType->alloc();
}

std::string vm::objectTypeToString(const ObjectType *type)
{
    return type->name;
}

#define GET_OPERATOR(object, op) (object)->objectType->operators.op

// Повертає посилання на binaryFunction з структуки ObjectOperators за зсувом
#define GET_BINARY_OPERATOR_BY_OFFSET(object, operatorOffset) \
    (*(binaryFunction*)(&((char*)&object->objectType->operators)[operatorOffset]));

#define OPERATOR_OFFSET(op) offsetof(ObjectOperators, op)

static Object* callBinaryOperator(Object* o1, Object* o2, size_t operatorOffset)
{
    auto o1Operator = GET_BINARY_OPERATOR_BY_OFFSET(o1, operatorOffset);
    auto o2Operator = GET_BINARY_OPERATOR_BY_OFFSET(o2, operatorOffset);

    Object* result;
    if (o1Operator)
    {
        if (o1Operator == o2Operator)
        {
            return o1Operator(o1, o2);
        }
        result = o1Operator(o1, o2);
        if (result != &P_NotImplemented)
        {
            return result;
        }
    }

    if (o2Operator)
    {
        result = o2Operator(o1, o2);
        if (result != &P_NotImplemented)
        {
            return result;
        }
    }
    return &P_NotImplemented;
}

static Object* callCompareOperator(Object* o1, Object* o2, ObjectCompOperator op)
{
    auto o1Operator = o1->objectType->comparison;
    auto o2Operator = o2->objectType->comparison;

    Object* result;
    if (o1Operator)
    {
        if (o1Operator == o2Operator)
        {
            return o1Operator(o1, o2, op);
        }
        result = o1Operator(o1, o2, op);
        if (result != &P_NotImplemented)
        {
            return result;
        }
    }

    if (o2Operator)
    {
        result = o2Operator(o1, o2, op);
        if (result != &P_NotImplemented)
        {
            return result;
        }
    }
    return &P_NotImplemented;
}

#define UNARY_OPERATOR(op_name, op)                                           \
    Object* vm::Object::op_name(Object* o)                                    \
    {                                                                         \
        auto op_ = GET_OPERATOR(o, op_name);                                  \
        if (op_ == nullptr)                                                   \
        {                                                                     \
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,   \
                utils::format(                                                \
                "Неправильний тип операнда \"%s\" для унарного оператора %s", \
                o->objectType->name.c_str(), #op));                           \
        }                                                                     \
        return op_(o);                                                        \
    }

#define BINARY_OPERATOR(op_name, op)                                               \
    Object* vm::Object::op_name(Object* o1, Object* o2)                            \
    {                                                                              \
        auto result = callBinaryOperator(o1, o2, OPERATOR_OFFSET(op_name));        \
        if (result == &P_NotImplemented)                                           \
        {                                                                          \
            VirtualMachine::currentVm->throwException(&TypeErrorObjectType,        \
                utils::format(                                                     \
                "Непідтримувані типи операндів \"%s\" та \"%s\" для оператора %s", \
                o1->objectType->name.c_str(), o2->objectType->name.c_str(), #op)); \
        }                                                                          \
        return result;                                                             \
    }

Object* vm::Object::compare(Object* o1, Object* o2, ObjectCompOperator op)
{
    auto result = callCompareOperator(o1, o2, op);
    if (result == &P_NotImplemented)
    {

        std::string opName;
        using enum ObjectCompOperator;
        switch (op)
        {
        case EQ: opName = "=="; break;
        case NE: opName = "!="; break;
        case GT: opName = "більше"; break;
        case GE: opName = "більше="; break;
        case LT: opName = "менше"; break;
        case LE: opName = "менше="; break;
        }
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType, utils::format(
            "Неможливо порівняти об'єкти типів \"%s\" та \"%s\" за допомогою оператора %s",
            o1->objectType->name.c_str(), o2->objectType->name.c_str(), opName.c_str()));
    }
    return result;
}

Object* vm::Object::call(Object* callable, Object**& sp, WORD argc)
{
    auto callOp = GET_OPERATOR(callable, call);
    if (callOp == nullptr)
    {
        VirtualMachine::currentVm->throwException(&TypeErrorObjectType,
            utils::format("Об'єкт типу \"%s\" не може бути викликаний",
                callable->objectType->name.c_str())
        );
    }

    auto result = callOp(callable, sp, argc);
    return result;
}

UNARY_OPERATOR(toString, toString)
UNARY_OPERATOR(toInteger, toInteger)
UNARY_OPERATOR(toReal, toReal)
UNARY_OPERATOR(toBool, toBool)
BINARY_OPERATOR(add, +)
BINARY_OPERATOR(sub, -)
BINARY_OPERATOR(mul, *)
BINARY_OPERATOR(div, /)
BINARY_OPERATOR(floorDiv, \\)
BINARY_OPERATOR(mod, %)
UNARY_OPERATOR(pos, +)
UNARY_OPERATOR(neg, -)

Object* vm::Object::getAttr(Object* o, const std::string& name)
{
    if (o->objectType->attributes.contains(name))
    {
        return o->objectType->attributes[name];
    }
    return nullptr;
}
