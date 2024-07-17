#include "cell_object.hpp"

using namespace vm;

static void traverse(CellObject* o)
{
    mark(o->value);
}

namespace vm
{
    TypeObject cellObjectType =
    {
        .base = nullptr,
        .name = "Cell",
        .size = sizeof(CellObject),
        .alloc = DEFAULT_ALLOC(CellObject),
        .dealloc = DEFAULT_DEALLOC(CellObject),
        .traverse = (traverseFunction)traverse,
    };
}

CellObject* vm::CellObject::create(Object* value)
{
    auto cellObject = (CellObject*)allocObject(&cellObjectType);
    cellObject->value = value;
    return cellObject;
}
