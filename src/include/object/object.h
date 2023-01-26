#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <unordered_map>
#include <cstddef>

// object - вказівник на об'єкт
// op - член перечислення Operator
#define GET_OPERATOR(object, op) (object)->objectType->operators->op

namespace vm
{
    struct Object;
    using comparisonFunction = Object * (*)(Object* self, Object* other);

    enum class ObjectTypes
    {
        CODE,
        OBJECT,
        FUNCTION,
        NATIVE_FUNCTION,
        INTEGER,
        BOOL,
        STRING,
        REAL,
        NULL_,
    };

    struct ComparisonOperators
    {
        comparisonFunction eq, ne, gt, ge, lt, le;
    };

    enum class ComparisonOperator : size_t
    {
        EQ = offsetof(ComparisonOperators, eq),
        NE = offsetof(ComparisonOperators, ne),
        GT = offsetof(ComparisonOperators, gt),
        GE = offsetof(ComparisonOperators, ge),
        LT = offsetof(ComparisonOperators, lt),
        LE = offsetof(ComparisonOperators, le),
    };

    struct NativeFunctionObject;

    struct ObjectOperators
    {
        NativeFunctionObject* call; // Виклик об'єкта
        NativeFunctionObject* toString;
        NativeFunctionObject* toInteger;
        NativeFunctionObject* toReal;
        NativeFunctionObject* toBool;
        NativeFunctionObject* add;
        NativeFunctionObject* sub;
        NativeFunctionObject* rSub; // Так як віднімання не є комутативним, потрібна "права" реалізація
        NativeFunctionObject* mul;
        NativeFunctionObject* div;
        NativeFunctionObject* rDiv; // Ділення теж не є комутативним
        NativeFunctionObject* mod;
        NativeFunctionObject* rMod; // Ділення по модулю теж не є комутативним
        NativeFunctionObject* inc; // Інкремент
        NativeFunctionObject* dec; // Декремент
        NativeFunctionObject* neg; // Заперечення
    };

    struct ObjectType
    {
        ObjectType* base; // Батьківський тип
        std::string name;
        ObjectTypes type;
        Object* (*alloc)(void); // Створення нового екземпляра
        void (*constructor)(Object* object, Object* args[]); // Ініціалізація екземпляра
        void (*destructor)(Object* object);
        ObjectOperators* operators;
        ComparisonOperators* comparsionOperators;
        // Зберігає методи, константи та статичні поля
        std::unordered_map<std::string, Object*> *publicAttributes;
        std::unordered_map<std::string, Object*> *privateAttributes;
    };

    extern ObjectType objectObjectType;

    struct Object
    {
        ObjectType* objectType;
    };

    Object* allocObject(ObjectType const *objectType);
    inline std::string objectTypeToString(const ObjectType *type);
    inline Object* getPublicAttribute(Object* object, std::string name);
    inline Object* getPrivateAttribute(Object* object, std::string name);
}

#endif
