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

Object* vm::getPublicAttribute(Object *object, std::string name)
{
    auto attribute = object->objectType->publicAttributes->find(name);
    if (attribute != object->objectType->publicAttributes->end())
    {
        return attribute->second;
    }
    return nullptr;
}

Object* vm::getPrivateAttribute(Object *object, std::string name)
{
    auto attribute = object->objectType->privateAttributes->find(name);
    if (attribute != object->objectType->privateAttributes->end())
    {
        return attribute->second;
    }
    return nullptr;
}

#define GET_OPERATOR(object, op) (object)->objectType->operators->op

// Повертає посилання на binaryFunction з структуки ObjectOperators за зсувом
#define GET_BINARY_OPERATOR_BY_OFFSET(object, offset) \
    (*(binaryFunction*)(&((char*)object->objectType->operators)[operatorOffset]));

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


#define CALL_UNARY_OPERATOR(object, op)  \
    auto op_ = GET_OPERATOR(object, op); \
    if (op_ == NULL)                     \
    {                                    \
        return &P_NotImplemented;        \
    }                                    \
    return op_(object);

#define UNARY_OPERATOR(op_name, op)                                           \
    Object* vm::Object::op_name(Object* o)                                    \
    {                                                                         \
        auto op_ = GET_OPERATOR(o, op_name);                                  \
        if (op_ == NULL)                                                      \
        {                                                                     \
            _currentVM->throwException(&TypeErrorObjectType, utils::format(   \
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
            _currentVM->throwException(&TypeErrorObjectType, utils::format(        \
                "Непідтримувані типи операндів \"%s\" та \"%s\" для оператора %s", \
                o1->objectType->name.c_str(), o2->objectType->name.c_str(), #op)); \
        }                                                                          \
        return result;                                                             \
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
