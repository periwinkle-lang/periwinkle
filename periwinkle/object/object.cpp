#include <cstddef>
#include <format>
#include <algorithm>
#include <string_view>
#include <unordered_map>

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
#include "tuple_object.hpp"
#include "periwinkle.hpp"
#include "keyword.hpp"
#include "plogger.hpp"

using namespace vm;

static Object* typeCall(TypeObject* type, std::span<Object*> argv, TupleObject* va, NamedArgs* na)
{
    if (type->constructor == nullptr)
    {
        getCurrentState()->setException(
            &TypeErrorObjectType,
            std::format("Неможливо створити екземпляр з типом \"{}\"", type->name)
        );
        return nullptr;
    }

    auto instance = type->constructor(type, argv, va, na);
    if (!instance) return nullptr;
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
        .name = "Об'єкт",
        .size = sizeof(Object),
    };

    TypeObject typeObjectType =
    {
        .base = &objectObjectType,
        .name = "Тип",
        .size = sizeof(TypeObject),
        .callableInfoOffset = CALLABLE_INFO_OFFSET(TypeObject, callableInfo),
        .operators =
        {
            .call = (callFunction)typeCall,
            .toString = (unaryFunction)typeToString,
        },
    };

    const char* constructorName = "конструктор";
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

#define GET_CALLABLE_INFO(object) \
    reinterpret_cast<vm::CallableInfo*>(reinterpret_cast<char*>(object) + (object)->objectType->callableInfoOffset)

bool vm::validateCall(Object* callable, WORD argc, vm::NamedArgs* namedArgs)
{
    auto callableInfo = GET_CALLABLE_INFO(callable);
    auto defaultCount = callableInfo->flags & CallableInfo::HAS_DEFAULTS ?
        callableInfo->defaults->parameters.size() : 0;
    std::string_view callableType = callableInfo->flags & CallableInfo::IS_METHOD ? "Метод" : "Функція";
    if (namedArgs && defaultCount == 0 && namedArgs->count != 0)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format("{} \"{}\" не має іменованих параметрів", callableType, callableInfo->name)
        );
        return false;
    }

    auto arityWithoutDefaults = callableInfo->arity - defaultCount;
    if (argc < arityWithoutDefaults)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format(
                "{} \"{}\" очікує {} {}, натомість передано {}",
                callableType, callableInfo->name, arityWithoutDefaults,
                utils::wordDeclension(arityWithoutDefaults, "аргумент"), argc)
        );
        return false;
    }

    if (callableInfo->flags & CallableInfo::IS_VARIADIC)
    {
        if (auto variadicCount = argc - arityWithoutDefaults; variadicCount > 0)
        {
            // Тепер змінна argc позначає кількість переданих змінних
            // без урахування варіативних аргументів
            argc -= variadicCount;
        }
    }
    else
    {
        if (argc > callableInfo->arity && defaultCount == 0)
        {
            getCurrentState()->setException(&TypeErrorObjectType,
                std::format(
                    "{} \"{}\" очікує {} {}, натомість передано {}",
                    callableType, callableInfo->name, arityWithoutDefaults,
                    utils::wordDeclension(arityWithoutDefaults, "аргумент"), argc)
            );
            return false;
        }
    }

    if (defaultCount)
    {
        if (argc > callableInfo->arity)
        {
            getCurrentState()->setException(&TypeErrorObjectType,
                std::format(
                    "{} \"{}\" очікує від {} до {} аргументів, натомість передано {}",
                    callableType, callableInfo->name, arityWithoutDefaults, callableInfo->arity, argc)
            );
            return false;
        }

        if (namedArgs != nullptr)
        {
            namedArgs->indexes.reserve(namedArgs->count);
            for (size_t i = 0; i < namedArgs->count; ++i)
            {
                std::string_view argName = namedArgs->names[i];
                const auto& defaults = callableInfo->defaults->parameters;
                auto it = std::find_if(defaults.begin(), defaults.end(),
                    [argName](const DefaultParameters::Pair& item)
                    {
                        return item.first == argName;
                    }
                );
                if (it != defaults.end())
                {
                    namedArgs->indexes.push_back(it - defaults.begin());
                }
                else
                {
                    getCurrentState()->setException(&TypeErrorObjectType,
                        std::format(
                            "{} \"{}\" не має параметра за замовчуванням з іменем \"{}\"",
                            callableType, callableInfo->name, argName)
                    );
                    return false;
                }
            }

            if (argc > arityWithoutDefaults)
            {
                auto overflow = argc - arityWithoutDefaults;
                auto maxIt = std::max_element(namedArgs->indexes.begin(), namedArgs->indexes.end(),
                    [overflow](size_t a, size_t b) { return a < overflow && a > b; });

                if (maxIt != namedArgs->indexes.end())
                {
                    getCurrentState()->setException(&TypeErrorObjectType,
                        std::format(
                            "{} \"{}\", аргумент \"{}\" приймає два значення",
                            callableType, callableInfo->name,
                            namedArgs->names[maxIt - namedArgs->indexes.begin()]
                        )
                    );
                    return false;
                }
            }
        }
    }
    return true;
}

#define GET_OPERATOR(object, op) (object)->objectType->operators.op

// Повертає посилання на binaryFunction з структури ObjectOperators за зсувом
#define GET_BINARY_OPERATOR_BY_OFFSET(object, operatorOffset) \
    (*reinterpret_cast<binaryFunction*>(reinterpret_cast<char*>(&(object)->objectType->operators) + operatorOffset))

// Повертає посилання на unaryFunction з структури ObjectOperators за зсувом
#define GET_UNARY_OPERATOR_BY_OFFSET(object, operatorOffset) \
    (*reinterpret_cast<unaryFunction*>(reinterpret_cast<char*>(&(object)->objectType->operators) + operatorOffset))

#define OPERATOR_OFFSET(op) offsetof(ObjectOperators, op)

static Object* _callBinaryOperator(Object* o1, Object* o2, size_t operatorOffset)
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

static constexpr std::string_view UNARY_OPERATOR_ERROR_MSG = "Неправильний тип операнда \"{}\" для унарного оператора {}";

#define UNARY_OPERATOR(op_name, op)                               \
    Object* vm::Object::op_name()                                 \
    {                                                             \
        auto op_ = GET_OPERATOR(this, op_name);                   \
        if (op_ == nullptr)                                       \
        {                                                         \
            getCurrentState()->setException(&TypeErrorObjectType, \
                std::format(                                      \
                UNARY_OPERATOR_ERROR_MSG,                         \
                objectType->name, #op));                          \
            return nullptr;                                       \
        }                                                         \
        return op_(this);                                         \
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

static constexpr std::string_view BINARY_OPERATOR_ERROR_MSG = "Непідтримувані типи операндів \"{}\" та \"{}\" для оператора {}";

#define BINARY_OPERATOR(op_name, op)                                          \
    Object* vm::Object::op_name(Object* o)                                    \
    {                                                                         \
        auto result = _callBinaryOperator(this, o, OPERATOR_OFFSET(op_name)); \
        if (result == &P_NotImplemented)                                      \
        {                                                                     \
            getCurrentState()->setException(&TypeErrorObjectType,             \
                std::format(BINARY_OPERATOR_ERROR_MSG,                        \
                    objectType->name, o->objectType->name, op));              \
            return nullptr;                                                   \
        }                                                                     \
        return result;                                                        \
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

        case GT: opName = Keyword::GREATER; break;
        case GE: opName = Keyword::GREATER_EQUAL; break;
        case LT: opName = Keyword::LESS; break;
        case LE: opName = Keyword::LESS_EQUAL; break;
        }
        getCurrentState()->setException(&TypeErrorObjectType, std::format(
            "Неможливо порівняти об'єкти типів \"{}\" та \"{}\" за допомогою оператора {}",
            objectType->name, o->objectType->name, opName));
        return nullptr;
    }
    return result;
}

static inline Object* _callObject(Object* callable, std::span<Object*> argv, NamedArgs* na)
{
    auto callableInfo = GET_CALLABLE_INFO(callable);

    if (!validateCall(callable, argv.size(), na))
        return nullptr;

    auto argc = argv.size();
    auto va = &P_emptyTuple;
    if (callableInfo->flags & CallableInfo::IS_VARIADIC)
    {
        auto arityWithoutDefaults = callableInfo->arity;
        if (callableInfo->flags & CallableInfo::HAS_DEFAULTS)
            arityWithoutDefaults -= callableInfo->defaults->parameters.size();
        if (auto variadicCount = argc - arityWithoutDefaults; variadicCount > 0)
        {
            va = TupleObject::create();
            va->items.reserve(variadicCount);
            va->items.insert(va->items.end(), argv.end() - variadicCount, argv.end());
            va->items.shrink_to_fit();
            argc -= variadicCount;
        }
    }
    return GET_OPERATOR(callable, call)(callable, argv.first(argc), va, na);
}

static constexpr std::string_view ERROR_MSG_NOT_CALLABLE{ "Об'єкт типу \"{}\" не може бути викликаний" };

Object* vm::Object::call(std::span<Object*> argv, NamedArgs* na)
{
    auto callOp = GET_OPERATOR(this, call);
    if (callOp == nullptr)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format(ERROR_MSG_NOT_CALLABLE, objectType->name)
        );
        return nullptr;
    }
    return _callObject(this, argv, na);
}

Object* vm::Object::stackCall(Object**& sp, u64 argc, NamedArgs* na)
{
    Object* result;
    auto callableInfo = GET_CALLABLE_INFO(this);
    auto stackCallOp = GET_OPERATOR(this, stackCall);
    if (stackCallOp != nullptr)
    {
        if (!validateCall(this, argc, na))
            return nullptr;

        auto defaultCount = callableInfo->flags & CallableInfo::HAS_DEFAULTS ?
            callableInfo->defaults->parameters.size() : 0;

        // Варіативний аргумент
        if (callableInfo->flags & CallableInfo::IS_VARIADIC)
        {
            auto va = &P_emptyTuple;
            if (auto variadicCount = argc - (callableInfo->arity - defaultCount); variadicCount > 0)
            {
                va = TupleObject::create();
                va->items.reserve(variadicCount);
                va->items.insert(va->items.end(), sp - variadicCount + 1, sp + 1);
                va->items.shrink_to_fit();
                argc -= variadicCount;
                sp -= variadicCount;
            }
            *(++sp) = va;
        }

        if (defaultCount)
        {
            if (na != nullptr)
            {
                for (size_t i = 0, j = na->count; i < defaultCount; ++i)
                {
                    if (j)
                    {
                        auto it = std::find(na->indexes.begin(), na->indexes.end(), i);
                        if (it != na->indexes.end())
                        {
                            auto index = it - na->indexes.begin();
                            *(++sp) = na->values[na->count - index - 1];
                            j--;
                            continue;
                        }
                    }
                    *(++sp) = callableInfo->defaults->parameters[defaultCount - i - 1].second;
                }
            }
            else
            {
                for (size_t i = 0, argLack = callableInfo->arity - argc; i < argLack; ++i)
                    *(++sp) = callableInfo->defaults->parameters[argLack - i - 1].second;
            }
        }
        result = stackCallOp(this, sp);
        // Очищення стека
        sp -= argc // аргументи
            + (callableInfo->flags & CallableInfo::IS_VARIADIC) // варіативний параметр
            + 1; // викликаний об'єкт
    }
    else
    {
        // Якщо об'єкт не підтримує stackCall, то викликається call оператор
        auto callOp = GET_OPERATOR(this, call);
        if (callOp != nullptr)
            result = _callObject(this, { sp - argc + 1, argc }, na);
        else
        {
            getCurrentState()->setException(&TypeErrorObjectType,
                std::format(ERROR_MSG_NOT_CALLABLE, objectType->name)
            );
            return nullptr;
        }
        // Очищення стека
        sp -= argc
            + 1; // викликаний об'єкт
    }
    return result;
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

static constexpr std::string_view TO_INTEGER_ERROR_MSG = "Неможливо конвертувати об'єкт типу \"{}\" в число";
static constexpr std::string_view TO_REAL_ERROR_MSG = "Неможливо конвертувати об'єкт типу \"{}\" в дійсне число";
static constexpr std::string_view GET_ITER_ERROR_MSG = "Для об'єкта типу \"{}\" неможливо отримати ітератор";

UNARY_OPERATOR_WITH_MESSAGE(toInteger, TO_INTEGER_ERROR_MSG)
UNARY_OPERATOR_WITH_MESSAGE(toReal, TO_REAL_ERROR_MSG)
BINARY_OPERATOR(add, Keyword::ADD)
BINARY_OPERATOR(sub, Keyword::SUB)
BINARY_OPERATOR(mul, Keyword::MUL)
BINARY_OPERATOR(div, Keyword::DIV)
BINARY_OPERATOR(floorDiv, Keyword::FLOOR_DIV)
BINARY_OPERATOR(mod, Keyword::MOD)
UNARY_OPERATOR(pos, Keyword::POS)
UNARY_OPERATOR(neg, Keyword::NEG)
UNARY_OPERATOR_WITH_MESSAGE(getIter, GET_ITER_ERROR_MSG)

static const std::unordered_map<size_t, const std::string_view> offsetToUnaryOperatorErrorMsg =
{
    {static_cast<size_t>(vm::ObjectOperatorOffset::GET_ITER), GET_ITER_ERROR_MSG},
};

static const std::unordered_map<size_t, const std::string_view> offsetToOperatorKeyword =
{
    {static_cast<size_t>(vm::ObjectOperatorOffset::ADD), Keyword::ADD},
    {static_cast<size_t>(vm::ObjectOperatorOffset::SUB), Keyword::SUB},
    {static_cast<size_t>(vm::ObjectOperatorOffset::MUL), Keyword::MUL},
    {static_cast<size_t>(vm::ObjectOperatorOffset::DIV), Keyword::DIV},
    {static_cast<size_t>(vm::ObjectOperatorOffset::FLOOR_DIV), Keyword::FLOOR_DIV},
    {static_cast<size_t>(vm::ObjectOperatorOffset::MOD), Keyword::MOD},
    {static_cast<size_t>(vm::ObjectOperatorOffset::POS), Keyword::POS},
    {static_cast<size_t>(vm::ObjectOperatorOffset::NEG), Keyword::NEG},
};

Object* vm::Object::callUnaryOperator(vm::ObjectOperatorOffset offset)
{
    auto op = GET_UNARY_OPERATOR_BY_OFFSET(this, static_cast<size_t>(offset));
    if (op == nullptr)
    {
        std::string errorMsg;
        if (offsetToOperatorKeyword.contains(static_cast<size_t>(offset)))
        {
            errorMsg = std::format(UNARY_OPERATOR_ERROR_MSG, objectType->name,
                offsetToOperatorKeyword.at(static_cast<size_t>(offset)));
        }
        else
        {
            errorMsg = std::vformat(offsetToUnaryOperatorErrorMsg.at(static_cast<size_t>(offset)),
                std::make_format_args(objectType->name));
        }
        getCurrentState()->setException(&TypeErrorObjectType, errorMsg);
        return nullptr;
    }
    return op(this);
}

Object* vm::Object::callBinaryOperator(Object* other, vm::ObjectOperatorOffset offset)
{
    plog::passert(offsetToOperatorKeyword.contains(static_cast<size_t>(offset))) << "Для оператора невизначено Keyword";
    auto result = _callBinaryOperator(this, other, static_cast<size_t>(offset));
    if (result == &P_NotImplemented)
    {
        getCurrentState()->setException(&TypeErrorObjectType,
            std::format(BINARY_OPERATOR_ERROR_MSG,
                objectType->name, other->objectType->name, offsetToOperatorKeyword.at(static_cast<size_t>(offset))));
        return nullptr;
    }
    return result;
}

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

std::optional<bool> vm::Object::asBool()
{
    if (this == &P_true) return true;
    if (this == &P_false) return false;
    if (OBJECT_IS(objectType, &intObjectType))
        return static_cast<bool>(static_cast<IntObject*>(this)->value);
    auto result = this->toBool();
    if (result == nullptr) return std::nullopt;
    return result == &P_true;
}
