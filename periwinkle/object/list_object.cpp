#include <sstream>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <format>

#include "object.hpp"
#include "list_object.hpp"
#include "string_object.hpp"
#include "bool_object.hpp"
#include "exception_object.hpp"
#include "int_object.hpp"
#include "null_object.hpp"
#include "native_method_object.hpp"
#include "end_iteration_object.hpp"
#include "tuple_object.hpp"
#include "argument_parser.hpp"
#include "utils.hpp"
#include "periwinkle.hpp"

using namespace vm;

#define CHECK_LIST(object)                     \
    if (OBJECT_IS(object, &listObjectType) == false) \
        return &P_NotImplemented;

#define CHECK_INDEX(index, listObject)                                \
    if (std::cmp_greater_equal(index, listObject->items.size()))      \
    {                                                                 \
        getCurrentState()->setException(                              \
            &IndexErrorObjectType, "Індекс виходить за межі списку"); \
        return nullptr;                                               \
    }

#define CHECK_NEGATIVE_INDEX(index, variableName)                       \
    if (index < 0LL)                                                    \
    {                                                                   \
        getCurrentState()->setException(                                \
            &IndexErrorObjectType,                                      \
            "\"" variableName "\" не може приймати від'ємних значень"); \
        return nullptr;                                                 \
    }

static Object* listInit(Object* o, std::span<Object*> args, TupleObject* va, NamedArgs* na)
{
    auto listObject = ListObject::create();
    listObject->items = va->items;
    return listObject;
}

static inline std::optional<bool> listObjectEqual(ListObject* a, ListObject* b)
{
    if (a->items.size() != b->items.size()) return false;

    for (size_t i = 0; i < a->items.size(); ++i)
    {
        auto cmpResult = a->items[i]->compare(b->items[i], ObjectCompOperator::EQ);
        if (cmpResult == nullptr) return std::nullopt;
        auto result = cmpResult->asBool();
        if (!result) return std::nullopt;
        if (result.value() == false)
            return false;
    }

    return true;
}

static Object* listComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    CHECK_LIST(o1);
    CHECK_LIST(o2);
    auto a = static_cast<ListObject*>(o1);
    auto b = static_cast<ListObject*>(o2);

    using enum ObjectCompOperator;

    if (op == EQ) return P_BOOL(listObjectEqual(a, b));
    if (op == NE) return P_BOOL(!listObjectEqual(a, b));

    auto cmpResult = std::strong_ordering::equal;
    try
    {
        cmpResult = std::lexicographical_compare_three_way(
            a->items.begin(), a->items.end(),
            b->items.begin(), b->items.end(),
            [](Object* obj1, Object* obj2) {
                auto cmpLess = obj1->compare(obj2, ObjectCompOperator::LT);
                if (cmpLess == nullptr) throw std::runtime_error("");
                auto less = cmpLess->asBool();
                if (!less) throw std::runtime_error("");
                if (less.value())
                    return std::strong_ordering::less;

                auto cmpGreater = obj1->compare(obj2, ObjectCompOperator::GT);
                if (cmpGreater == nullptr) throw std::runtime_error("");
                auto greater = cmpGreater->asBool();
                if (!greater) throw std::runtime_error("");
                if (greater.value())
                    return std::strong_ordering::greater;

                return std::strong_ordering::equal;
            }
        );
    }
    catch (const std::runtime_error& e)
    {
        return nullptr;
    }

    bool result;
    if (op == LT) result = (cmpResult == std::strong_ordering::less);
    if (op == LE) result = (cmpResult == std::strong_ordering::less || cmpResult == std::strong_ordering::equal);
    if (op == GT) result = (cmpResult == std::strong_ordering::greater);
    if (op == GE) result = (cmpResult == std::strong_ordering::greater || cmpResult == std::strong_ordering::equal);

    return P_BOOL(result);
}

static void listTraverse(ListObject* list)
{
    for (auto o : list->items)
    {
        mark(o);
    }
}

static Object* listToString(Object* o)
{
    auto listObject = (ListObject*)o;
    std::stringstream str;
    str << "[";

    for (auto it = listObject->items.begin(); it != listObject->items.end(); it++)
    {
        if ((*it)->objectType == &stringObjectType)
        {
            str << "\"" << utils::escapeString(((StringObject*)*it)->asUtf8()) << "\"";
        }
        else
        {
            auto stringObject = (StringObject*)(*it)->toString();
            str << stringObject->asUtf8();
        }

        if (it + 1 != listObject->items.end())
            str << ", ";
    }

    str << "]";
    return StringObject::create(str.str());
}

static Object* listToBool(Object* o)
{
    auto listObject = (ListObject*)o;
    return P_BOOL(listObject->items.size());
}

static Object* listAdd(Object* o1, Object* o2)
{
    CHECK_LIST(o1);
    CHECK_LIST(o2);
    auto listObject1 = (ListObject*)o1;
    auto listObject2 = (ListObject*)o2;

    auto newListObject = ListObject::create();

    newListObject->items.insert(
        newListObject->items.end(),
        listObject1->items.begin(),
        listObject1->items.end());

    newListObject->items.insert(
        newListObject->items.end(),
        listObject2->items.begin(),
        listObject2->items.end());

    return newListObject;
}

static Object* listGetIter(ListObject* o)
{
    auto iterator = ListIterObject::create(o->items);
    return iterator;
}

#define X_OBJECT_STRUCT ListObject
#define X_OBJECT_TYPE vm::listObjectType

METHOD_TEMPLATE(listRemove)
{
    OBJECT_CAST();
    auto it = o->items.begin();
    for (; it != o->items.end(); it++)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) break;
    }

    if (it != o->items.end())
    {
        o->items.erase(it);
        return &P_true;
    }
    return &P_false;
}
OBJECT_METHOD(listRemove, "видалити", 1, false, nullptr)


METHOD_TEMPLATE(listRemoveAll)
{
    OBJECT_CAST();
    bool anyErased = false;

    auto it = o->items.begin();
    while (it != o->items.end())
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;

        if (b.value())
        {
            it = o->items.erase(it);
            anyErased = true;
        }
        else
        {
            ++it;
        }
    }

    return P_BOOL(anyErased);
}
OBJECT_METHOD(listRemoveAll, "видалитиВсі", 1, false, nullptr)


METHOD_TEMPLATE(listInsert)
{
    OBJECT_CAST();
    IntObject* index;
    Object* element;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    o->items.insert(o->items.begin() + index->value, element);
    return &P_null;
}
OBJECT_METHOD(listInsert, "вставити", 2, false, nullptr);


METHOD_TEMPLATE(listSetItem)
{
    OBJECT_CAST();
    IntObject* index;
    Object* element;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    o->items[index->value] = element;
    return &P_null;
}
OBJECT_METHOD(listSetItem, "встановити", 2, false, nullptr);


METHOD_TEMPLATE(listSize)
{
    OBJECT_CAST();
    return IntObject::create(o->items.size());
}
OBJECT_METHOD(listSize, "розмір", 0, false, nullptr);


METHOD_TEMPLATE(listPush)
{
    OBJECT_CAST();
    o->items.push_back(args[0]);
    return &P_null;
}
OBJECT_METHOD(listPush, "додати", 1, false, nullptr);


METHOD_TEMPLATE(listReplace)
{
    OBJECT_CAST();
    size_t replaceCount = 0;

    for (auto it = o->items.begin(); it != o->items.end(); ++it)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value())
        {
            *it = args[1];
            ++replaceCount;
        }
    }

    return P_BOOL(replaceCount);
}
OBJECT_METHOD(listReplace, "замінити", 2, false, nullptr);


METHOD_TEMPLATE(listFindItem)
{
    OBJECT_CAST();
    auto it = o->items.begin();
    for (; it != o->items.end(); it++)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) break;
    }

    size_t index = -1;
    if (it != o->items.end())
    {
        index = it - o->items.begin();
    }
    return IntObject::create(index);
}
OBJECT_METHOD(listFindItem, "знайти", 1, false, nullptr);


METHOD_TEMPLATE(listCopy)
{
    OBJECT_CAST();
    auto newListObject = ListObject::create();
    newListObject->items = o->items;
    return newListObject;
}
OBJECT_METHOD(listCopy, "копія", 0, false, nullptr);


METHOD_TEMPLATE(listCount)
{
    OBJECT_CAST();
    int count = 0;

    for (auto& item : o->items)
    {
        auto cmp = item->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) count++;
    }

    return IntObject::create(count);
}
OBJECT_METHOD(listCount, "кількість", 1, false, nullptr);


METHOD_TEMPLATE(listContains)
{
    OBJECT_CAST();
    auto it = o->items.begin();
    for (; it != o->items.end(); ++it)
    {
        auto cmp = (*it)->compare(args[0], ObjectCompOperator::EQ);
        if (cmp == nullptr) return nullptr;
        auto b = cmp->asBool();
        if (!b) return nullptr;
        if (b.value()) break;
    }

    return P_BOOL(it != o->items.end());
}
OBJECT_METHOD(listContains, "містить", 1, false, nullptr);


METHOD_TEMPLATE(listReverse)
{
    OBJECT_CAST();
    std::reverse(o->items.begin(), o->items.end());
    return &P_null;
}
OBJECT_METHOD(listReverse, "обернути", 0, false, nullptr);


METHOD_TEMPLATE(listGetItem)
{
    OBJECT_CAST();
    IntObject* index;
    ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    if (!argParser.parse(args)) return nullptr;

    CHECK_INDEX(index->value, o);
    return o->items[index->value];
}
OBJECT_METHOD(listGetItem, "отримати", 1, false, nullptr);


METHOD_TEMPLATE(listClear)
{
    OBJECT_CAST();
    o->items.clear();
    return &P_null;
}
OBJECT_METHOD(listClear, "очистити", 0, false, nullptr);


static DefaultParameters listSliceDefaults = {{
    {"кількість", &P_maxInt},
}};

METHOD_TEMPLATE(listSlice)
{
    OBJECT_CAST();
    IntObject *start, *count;
    ArgParser argParser{
        {&start, intObjectType, "початок"},
        {&count, intObjectType, "кількість"},
    };
    if (!argParser.parse(args, &listSliceDefaults, na)) return nullptr;
    auto slice = ListObject::create();
    if (count->value == 0) return slice;

    CHECK_NEGATIVE_INDEX(start->value, "початок")
    CHECK_NEGATIVE_INDEX(count->value, "кількість")
    CHECK_INDEX(start->value, o);

    i64 maxCount = std::min(count->value, static_cast<i64>(o->items.size() - start->value));
    slice->items = std::vector<Object*>{
        o->items.begin() + start->value,
        o->items.begin() + start->value + maxCount
    };
    return slice;
}
OBJECT_METHOD(listSlice, "зріз", 1, false, &listSliceDefaults);


static DefaultParameters listSortDefaults = {{
    {"заКлючем", &P_null},
    {"порівняння", &P_null},
    {"обернути", &P_false},
}};

METHOD_TEMPLATE(listSort)
{
    OBJECT_CAST();
    Object *keyFunction, *cmpFunction;
    BoolObject* reverse;
    ArgParser argParser{
        {&keyFunction, objectObjectType, "заКлючем"},
        {&cmpFunction, objectObjectType, "порівняння"},
        {&reverse, boolObjectType, "обернути"}
    };
    if (!argParser.parse(args, &listSortDefaults, na)) return nullptr;

    if (keyFunction != &P_null)
    {
        using Pair = std::pair<Object*, Object*>;
        std::vector<Pair> transformedItems;
        transformedItems.reserve(o->items.size());
        for (auto it = o->items.begin(); it != o->items.end(); ++it)
        {
            auto key = keyFunction->call({it, 1});
            if (key == nullptr) return nullptr;
            transformedItems.emplace_back(key, *it);
        }

        try
        {
            if (cmpFunction == &P_null)
            {
                std::sort(transformedItems.begin(), transformedItems.end(),
                    [](const Pair& a, const Pair& b)
                    {
                        auto result = a.first->compare(b.first, ObjectCompOperator::LT);
                        // Якщо результат порівняння nullptr, значить стався виняток,
                        // і щоб перервати сортування, викидається C++ виняток, який одразу і обробляється
                        if (result == nullptr) throw std::runtime_error("");
                        auto result_ = result->asBool();
                        if (!result_) throw std::runtime_error("");
                        return result_.value();
                    }
                );
            }
            else
            {
                std::sort(transformedItems.begin(), transformedItems.end(),
                    [&](const Pair& a, const Pair& b)
                    {
                        Object* argv[] = { a.first, b.first };
                        Object* result = cmpFunction->call({argv, 2});
                        if (result == nullptr) throw std::runtime_error("");
                        return result == &P_true;
                    }
                );
            }
        }
        catch (const std::runtime_error& e)
        {
            return nullptr;
        }

        for (size_t i = 0; i < transformedItems.size(); ++i)
        {
            o->items[i] = transformedItems[i].second;
        }
    }
    else
    {
        try
        {
            if (cmpFunction == &P_null)
            {
                std::sort(o->items.begin(), o->items.end(),
                    [](Object* a, Object* b) {
                        Object* result = a->compare(b, ObjectCompOperator::LT);
                        if (result == nullptr) throw std::runtime_error("");
                        auto result_ = result->asBool();
                        if (!result_) throw std::runtime_error("");
                        return result_.value();
                    }
                );
            }
            else
            {
                std::sort(o->items.begin(), o->items.end(),
                    [&](Object* a, Object* b)
                    {
                        Object* argv[] = { a, b };
                        Object* result = cmpFunction->call({argv, 2});
                        if (result == nullptr) throw std::runtime_error("");
                        return result == &P_true;
                    });
            }
        }
        catch (const std::runtime_error& e)
        {
            return nullptr;
        }
    }

    if (reverse == &P_true)
    {
        std::reverse(o->items.begin(), o->items.end());
    }

    return &P_null;
}
OBJECT_METHOD(listSort, "впорядкувати", 0, false, &listSortDefaults);


METHOD_TEMPLATE(listExtend)
{
    OBJECT_CAST();
    auto iterable = args[0];

    // Оптимізація для вбудованих об'єктів
    if (OBJECT_IS(iterable, &listObjectType))
    {
        auto listObject = static_cast<ListObject*>(iterable);
        o->items.insert(o->items.end(), listObject->items.begin(), listObject->items.end());
        return &P_null;
    }
    else if (OBJECT_IS(iterable, &tupleObjectType))
    {
        auto tupleObject = static_cast<TupleObject*>(iterable);
        o->items.insert(o->items.end(), tupleObject->items.begin(), tupleObject->items.end());
        return &P_null;
    }

    auto iterator = iterable->getIter();
    if (iterator == nullptr) return nullptr;
    auto nextMethod = iterator->getAttr("наступний");
    if (nextMethod == nullptr)
    {
        getCurrentState()->setException(
            &TypeErrorObjectType,
            std::format("Тип \"{}\" не є ітератором", iterator->objectType->name));
        return nullptr;
    }
    Object* iteratorArgs[] { iterator };
    for (Object* item = nullptr;;)
    {
        item = nextMethod->call(iteratorArgs);
        if (item == nullptr) return nullptr;
        if (item == &P_endIter) break;
        o->items.push_back(item);
    }
    return &P_null;
}
OBJECT_METHOD(listExtend, "розширити", 1, false, nullptr)


static void listIterTraverse(ListIterObject* o)
{
    for (auto o : o->iterable)
    {
        mark(o);
    }
}

#undef X_OBJECT_STRUCT
#undef X_OBJECT_TYPE
#define X_OBJECT_STRUCT ListIterObject
#define X_OBJECT_TYPE vm::listIterObjectType

METHOD_TEMPLATE(listIterNext)
{
    OBJECT_CAST();
    if (o->position < o->length)
    {
        return o->iterable[o->position++];
    }
    return &P_endIter;
}
OBJECT_METHOD(listIterNext, "наступний", 0, false, nullptr)


namespace vm
{
    TypeObject listObjectType =
    {
        .base = &objectObjectType,
        .name = "Список",
        .size = sizeof(ListObject),
        .alloc = DEFAULT_ALLOC(ListObject),
        .dealloc = DEFAULT_DEALLOC(ListObject),
        .callableInfo =
        {
            .arity = 0,
            .name = constructorName,
            .flags = CallableInfo::IS_VARIADIC | CallableInfo::IS_METHOD,
        },
        .constructor = listInit,
        .operators =
        {
            .toString = listToString,
            .toBool = listToBool,
            .add = listAdd,
            .getIter = (unaryFunction)listGetIter,
        },
        .comparison = listComparison,
        .traverse = (traverseFunction)listTraverse,
        .attributes =
        {
            METHOD_ATTRIBUTE(listRemove),
            METHOD_ATTRIBUTE(listRemoveAll),
            METHOD_ATTRIBUTE(listInsert),
            METHOD_ATTRIBUTE(listSetItem),
            METHOD_ATTRIBUTE(listSize),
            METHOD_ATTRIBUTE(listPush),
            METHOD_ATTRIBUTE(listReplace),
            METHOD_ATTRIBUTE(listFindItem),
            METHOD_ATTRIBUTE(listCopy),
            METHOD_ATTRIBUTE(listCount),
            METHOD_ATTRIBUTE(listContains),
            METHOD_ATTRIBUTE(listReverse),
            METHOD_ATTRIBUTE(listGetItem),
            METHOD_ATTRIBUTE(listClear),
            METHOD_ATTRIBUTE(listSlice),
            METHOD_ATTRIBUTE(listSort),
            METHOD_ATTRIBUTE(listExtend),
        },
    };

    TypeObject listIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторСписку",
        .size = sizeof(ListIterObject),
        .alloc = DEFAULT_ALLOC(ListIterObject),
        .traverse = (traverseFunction)listIterTraverse,
        .attributes =
        {
            METHOD_ATTRIBUTE(listIterNext),
        },
    };
}

ListObject* vm::ListObject::create()
{
    auto listObject = (ListObject*)allocObject(&listObjectType);
    return listObject;
}

ListIterObject* vm::ListIterObject::create(const std::vector<Object*> iterable)
{
    auto listIterObject = (ListIterObject*)allocObject(&listIterObjectType);
    listIterObject->iterable = iterable;
    listIterObject->length = iterable.size();
    return listIterObject;
}
