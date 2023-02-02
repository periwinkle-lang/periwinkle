#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <unordered_map>

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
        EXCEPTION,
    };

    using unaryFunction   = vm::Object*(*)(Object*);
    using binaryFunction  = vm::Object*(*)(Object*, Object*);
    using ternaryFunction = vm::Object*(*)(Object*, Object*, Object*);
    using callFunction    = vm::Object*(*)(Object*[]);

    struct ObjectOperators
    {
         callFunction call; // Виклик об'єкта
         unaryFunction toString;
         unaryFunction toInteger;
         unaryFunction toReal;
         unaryFunction toBool;
         binaryFunction add;
         binaryFunction sub;
         binaryFunction mul;
         binaryFunction div;
         binaryFunction floorDiv; // Ділення з округленням
         binaryFunction mod;
         unaryFunction inc; // Інкремент
         unaryFunction dec; // Декремент
         unaryFunction pos; // Унарний оператор +
         unaryFunction neg; // Заперечення
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
        binaryFunction comparisonOperators[6]; // Порядок операторів: eq, ne, gt, ge, lt, le
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
