#ifndef OBJECT_H
#define OBJECT_H

#include <string>
#include <unordered_map>
#include <vector>
#include <span>
#include <optional>

#include "types.hpp"

#define DEFAULT_ALLOC(objectStruct)        \
    []()                                   \
    {                                      \
        auto newObject = new objectStruct; \
        return (vm::Object*)newObject;     \
    }

#define DEFAULT_DEALLOC(objectStruct) \
    [](vm::Object* o)                 \
    {                                 \
        delete (objectStruct*)o;      \
    }

#define OBJECT_STATIC_METHOD(func, name, arity, variadic, defaults) \
    static const char* func##__methodName = name;                   \
    static NativeFunctionObject func##__methodImpl {                \
        func##__methodName, func, arity, variadic, defaults         \
    };

// Перед використанням потрібно визначити макрос X_OBJECT_TYPE
#define OBJECT_METHOD(method, name, arity, variadic, defaults) \
    static const char* method##__methodName = name;            \
    static NativeMethodObject method##__methodImpl {           \
        method##__methodName, method, &X_OBJECT_TYPE,          \
        arity, variadic, defaults                              \
    };

#define METHOD_TEMPLATE(name) \
    static Object* name(Object* _o, std::span<Object*> args, TupleObject* va, NamedArgs* na)

// Потрібно використовувати в парі з METHOD_TEMPLATE, в початку функції
// Перед використанням потрібно визначити макрос X_OBJECT_STRUCT
#define OBJECT_CAST() \
    X_OBJECT_STRUCT* o = static_cast<X_OBJECT_STRUCT*>(_o);

#define METHOD_ATTRIBUTE(method) { method##__methodName, &method##__methodImpl }
#define STATIC_METHOD_ATTRIBUTE(method) METHOD_ATTRIBUTE(method)

// Порівнює тип об'єкта з переданим типом
#define OBJECT_IS(object, type) ((object)->objectType == type)

namespace vm
{
    struct Object;
    struct TypeObject;
    struct NativeMethodObject;
    struct TupleObject;

    struct NamedArgs
    {
        std::span<std::string> names;
        std::vector<Object*> values;
        // Зберігає індекс параметра за замовчуванням, до якого належить аргумент
        // для викликаного об'єкта. Заповнюється за допомогою vm::validateCall,
        // після чого може бути використовується в vm::ArgParser.
        std::vector<size_t> indexes;
        u64 count;
    };

    struct DefaultParameters
    {
        using Pair = std::pair<std::string_view, Object*>;
        std::vector<Pair> parameters;
    };

    enum class ObjectCompOperator
    {
        EQ, NE, GT, GE, LT, LE
    };

    using unaryFunction      = vm::Object*(*)(Object*);
    using binaryFunction     = vm::Object*(*)(Object*, Object*);
    using ternaryFunction    = vm::Object*(*)(Object*, Object*, Object*);
    using callFunction       = vm::Object*(*)(Object*, std::span<Object*>, TupleObject*, NamedArgs*);
    using stackCallFunction  = vm::Object*(*)(Object*, Object**&);
    using allocFunction      = vm::Object*(*)(void);
    using deallocFunction    = void (*)(Object*);
    using comparisonFunction = vm::Object* (*)(Object*, Object*, ObjectCompOperator);
    using traverseFunction   = void (*)(Object*);

    struct ObjectOperators
    {
        callFunction call; // Виклик об'єкта, дані передаються через аргументи
        stackCallFunction stackCall; // Виклик об'єкта, дані передаються через стек
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

    enum class ObjectOperatorOffset : WORD
    {
        ADD = offsetof(ObjectOperators, add),
        SUB = offsetof(ObjectOperators, sub),
        MUL = offsetof(ObjectOperators, mul),
        DIV = offsetof(ObjectOperators, div),
        FLOOR_DIV = offsetof(ObjectOperators, floorDiv),
        MOD = offsetof(ObjectOperators, mod),
        POS = offsetof(ObjectOperators, pos),
        NEG = offsetof(ObjectOperators, neg),
        GET_ITER = offsetof(ObjectOperators, getIter),
    };

    extern TypeObject typeObjectType;
    extern TypeObject objectObjectType;

    struct Object
    {
        TypeObject* objectType = &typeObjectType;
        bool marked = false;

        // Викликає об'єкт
        Object* call(std::span<Object*>, NamedArgs* na=nullptr);

        // Викликає об'єкт, але аргмументи передаються через стек віртуальної машини.
        // Також очищає стек від аргументів
        Object* stackCall(Object**& sp, u64 argc, NamedArgs* na=nullptr);

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

        // Викликає операцію // для вхідних об'єктів
        Object* floorDiv(Object* o);

        // Викликає операцію % для вхідних об'єктів
        Object* mod(Object* o);

        // Викликає операцію унарного + для об'єкта
        Object* pos();

        // Викликає операцію унарного - для об'єкта
        Object* neg();

        // Повертає ітератор об'єкта
        Object* getIter();

        // Викликає унарний оператор для об'єкта
        Object* callUnaryOperator(vm::ObjectOperatorOffset offset);

        // Викликає бінарний оператор для об'єктів
        Object* callBinaryOperator(Object* other, vm::ObjectOperatorOffset offset);

        // Отримання атрибута за його іменем, якщо атрибут не знайдений, повертає nullptr
        Object* getAttr(const std::string& name);

        // Перетворює об'єкт в C++ bool, якщо була викинута помилка в ході виконання, то повертається std::nullopt
        std::optional<bool> asBool();
    };

    struct CallableInfo
    {
        WORD arity = 0;
        std::string name;
        DefaultParameters* defaults = nullptr;
        u8 flags = 0;

        enum Flags : u8
        {
            IS_VARIADIC = 1 << 0,
            IS_METHOD = 1 << 1,
            HAS_DEFAULTS = 1 << 2,
        };
    };

    struct TypeObject : Object
    {
        TypeObject* base; // Батьківський тип
        std::string name;
        u32 size = 0;
        size_t callableInfoOffset = 0; // offsetof для поля, де зберігається CallableInfo
        allocFunction alloc = nullptr; // Створення нового екземпляра
        deallocFunction dealloc = nullptr;
        CallableInfo callableInfo;
        callFunction constructor = nullptr; // Ініціалізація екземпляра
        NativeMethodObject* destructor = nullptr;
        ObjectOperators operators;
        comparisonFunction comparison = nullptr;

        // Повинно обходити всі об'єкти, на які посилається даний об'єкт.
        // Потрібно використати метод mark(Object*) до кожного об'єкту
        traverseFunction traverse = nullptr;

        // Зберігає методи та поля
        std::unordered_map<std::string, Object*> attributes;
    };

    // Використовується як callableName для конструкторів в TypeObject
    extern const char* constructorName;

    void mark(Object* o);
    Object* allocObject(TypeObject* objectType);
    bool isInstance(const Object* o, const TypeObject& type);

    // Перевіряє чи передана правильна кількість аргументів для виклику функції
    bool validateCall(Object* callable, WORD argc, vm::NamedArgs* namedArgs);
}

#endif
