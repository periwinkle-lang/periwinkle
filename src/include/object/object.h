#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <unordered_map>

#include "types.h"

#define DEFAULT_ALLOC(object)          \
    []()                               \
    {                                  \
        auto newObject = new object;   \
        return (vm::Object*)newObject; \
    }

namespace vm
{
    struct Object;
    struct TypeObject;
    struct NativeMethodObject;

    enum class ObjectTypes
    {
        OBJECT,
        TYPE,
        CODE,
        FUNCTION,
        NATIVE_FUNCTION,
        NATIVE_METHOD,
        INTEGER,
        BOOL,
        STRING, STRING_ITERATOR,
        REAL,
        NULL_,
        EXCEPTION,
        CELL,
        ARRAY, ARRAY_ITERATOR,
        END_ITERATION,
    };

    enum class ObjectCompOperator
    {
        EQ, NE, GT, GE, LT, LE
    };

    using unaryFunction      = vm::Object*(*)(Object*);
    using binaryFunction     = vm::Object*(*)(Object*, Object*);
    using ternaryFunction    = vm::Object*(*)(Object*, Object*, Object*);
    using callFunction       = vm::Object*(*)(Object*, Object**&, u64);
    using comparisonFunction = vm::Object*(*)(Object*, Object*, ObjectCompOperator);

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
         unaryFunction pos; // Унарний оператор +
         unaryFunction neg; // Заперечення
         unaryFunction getIter; // Повертає ітератор
    };

    extern TypeObject typeObjectType;
    extern TypeObject objectObjectType;

    struct Object
    {
        TypeObject* objectType = &typeObjectType;

        // Викликає об'єкт
        static Object* call(Object* callable, Object**& sp, u64 argc);

        // Викликає операції порівяння для вхідних об'єктів
        static Object* compare(Object* o1, Object* o2, ObjectCompOperator op);

        // Приведення об'єкту до типу StringObject
        static Object* toString(Object* o);

        // Приведення об'єкту до типу IntObject
        static Object* toInteger(Object* o);

        // Приведення об'єкту до типу RealObject
        static Object* toReal(Object* o);

        // Приведення об'єкту до типу BoolObject
        static Object* toBool(Object* o);

        // Викликає операцію + для вхідних об'єктів
        static Object* add(Object* o1, Object* o2);

        // Викликає операцію - для вхідних об'єктів
        static Object* sub(Object* o1, Object* o2);

        // Викликає операцію * для вхідних об'єктів
        static Object* mul(Object* o1, Object* o2);

        // Викликає операцію / для вхідних об'єктів
        static Object* div(Object* o1, Object* o2);

        // Викликає операцію \ для вхідних об'єктів
        static Object* floorDiv(Object* o1, Object* o2);

        // Викликає операцію % для вхідних об'єктів
        static Object* mod(Object* o1, Object* o2);

        // Викликає операцію унарного + для об'єкта
        static Object* pos(Object* o);

        // Викликає операцію унарного - для об'єкта
        static Object* neg(Object* o);

        // Повертає ітератор об'єкта
        static Object* getIter(Object* o);

        static Object* getAttr(Object* o, const std::string& name);
    };

    struct TypeObject : Object
    {
        TypeObject* base; // Батьківський тип
        std::string name;
        ObjectTypes type;
        Object* (*alloc)(void); // Створення нового екземпляра
        NativeMethodObject* constructor; // Ініціалізація екземпляра
        NativeMethodObject* descructor;
        ObjectOperators operators;
        comparisonFunction comparison;
        // Зберігає методи та поля
        std::unordered_map<std::string, Object*> attributes;
    };

    Object* allocObject(TypeObject* objectType);
    inline std::string objectTypeToString(const TypeObject* type);
}

#endif
