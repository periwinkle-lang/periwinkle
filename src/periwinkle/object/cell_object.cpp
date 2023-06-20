#include "cell_object.h"

using namespace vm;

namespace vm
{
    TypeObject cellObjectType =
    {
        .base = nullptr,
        .name = "Cell",
        .alloc = DEFAULT_ALLOC(CellObject),
    };
}

CellObject* vm::CellObject::create(Object* value)
{
    auto cellObject = (CellObject*)allocObject(&cellObjectType);
    cellObject->value = value;
    return cellObject;
}
