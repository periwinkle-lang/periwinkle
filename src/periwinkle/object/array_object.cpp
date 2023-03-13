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

using namespace vm;
extern ObjectType objectObjectType;


#define CHECK_ARRAY(object)                             \
    if (object->objectType->type != ObjectTypes::ARRAY) \
        return &P_NotImplemented;

#define CHECK_INDEX(index, arrayObject)                               \
    if (std::cmp_greater_equal(index, arrayObject->items.size()))     \
    {                                                                 \
        VirtualMachine::currentVm->throwException(                    \
            &IndexErrorObjectType, "Індекс виходить за межі масиву"); \
    }


static Object* arrayToString(Object* o)
{
    auto arrayObject = (ArrayObject*)o;
    std::stringstream str;
    str << "[";

    for (auto it = arrayObject->items.begin(); it != arrayObject->items.end(); it++)
    {
        auto stringObject = (StringObject*)Object::toString(*it);
        str << stringObject->value;
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

static Object* arrayLength(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    return IntObject::create(arrayObject->items.size());
}

static Object* arrayPush(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    arrayObject->items.push_back(args[0]);
    return &P_null;
}

static Object* arrayInsert(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    auto index = ((IntObject*)args[0])->value;
    CHECK_INDEX(index, arrayObject);
    arrayObject->items.insert(arrayObject->items.begin() + index, args[1]);
    return &P_null;
}

static Object* arraySetItem(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    auto index = ((IntObject*)args[0])->value;
    CHECK_INDEX(index, arrayObject);
    arrayObject->items[index] = args[1];
    return &P_null;
}

static Object* arrayGetItem(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    auto index = ((IntObject*)args[0])->value;
    CHECK_INDEX(index, arrayObject);
    return arrayObject->items[index];
}

static Object* arrayFindItem(Object* o, std::span<Object*> args)
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

static Object* arrayClear(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    arrayObject->items.clear();
    return &P_null;
}

static Object* arrayCount(Object* o, std::span<Object*> args)
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

static Object* arrayCopy(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    auto newArrayObject = ArrayObject::create();
    newArrayObject->items = arrayObject->items;
    return newArrayObject;
}

static Object* arrayRemove(Object* o, std::span<Object*> args)
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

static Object* arrayRemoveAll(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    auto erased = std::erase_if(
        arrayObject->items,
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    return P_BOOL(erased);
}

static Object* arrayReverse(Object* o, std::span<Object*> args)
{
    auto arrayObject = (ArrayObject*)o;
    std::reverse(arrayObject->items.begin(), arrayObject->items.end());
    return &P_null;
}

Object* allocArrayObject();

namespace vm
{
    ObjectType arrayObjectType =
    {
        .base = &objectObjectType,
        .name = "Масив",
        .type = ObjectTypes::ARRAY,
        .alloc = &allocArrayObject,
        .operators =
        {
            .toString = arrayToString,
            .toBool = arrayToBool,
            .add = arrayAdd,
        },
        .attributes =
        {
            {"довжина",     NativeMethodObject::create(0, "довжина",     arrayLength)},
            {"додати",      NativeMethodObject::create(1, "додати",      arrayPush)},
            {"вставити",    NativeMethodObject::create(2, "вставити",    arrayInsert)},
            {"встановити",  NativeMethodObject::create(2, "встановити",  arraySetItem)},
            {"отримати",    NativeMethodObject::create(1, "отримати",    arrayGetItem)},
            {"знайти",      NativeMethodObject::create(1, "знайти",      arrayFindItem)},
            {"очистити",    NativeMethodObject::create(0, "очистити",    arrayClear)},
            {"кількість",   NativeMethodObject::create(1, "кількість",   arrayCount)},
            {"копія",       NativeMethodObject::create(0, "копія",       arrayCopy)},
            {"видалити",    NativeMethodObject::create(1, "видалити",    arrayRemove)},
            {"видалитиВсі", NativeMethodObject::create(1, "видалитиВсі", arrayRemoveAll)},
            {"обернути",    NativeMethodObject::create(0, "обернути",    arrayReverse)},
        },
    };
}

Object* allocArrayObject()
{
    auto arrayObject = new ArrayObject;
    arrayObject->objectType = &arrayObjectType;
    return (Object*)arrayObject;
}

ArrayObject* vm::ArrayObject::create()
{
    auto arrayObject = (ArrayObject*)allocObject(&arrayObjectType);
    return arrayObject;
}
