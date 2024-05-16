#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <unordered_map>
#include <vector>

#include "types.hpp"

#define DEFAULT_ALLOC(object)          \
    []()                               \
    {                                  \
        auto newObject = new object;   \
        return (vm::Object*)newObject; \
    }

#define METHOD_TEMPLATE(name, object) \
    static Object* name(object* o, std::span<Object*> args, ListObject* va, NamedArgs* na)

// Порівнює тип об'єкта з переданим типом
#define OBJECT_IS(object, type) ((object)->objectType == type)

namespace vm
{
    struct Object;
    struct TypeObject;
    struct NativeMethodObject;
    struct ListObject;

    struct NamedArgs
    {
        std::vector<std::string>* names;
        std::vector<Object*> values;
        u64 count;
    };

    struct DefaultParameters
    {
        std::vector<std::string> names;
        std::vector<Object*> values;
    };

    enum class ObjectCompOperator
    {
        EQ, NE, GT, GE, LT, LE
    };

    using unaryFunction      = vm::Object*(*)(Object*);
    using binaryFunction     = vm::Object*(*)(Object*, Object*);
    using ternaryFunction    = vm::Object*(*)(Object*, Object*, Object*);
    using callFunction       = vm::Object*(*)(Object*, Object**&, u64, NamedArgs*);
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
        Object* call(Object**& sp, u64 argc, NamedArgs* namedArgs=nullptr);

        // Викликає операції порівяння для вхідних об'єктів
        Object* compare(Object* o, ObjectCompOperator op);

        // Приведення об'єкту до типу StringObject
        Object* toString();

        // Приведення об'єкту до типу IntObject
        Object* toInteger();

        // Приведення об'єкту до типу RealObject
        Object* toReal();

        // Приведення об'єкту до типу BoolObject
        Object* toBool();

        // Викликає операцію + для вхідних об'єктів
        Object* add(Object* o);

        // Викликає операцію - для вхідних об'єктів
        Object* sub(Object* o);

        // Викликає операцію * для вхідних об'єктів
        Object* mul(Object* o);

        // Викликає операцію / для вхідних об'єктів
        Object* div(Object* o);

        // Викликає операцію \ для вхідних об'єктів
        Object* floorDiv(Object* o);

        // Викликає операцію % для вхідних об'єктів
        Object* mod(Object* o);

        // Викликає операцію унарного + для об'єкта
        Object* pos();

        // Викликає операцію унарного - для об'єкта
        Object* neg();

        // Повертає ітератор об'єкта
        Object* getIter();

        // Отримання атрибута за його іменем, якщо атрибут не знайдений, повертає nullptr
        Object* getAttr(const std::string& name);
    };

    struct TypeObject : Object
    {
        TypeObject* base; // Батьківський тип
        std::string name;
        Object* (*alloc)(void); // Створення нового екземпляра
        NativeMethodObject* constructor; // Ініціалізація екземпляра
        NativeMethodObject* descructor;
        ObjectOperators operators;
        comparisonFunction comparison;
        // Зберігає методи та поля
        std::unordered_map<std::string, Object*> attributes;
    };

    Object* allocObject(TypeObject* objectType);
    bool isInstance(const Object* o, const TypeObject& type);
    bool objectToBool(Object* o);
}

#endif
