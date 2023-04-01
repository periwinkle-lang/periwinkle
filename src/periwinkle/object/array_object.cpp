﻿#include <sstream>
#include <algorithm>
#include <utility>

#include "array_object.h"
#include "string_object.h"
#include "bool_object.h"
#include "exception_object.h"
#include "int_object.h"
#include "null_object.h"
#include "native_method_object.h"
#include "end_iteration_object.h"
#include "argument_parser.h"

using namespace vm;


#define CHECK_ARRAY(object)                             \
    if (object->objectType->type != ObjectTypes::ARRAY) \
        return &P_NotImplemented;

#define CHECK_INDEX(index, arrayObject)                               \
    if (std::cmp_greater_equal(index, arrayObject->items.size()))     \
    {                                                                 \
        VirtualMachine::currentVm->throwException(                    \
            &IndexErrorObjectType, "Індекс виходить за межі масиву"); \
    }


static Object* arrayInit(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = ArrayObject::create();
    arrayObject->items = va->items;
    return arrayObject;
}

static Object* arrayToString(Object* o)
{
    auto arrayObject = (ArrayObject*)o;
    std::stringstream str;
    str << "[";

    for (auto it = arrayObject->items.begin(); it != arrayObject->items.end(); it++)
    {
        auto stringObject = (StringObject*)Object::toString(*it);
        str << stringObject->asUtf8();
        if (it + 1 != arrayObject->items.end())
            str << ", ";
    }

    str << "]";
    return StringObject::create(str.str());
}

static Object* arrayToBool(Object* o)
{
    auto arrayObject = (ArrayObject*)o;
    return P_BOOL(arrayObject->items.size());
}

static Object* arrayAdd(Object* o1, Object* o2)
{
    CHECK_ARRAY(o1);
    CHECK_ARRAY(o2);
    auto arrayObject1 = (ArrayObject*)o1;
    auto arrayObject2 = (ArrayObject*)o2;

    auto newArrayObject = ArrayObject::create();

    newArrayObject->items.insert(
        newArrayObject->items.end(),
        arrayObject1->items.begin(),
        arrayObject1->items.end());

    newArrayObject->items.insert(
        newArrayObject->items.end(),
        arrayObject2->items.begin(),
        arrayObject2->items.end());

    return newArrayObject;
}

static Object* arrayGetIter(ArrayObject* o)
{
    auto iterator = ArrayIterObject::create(o->items);
    return iterator;
}

static Object* arrayLength(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    return IntObject::create(arrayObject->items.size());
}

static Object* arrayPush(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    arrayObject->items.push_back(args[0]);
    return &P_null;
}

static Object* arrayInsert(Object* o, std::span<Object*> args, ArrayObject* va)
{
    IntObject* index;
    Object* element;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    argParser.parse(args);

    auto arrayObject = (ArrayObject*)o;
    CHECK_INDEX(index->value, arrayObject);
    arrayObject->items.insert(arrayObject->items.begin() + index->value, element);
    return &P_null;
}

static Object* arraySetItem(Object* o, std::span<Object*> args, ArrayObject* va)
{
    IntObject* index;
    Object* element;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    argParser.parse(args);

    auto arrayObject = (ArrayObject*)o;
    CHECK_INDEX(index->value, arrayObject);
    arrayObject->items[index->value] = element;
    return &P_null;
}

static Object* arrayGetItem(Object* o, std::span<Object*> args, ArrayObject* va)
{
    IntObject* index;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    argParser.parse(args);

    auto arrayObject = (ArrayObject*)o;
    CHECK_INDEX(index->value, arrayObject);
    return arrayObject->items[index->value];
}

static Object* arrayFindItem(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;

    auto it = std::find_if(
        arrayObject->items.begin(),
        arrayObject->items.end(),
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    size_t index = -1;
    if (it != arrayObject->items.end())
    {
        index = it - arrayObject->items.begin();
    }
    return IntObject::create(index);
}

static Object* arrayClear(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    arrayObject->items.clear();
    return &P_null;
}

static Object* arrayCount(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    auto count = std::count_if(
        arrayObject->items.begin(),
        arrayObject->items.end(),
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );
    return IntObject::create(count);
}

static Object* arrayCopy(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    auto newArrayObject = ArrayObject::create();
    newArrayObject->items = arrayObject->items;
    return newArrayObject;
}

static Object* arrayRemove(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;

    auto it = std::find_if(
        arrayObject->items.begin(),
        arrayObject->items.end(),
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    if (it != arrayObject->items.end())
    {
        arrayObject->items.erase(it);
        return &P_true;
    }
    return &P_false;
}

static Object* arrayRemoveAll(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    auto erased = std::erase_if(
        arrayObject->items,
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    return P_BOOL(erased);
}

static Object* arrayReverse(Object* o, std::span<Object*> args, ArrayObject* va)
{
    auto arrayObject = (ArrayObject*)o;
    std::reverse(arrayObject->items.begin(), arrayObject->items.end());
    return &P_null;
}

static Object* arrayIterNext(ArrayIterObject* o, std::span<Object*> args, ArrayObject* va)
{
    if (o->position < o->length)
    {
        return o->iterable[o->position++];
    }
    return &P_endIter;
}

namespace vm
{
    TypeObject arrayObjectType =
    {
        .base = &objectObjectType,
        .name = "Масив",
        .type = ObjectTypes::ARRAY,
        .alloc = DEFAULT_ALLOC(ArrayObject),
        .constructor = new NATIVE_METHOD("конструктор", 0, true, arrayInit, arrayObjectType),
        .operators =
        {
            .toString = arrayToString,
            .toBool = arrayToBool,
            .add = arrayAdd,
            .getIter = (unaryFunction)arrayGetIter,
        },
        .attributes =
        {
            OBJECT_METHOD("довжина",    0, false, arrayLength,    arrayObjectType),
            OBJECT_METHOD("додати",     1, false, arrayPush,      arrayObjectType),
            OBJECT_METHOD("вставити",   2, false, arrayInsert,    arrayObjectType),
            OBJECT_METHOD("встановити", 2, false, arraySetItem,   arrayObjectType),
            OBJECT_METHOD("отримати",   1, false, arrayGetItem,   arrayObjectType),
            OBJECT_METHOD("знайти",     1, false, arrayFindItem,  arrayObjectType),
            OBJECT_METHOD("очистити",   0, false, arrayClear,     arrayObjectType),
            OBJECT_METHOD("кількість",  1, false, arrayCount,     arrayObjectType),
            OBJECT_METHOD("копія",      0, false, arrayCopy,      arrayObjectType),
            OBJECT_METHOD("видалити",   1, false, arrayRemove,    arrayObjectType),
            OBJECT_METHOD("видалитиВсі",1, false, arrayRemoveAll, arrayObjectType),
            OBJECT_METHOD("обернути",   0, false, arrayReverse,   arrayObjectType),
        },
    };

    TypeObject arrayIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторМасиву",
        .type = ObjectTypes::ARRAY_ITERATOR,
        .alloc = DEFAULT_ALLOC(ArrayIterObject),
        .attributes =
        {
            OBJECT_METHOD("наступний", 0, false, (nativeMethod)arrayIterNext, arrayIterObjectType),
        },
    };
}

ArrayObject* vm::ArrayObject::create()
{
    auto arrayObject = (ArrayObject*)allocObject(&arrayObjectType);
    return arrayObject;
}

ArrayIterObject* vm::ArrayIterObject::create(const std::vector<Object*> iterable)
{
    auto arrayIterObject = (ArrayIterObject*)allocObject(&arrayIterObjectType);
    arrayIterObject->iterable = iterable;
    arrayIterObject->length = iterable.size();
    return arrayIterObject;
}
