#include "method_with_instance_object.h"

using namespace vm;

namespace vm
{
    TypeObject methodWithInstanceObjectType =
    {
        .base = nullptr,
        .name = "MethodWithInstance",
        .type = ObjectTypes::METHOD_WITH_INSTANCE,
        .alloc = DEFAULT_ALLOC(MethodWithInstanceObject),
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
