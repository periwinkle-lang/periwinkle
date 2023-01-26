#include "object.h"
#include "bool_object.h"

using namespace vm;

Object* eq(Object *self, Object *other)
{
    bool result = self == other;
    return BoolObject::create(result);
}

Object* ne(Object *self, Object *other)
{
    bool result = self != other;
    return BoolObject::create(result);
}

namespace vm
{
    ObjectType objectObjectType =
    {
        .base = nullptr, // Object - базовий тип для всіх типів, і тому ні від кого не наслідується
        .name = "Object",
        .type = ObjectTypes::OBJECT,
        .comparsionOperators = new ComparisonOperators
        {
            .eq = &eq,
            .ne = &ne,
        },
    };
}

Object* vm::allocObject(ObjectType const *objectType)
{
    return objectType->alloc();
}

std::string vm::objectTypeToString(const ObjectType *type)
{
    return type->name;
}

Object* vm::getPublicAttribute(Object *object, std::string name)
{
    auto attribute = object->objectType->publicAttributes->find(name);
    if (attribute != object->objectType->publicAttributes->end())
    {
        return attribute->second;
    }
    return nullptr;
}

Object* vm::getPrivateAttribute(Object *object, std::string name)
{
    auto attribute = object->objectType->privateAttributes->find(name);
    if (attribute != object->objectType->privateAttributes->end())
    {
        return attribute->second;
    }
    return nullptr;
}
