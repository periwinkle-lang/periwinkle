#include "string_object.h"
#include "native_function_object.h"
#include "exception_object.h"
#include "null_object.h"

using namespace vm;

static bool tryConvertToString(Object * o, std::string& str)
{
    if (o->objectType->operators->toString != NULL)
    {
        str = ((StringObject*)Object::toString(o))->value;
        return true;
    }
    return false;
}

// Конвертує об'єкт до StringObject, окрім випадку, коли об'єкт типу "нич"
#define TO_STRING(object, str)                           \
    if (object->objectType->type == ObjectTypes::STRING) \
        str = ((StringObject*)object)->value;            \
    else                                                 \
    {                                                    \
        if (object == &P_null                            \
            || tryConvertToString(object, str) == false) \
            return &P_NotImplemented;                    \
    }

static Object* strAdd(Object* o1, Object* o2)
{
    std::string a, b;
    TO_STRING(o1, a);
    TO_STRING(o2, b);
    return StringObject::create(a + b);
}

Object* allocStringObject();

namespace vm
{
    ObjectType stringObjectType =
    {
        .base = &objectObjectType,
        .name = "String",
        .type = ObjectTypes::STRING,
        .alloc = &allocStringObject,
        .operators = new ObjectOperators
        {
            .add = strAdd,
        },
    };
}

Object* allocStringObject()
{
    auto stringObject = new StringObject;
    stringObject->objectType = &stringObjectType;
    return (Object*)stringObject;
}

StringObject* vm::StringObject::create(std::string value)
{
    auto stringObject = (StringObject*)allocObject(&stringObjectType);
    stringObject->value = value;
    return stringObject;
}
