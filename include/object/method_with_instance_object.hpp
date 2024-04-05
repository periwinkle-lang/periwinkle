#ifndef METHOD_WITH_INSTANCE_OBJECT_H
#define METHOD_WITH_INSTANCE_OBJECT_H

#include "object.hpp"

namespace vm
{
    extern TypeObject methodWithInstanceObjectType;

    // Допоміжний об'єкт, що містить екземпляр та метод для його виклику
    struct MethodWithInstanceObject : Object
    {
        Object* instance;
        Object* callable;

        static MethodWithInstanceObject* create(Object* instance, Object* callable);
    };
}

#endif
