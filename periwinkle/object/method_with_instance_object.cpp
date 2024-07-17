#include "method_with_instance_object.hpp"

using namespace vm;

static void traverse(MethodWithInstanceObject* o)
{
    mark(o->instance);
    mark(o->callable);
}

namespace vm
{
    TypeObject methodWithInstanceObjectType =
    {
        .base = nullptr,
        .name = "MethodWithInstance",
        .size = sizeof(MethodWithInstanceObject),
        .alloc = DEFAULT_ALLOC(MethodWithInstanceObject),
        .traverse = (traverseFunction)traverse,
    };
}

MethodWithInstanceObject* vm::MethodWithInstanceObject::create(
    Object* instance, Object* callable)
{
    auto o = (MethodWithInstanceObject*)allocObject(&methodWithInstanceObjectType);
    o->instance = instance;
    o->callable = callable;
    return o;
}
