﻿#include <span>
#include <numeric>

#include "builtins.h"
#include "null_object.h"
#include "string_object.h"
#include "utils.h"
#include "array_object.h"

using namespace vm;

static std::string joinObjectString(
    const std::string& sep, const std::vector<Object*>& objects)
{
    if (objects.size())
    {
        auto& str = ((StringObject*)Object::toString(objects[0]))->value;
        std::string result = std::accumulate(
            ++objects.begin(), objects.end(), str,
            [sep](const std::string& a, Object* o)
            {
                auto& str = ((StringObject*)Object::toString(o))->value;
                return a + sep + str;
            });
        return result;
    }
    return "";
}

static Object* printNative(std::span<Object*> args, ArrayObject* va)
{
    auto str = joinObjectString(" ", va->items);
    std::cout << str << std::flush;
    return &P_null;
}

static Object* printLnNative(std::span<Object*> args, ArrayObject* va)
{
    auto str = joinObjectString(" ", va->items);
    std::cout << str << std::endl;
    return &P_null;
}

static Object* readLineNative(std::span<Object*> args, ArrayObject* va)
{
    std::string line = utils::readline();
    return StringObject::create(line);
}

static Object* createArray(std::span<Object*> args, ArrayObject* va)
{
    auto a = ArrayObject::create();
    a->items = va->items;
    return a;
}

builtin_t* vm::getBuiltin()
{
    static builtin_t* builtin;
    if (builtin == nullptr)
    {
        builtin = new builtin_t();
        builtin->insert(
        {
            {"друк", NativeFunctionObject::create(0, true, "друк", printNative)},
            {"друклн", NativeFunctionObject::create(0, true, "друклн", printLnNative)},
            {"зчитати", NativeFunctionObject::create(0, false, "зчитати", readLineNative)},
            {"Масив", NativeFunctionObject::create(0, true, "Масив", createArray)},
        }
        );
    }
    return builtin;
}
