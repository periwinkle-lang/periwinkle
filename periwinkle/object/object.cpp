#include <cstddef>
#include <format>

#include "object.hpp"
#include "bool_object.hpp"
#include "exception_object.hpp"
#include "vm.hpp"
#include "utils.hpp"
#include "string_object.hpp"
#include "native_method_object.hpp"
#include "string_vector_object.hpp"
#include "int_object.hpp"
#include "real_object.hpp"
#include "periwinkle.hpp"

using namespace vm;

static Object* typeCall(TypeObject* type, Object**& sp, WORD argc, NamedArgs* na)
{
    if (type->constructor == nullptr)
    {
        getCurrentState()->setException(
            &TypeErrorObjectType,
            std::format("Неможливо створити екземпляр з типом \"{}\"",
                type->name)
        );
        return nullptr;
    }

    auto instance = callNativeMethod(type, type->constructor, {sp - argc + 1, argc}, na);
    if (!instance) return nullptr;
    sp -= argc + 1;
    return instance;
}

static Object* typeToString(TypeObject* type)
{
    auto str = StringObject::create(std::format("<Тип {}>", type->name));
    return str;
}

namespace vm
{
    TypeObject objectObjectType =
    {
        // Object - базовий тип для всіх типів, і тому ні від кого не наслідується
        .base = nullptr,
        .name = "Обєкт",
        .size = sizeof(Object),
    };

    TypeObject typeObjectType =
    {
        .base = &objectObjectType,
        .name = "Тип",
        .size = sizeof(TypeObject),
        .operators =
        {
            .call = (callFunction)typeCall,
            .toString = (unaryFunction)typeToString,
        },
    };
}

void vm::mark(Object* o)
{
    if (o == nullptr || o->marked)
    {
        return;
    }
    o->marked = true;
    if (auto traverse = o->objectType->traverse)
    {
        traverse(o);
    }
}

Object* vm::allocObject(TypeObject* objectType)
{
    auto o = objectType->alloc();
    o->objectType = objectType;
    getCurrentState()->getGC()->addObject(o);
    return o;
}

bool vm::isInstance(const Object* o, const TypeObject& type)
{
    auto oType = o->objectType;
    for (;;)
    {
        if (oType == &type)
        {
            return true;
        }
        else if (oType->base == nullptr)
        {
            return false;
        }

        oType = oType->base;
    }
}

bool vm::objectToBool(Object* o)
{
    if (o == &P_true) return true;
    if (o == &P_false) return false;
    if (OBJECT_IS(o, &intObjectType)) return static_cast<IntObject*>(o)->value;
    if (OBJECT_IS(o, &realObjectType)) return static_cast<RealObject*>(o)->value;
    if (OBJECT_IS(o, &stringObjectType)) return !static_cast<StringObject*>(o)->value.empty();
    if (OBJECT_IS(o, &listObjectType)) return !static_cast<ListObject*>(o)->items.empty();
    return static_cast<BoolObject*>(o->toBool())->value;
}

#define GET_OPERATOR(object, op) (object)->objectType->operators.op

// Повертає посилання на binaryFunction з структури ObjectOperators за зсувом
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
    Object* vm::Object::op_name()                                             \
    {                                                                         \
        auto op_ = GET_OPERATOR(this, op_name);                               \
        if (op_ == nullptr)                                                   \
        {                                                                     \
            getCurrentState()->setException(&TypeErrorObjectType,             \
                std::format(                                                  \
                "Неправильний тип операнда \"{}\" для унарного оператора {}", \
                objectType->name, #op));                                      \
            return nullptr;                                                   \
        }                                                                     \
        return op_(this);                                                     \
    }

#define UNARY_OPERATOR_WITH_MESSAGE(op_name, message)              \
    Object* vm::Object::op_name()                                  \
    {                                                              \
        auto op_ = GET_OPERATOR(this, op_name);                    \
        if (op_ == nullptr)                                        \
        {                                                          \
            getCurrentState()->setException(&TypeErrorObjectType,  \
                std::format(message, objectType->name));           \
            return nullptr;                                        \
        }                                                          \
        return op_(this);                                          \
    }

#define BINARY_OPERATOR(op_name, op)                                               \
    Object* vm::Object::op_name(Object* o)                                         \
    {                                                                              \
        auto result = callBinaryOperator(this, o, OPERATOR_OFFSET(op_name));       \
        if (result == &P_NotImplemented)                                           \
        {                                                                          \
            getCurrentState()->setException(&TypeErrorObjectType,                  \
                std::format(                                                       \
                "Непідтримувані типи операндів \"{}\" та \"{}\" для оператора {}", \
                objectType->name, o->objectType->name, #op));                      \
            return nullptr;                                                        \
        }                                                                          \
        return result;                                                             \
    }

Object* vm::Object::compare(Object* o, ObjectCompOperator op)
{
    auto result = callCompareOperator(this, o, op);
    if (result == &P_NotImplemented)
    {
        std::string opName;
        using enum ObjectCompOperator;
        switch (op)
        {
        // Якщо оператор порівння нереалізовано, то вони порівнюються за посиланням.
        case EQ: return P_BOOL(this == o); break;
        case NE: return P_BOOL(this != o); break;

        case GT: opName = "більше"; break;
        case GE: opName = "більше="; break;
        case LT: opName = "менше"; break;
        case LE: opName = "менше="; break;
        }
        getCurrentState()->setException(&TypeErrorObjectType, std::format(
            "Неможливо порівняти об'єкти типів \"{}\" та \"{}\" за допомогою оператора {}",
            objectType->name, o->objectType->name, opName));
        return nullptr;
    }
    return result;
}

Object* vm::Object::call(Object**& sp, WORD argc, NamedArgs* namedArgs)
{
    auto callOp = GET_OPERATOR(this, call);
    if (callOp == nullptr)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format("Об'єкт типу \"{}\" не може бути викликаний",
                objectType->name)
        );
        return nullptr;
    }

    return callOp(this, sp, argc, namedArgs);
}

Object* vm::Object::toString()
{
    auto op = GET_OPERATOR(this, toString);
    if (op == nullptr)
    {
        return StringObject::create(
            std::format("<екземпляр класу {} {}>", objectType->name, static_cast<void*>(this)));
    }
    return op(this);
}

Object* vm::Object::toBool()
{
    auto op = GET_OPERATOR(this, toBool);
    if (op == nullptr)
    {
        return &P_true;
    }
    return op(this);
}

UNARY_OPERATOR_WITH_MESSAGE(toInteger, "Неможливо конвертувати об'єкт типу \"{}\" в число")
UNARY_OPERATOR_WITH_MESSAGE(toReal, "Неможливо конвертувати об'єкт типу \"{}\" в дійсне число")
BINARY_OPERATOR(add, +)
BINARY_OPERATOR(sub, -)
BINARY_OPERATOR(mul, *)
BINARY_OPERATOR(div, /)
BINARY_OPERATOR(floorDiv, \\)
BINARY_OPERATOR(mod, %)
UNARY_OPERATOR(pos, +)
UNARY_OPERATOR(neg, -)
UNARY_OPERATOR_WITH_MESSAGE(getIter, "Для об'єкта типу \"{}\" неможливо отримати ітератор")

Object* vm::Object::getAttr(const std::string& name)
{
    if (objectType->attributes.contains(name))
    {
        return objectType->attributes[name];
    }
    else if (OBJECT_IS(this, &objectObjectType))
    {
        auto type = (TypeObject*)this;
        if (type->attributes.contains(name))
        {
            return type->attributes[name];
        }
    }
    return nullptr;
}
