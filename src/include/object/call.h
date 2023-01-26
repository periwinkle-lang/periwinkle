#ifndef CALL_H
#define CALL_H

#include "vm.h"
#include "object.h"

#define IS_CALLABLE(OBJECT) (OBJECT->objectType->type == vm::ObjectTypes::FUNCTION \
                            || OBJECT->objectType->type == vm::ObjectTypes::NATIVE_FUNCTION)

namespace vm
{
    Object* objectCall(Object* callable, Object** stack, WORD argc);
    Object* nativeFunctionCall(Object* callable, Object* args[]);
}

#endif
