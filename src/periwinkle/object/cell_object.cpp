#include "cell_object.h"

using namespace vm;

Object* allocCellObject();

namespace vm
{
    ObjectType cellObjectType =
    {
        .base = nullptr,
        .name = "Cell",
        .type = ObjectTypes::CELL,
        .alloc = &allocCellObject,
    };
}

Object* allocCellObject()
{
    auto cellObject = new CellObject;
    cellObject->objectType = &cellObjectType;
    return (Object*)cellObject;
}

CellObject* vm::CellObject::create(Object* value)
{
    auto cellObject = (CellObject*)allocObject(&cellObjectType);
    cellObject->value = value;
    return cellObject;
}
