#include <utility>

#include "tuple_object.hpp"
#include "bool_object.hpp"
#include "exception_object.hpp"
#include "string_object.hpp"
#include "int_object.hpp"
#include "native_method_object.hpp"
#include "utils.hpp"
#include "argument_parser.hpp"
#include "periwinkle.hpp"
#include "end_iteration_object.hpp"

using namespace vm;

#define CHECK_TUPLE(object)                           \
    if (OBJECT_IS(object, &tupleObjectType) == false) \
        return &P_NotImplemented;

#define CHECK_INDEX(index, tupleObject)                                \
    if (std::cmp_greater_equal(index, tupleObject->items.size()))      \
    {                                                                  \
        getCurrentState()->setException(                               \
            &IndexErrorObjectType, "Індекс виходить за межі кортежу"); \
        return nullptr;                                                \
    }

#define CHECK_NEGATIVE_INDEX(index, variableName)                       \
    if (index < 0LL)                                                    \
    {                                                                   \
        getCurrentState()->setException(                                \
            &IndexErrorObjectType,                                      \
            "\"" variableName "\" не може приймати від'ємних значень"); \
        return nullptr;                                                 \
    }

static Object* tupleInit(Object* o, std::span<Object*> args, TupleObject* va, NamedArgs* na)
{
    return va;
}

static inline std::optional<bool> tupleObjectEqual(TupleObject* a, TupleObject* b)
{
    if (a->items.size() != b->items.size()) return false;

    for (size_t i = 0; i < a->items.size(); ++i)
    {
        auto cmpResult = a->items[i]->compare(b->items[i], ObjectCompOperator::EQ);
        if (cmpResult == nullptr) return std::nullopt;
        auto result = cmpResult->asBool();
        if (!result) return std::nullopt;
        if (result.value() == false)
            return false;
    }

    return true;
}

static Object* tupleComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    CHECK_TUPLE(o1);
    CHECK_TUPLE(o2);
    auto a = static_cast<TupleObject*>(o1);
    auto b = static_cast<TupleObject*>(o2);

    using enum ObjectCompOperator;

    if (op == EQ) return P_BOOL(tupleObjectEqual(a, b));
    if (op == NE) return P_BOOL(!tupleObjectEqual(a, b));

    auto cmpResult = std::strong_ordering::equal;
    try
    {
        cmpResult = std::lexicographical_compare_three_way(
            a->items.begin(), a->items.end(),
            b->items.begin(), b->items.end(),
            [](Object* obj1, Object* obj2) {
                auto cmpLess = obj1->compare(obj2, ObjectCompOperator::LT);
                if (cmpLess == nullptr) throw std::runtime_error("");
                auto less = cmpLess->asBool();
                if (!less) throw std::runtime_error("");
                if (less.value())
                    return std::strong_ordering::less;

                auto cmpGreater = obj1->compare(obj2, ObjectCompOperator::GT);
                if (cmpGreater == nullptr) throw std::runtime_error("");
                auto greater = cmpGreater->asBool();
                if (!greater) throw std::runtime_error("");
                if (greater.value())
                    return std::strong_ordering::greater;

                return std::strong_ordering::equal;
            }
        );
    }
    catch (const std::runtime_error& e)
    {
        return nullptr;
    }

    bool result;
    if (op == LT) result = (cmpResult == std::strong_ordering::less);
    if (op == LE) result = (cmpResult == std::strong_ordering::less || cmpResult == std::strong_ordering::equal);
    if (op == GT) result = (cmpResult == std::strong_ordering::greater);
    if (op == GE) result = (cmpResult == std::strong_ordering::greater || cmpResult == std::strong_ordering::equal);

    return P_BOOL(result);
}

static void tupleTraverse(TupleObject* tuple)
{
    for (auto o : tuple->items)
    {
        mark(o);
    }
}

static Object* tupleToString(Object* o)
{
    auto tupleObject = static_cast<TupleObject*>(o);
    std::stringstream str;
    str << "(";

    for (auto it = tupleObject->items.begin(); it != tupleObject->items.end(); it++)
    {
        if ((*it)->objectType == &stringObjectType)
        {
            str << "\"" << utils::escapeString((static_cast<StringObject*>(*it))->asUtf8()) << "\"";
        }
        else
        {
            auto stringObject = static_cast<StringObject*>((*it)->toString());
            str << stringObject->asUtf8();
        }

        if (it + 1 != tupleObject->items.end())
            str << ", ";
    }

    str << ")";
    return StringObject::create(str.str());
}

static Object* tupleToBool(Object* o)
{
    return P_BOOL(static_cast<TupleObject*>(o)->items.size());
}

static Object* tupleAdd(Object* o1, Object* o2)
{
    CHECK_TUPLE(o1);
    CHECK_TUPLE(o2);
    auto a = static_cast<TupleObject*>(o1);
    auto b = static_cast<TupleObject*>(o2);

    auto newTupleObject = TupleObject::create();

    newTupleObject->items.insert(
        newTupleObject->items.end(),
        a->items.begin(),
        a->items.end());

    newTupleObject->items.insert(
        newTupleObject->items.end(),
        b->items.begin(),
        b->items.end());

    newTupleObject->items.shrink_to_fit();
    return newTupleObject;
}

static Object* tupleGetIter(TupleObject* o)
{
    return TupleIterObject::create(o);
}

#define X_OBJECT_STRUCT TupleObject
#define X_OBJECT_TYPE vm::tupleObjectType

METHOD_TEMPLATE(tupleSize)
{
    OBJECT_CAST();
    return IntObject::create(o->items.size());
}
OBJECT_METHOD(tupleSize, "розмір", 0, false, nullptr);


METHOD_TEMPLATE(tupleFindItem)
{
    OBJECT_CAST();
    auto it = o->items.begin();
    for (; it != o->items.end(); it++)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) break;
    }

    size_t index = -1;
    if (it != o->items.end())
    {
        index = it - o->items.begin();
    }
    return IntObject::create(index);
}
OBJECT_METHOD(tupleFindItem, "знайти", 1, false, nullptr);


METHOD_TEMPLATE(tupleCount)
{
    OBJECT_CAST();
    int count = 0;

    for (auto& item : o->items)
    {
        auto cmp = item->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) count++;
    }

    return IntObject::create(count);
}
OBJECT_METHOD(tupleCount, "кількість", 1, false, nullptr);


METHOD_TEMPLATE(tupleContains)
{
    OBJECT_CAST();
    auto it = o->items.begin();
    for (; it != o->items.end(); ++it)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) break;
    }

    return P_BOOL(it != o->items.end());
}
OBJECT_METHOD(tupleContains, "містить", 1, false, nullptr);


METHOD_TEMPLATE(tupleGetItem)
{
    OBJECT_CAST();
    IntObject* index;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    return o->items[index->value];
}
OBJECT_METHOD(tupleGetItem, "отримати", 1, false, nullptr);


static DefaultParameters tupleSliceDefaults = {{
    {"кількість", &P_maxInt},
}};

METHOD_TEMPLATE(tupleSlice)
{
    OBJECT_CAST();
    IntObject *start, *count;
    ArgParser argParser{
        {&start, intObjectType, "початок"},
        {&count, intObjectType, "кількість"},
    };
    if (!argParser.parse(args, &tupleSliceDefaults, na)) return nullptr;
    if (count->value == 0) return &vm::P_emptyTuple;

    CHECK_NEGATIVE_INDEX(start->value, "початок")
    CHECK_NEGATIVE_INDEX(count->value, "кількість")
    CHECK_INDEX(start->value, o);

    i64 maxCount = std::min(count->value, static_cast<i64>(o->items.size() - start->value));
    auto slice = TupleObject::create();
    slice->items = std::vector<Object*>{
        o->items.begin() + start->value,
        o->items.begin() + start->value + maxCount
    };
    return slice;
}
OBJECT_METHOD(tupleSlice, "зріз", 1, false, &tupleSliceDefaults);

#undef X_OBJECT_STRUCT
#undef X_OBJECT_TYPE
#define X_OBJECT_STRUCT TupleIterObject
#define X_OBJECT_TYPE vm::tupleIterObjectType

static void tupleIterTraverse(TupleIterObject* o)
{
    mark(o->tuple);
}

METHOD_TEMPLATE(tupleIterNext)
{
    OBJECT_CAST();
    if (o->position < o->tuple->items.size())
    {
        return o->tuple->items[o->position++];
    }
    return &P_endIter;
}
OBJECT_METHOD(tupleIterNext, "наступний", 0, false, nullptr)

namespace vm
{
    TypeObject tupleObjectType =
    {
        .base = &objectObjectType,
        .name = "Кортеж",
        .size = sizeof(TupleObject),
        .alloc = DEFAULT_ALLOC(TupleObject),
        .dealloc = DEFAULT_DEALLOC(TupleObject),
        .callableInfo =
        {
            .arity = 0,
            .name = constructorName,
            .flags = CallableInfo::IS_VARIADIC | CallableInfo::IS_METHOD,
        },
        .constructor = tupleInit,
        .operators =
        {
            .toString = tupleToString,
            .toBool = tupleToBool,
            .add = tupleAdd,
            .getIter = (unaryFunction)tupleGetIter,
        },
        .comparison = tupleComparison,
        .traverse = (traverseFunction)tupleTraverse,
        .attributes =
        {
            METHOD_ATTRIBUTE(tupleSize),
            METHOD_ATTRIBUTE(tupleFindItem),
            METHOD_ATTRIBUTE(tupleCount),
            METHOD_ATTRIBUTE(tupleContains),
            METHOD_ATTRIBUTE(tupleGetItem),
            METHOD_ATTRIBUTE(tupleSlice),
        },
    };

    TupleObject P_emptyTuple = { {.objectType = &tupleObjectType}, {} };

    TypeObject tupleIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторКортежу",
        .size = sizeof(TupleIterObject),
        .alloc = DEFAULT_ALLOC(TupleIterObject),
        .traverse = (traverseFunction)tupleIterTraverse,
        .attributes =
        {
            METHOD_ATTRIBUTE(tupleIterNext),
        },
    };
}

TupleObject* vm::TupleObject::create()
{
    return static_cast<TupleObject*>(allocObject(&tupleObjectType));
}

TupleIterObject* vm::TupleIterObject::create(TupleObject* tuple)
{
    auto tupleIterObject = static_cast<TupleIterObject*>(allocObject(&tupleIterObjectType));
    tupleIterObject->tuple = tuple;
    return tupleIterObject;
}
