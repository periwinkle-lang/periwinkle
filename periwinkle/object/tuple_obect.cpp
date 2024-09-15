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

static Object* tupleInit(Object* o, std::span<Object*> args, TupleObject* va, NamedArgs* na)
{
    return va;
}

static inline bool tupleObjectEqual(TupleObject* a, TupleObject* b, bool notEqual=false)
{
    if (a->items.size() != b->items.size())
    {
        return false;
    }

    for (size_t i = 0; i < a->items.size(); ++i)
    {
        if ((static_cast<BoolObject*>(a->items[i]->compare(b->items[i],
            notEqual ? ObjectCompOperator::NE : ObjectCompOperator::EQ)))->value == false)
        {
            return false;
        }
    }

    return true;
}

static Object* tupleComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    CHECK_TUPLE(o1);
    CHECK_TUPLE(o2);
    auto a = static_cast<TupleObject*>(o1);
    auto b = static_cast<TupleObject*>(o2);
    bool result;

    using enum ObjectCompOperator;
    switch (op)
    {
    case EQ: result = tupleObjectEqual(a, b);       break;
    case NE: result = tupleObjectEqual(a, b, true); break;
    default:
        return &P_NotImplemented;
    }

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
    auto it = std::find_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)obj->compare(
            args[0], ObjectCompOperator::EQ))->value; }
    );

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
    auto count = std::count_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)obj->compare(
            args[0], ObjectCompOperator::EQ))->value; }
    );
    return IntObject::create(count);
}
OBJECT_METHOD(tupleCount, "кількість", 1, false, nullptr);


METHOD_TEMPLATE(tupleContains)
{
    OBJECT_CAST();
    auto it = std::find_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)obj->compare(
            args[0], ObjectCompOperator::EQ))->value; }
    );

    i64 index = -1;
    if (it != o->items.end())
    {
        index = it - o->items.begin();
    }

    return P_BOOL(index != -1);
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


METHOD_TEMPLATE(tupleSlice)
{
    OBJECT_CAST();
    IntObject *start, *count;
    ArgParser argParser{
        {&start, intObjectType, "початок"},
        {&count, intObjectType, "кількість"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(start->value, o);
    CHECK_INDEX(start->value + count->value - (count->value == 0 ? 0 : 1), o);
    auto slice = TupleObject::create();
    slice->items = std::vector<Object*>{
        o->items.begin() + start->value, o->items.begin() + count->value };
    return slice;
}
OBJECT_METHOD(tupleSlice, "зріз", 2, false, nullptr);

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
