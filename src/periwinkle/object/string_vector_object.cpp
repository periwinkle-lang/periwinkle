#include "string_vector_object.h"

using namespace vm;

namespace vm
{
    TypeObject stringVectorObjectType =
    {
        .base = nullptr,
        .name = "StringVectorObject",
        .alloc = DEFAULT_ALLOC(StringVectorObject),
    };
}

StringVectorObject* vm::StringVectorObject::create(const std::vector<std::string>& value)
{
    auto stringVectorObject = (StringVectorObject*)allocObject(&stringVectorObjectType);
    stringVectorObject->value = value;
    return stringVectorObject;
}
