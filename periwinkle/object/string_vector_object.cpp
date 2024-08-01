#include "string_vector_object.hpp"

using namespace vm;

namespace vm
{
    TypeObject stringVectorObjectType =
    {
        .base = nullptr,
        .name = "StringVectorObject",
        .size = sizeof(StringVectorObject),
        .alloc = DEFAULT_ALLOC(StringVectorObject),
        .dealloc = DEFAULT_DEALLOC(StringVectorObject),
    };
}

StringVectorObject* vm::StringVectorObject::create(const std::vector<std::string>& value)
{
    auto stringVectorObject = (StringVectorObject*)allocObject(&stringVectorObjectType);
    stringVectorObject->value = value;
    return stringVectorObject;
}
