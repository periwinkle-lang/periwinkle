#include <sstream>
#include <algorithm>
#include <utility>

#include "object.h"
#include "list_object.h"
#include "string_object.h"
#include "bool_object.h"
#include "exception_object.h"
#include "int_object.h"
#include "null_object.h"
#include "native_method_object.h"
#include "end_iteration_object.h"
#include "argument_parser.h"
#include "utils.h"

using namespace vm;


#define CHECK_LIST(object)                     \
    if (OBJECT_IS(object, &listObjectType) == false) \
        return &P_NotImplemented;

#define CHECK_INDEX(index, listObject)                                \
    if (std::cmp_greater_equal(index, listObject->items.size()))      \
    {                                                                 \
        VirtualMachine::currentVm->throwException(                    \
            &IndexErrorObjectType, "Індекс виходить за межі списку"); \
    }


static Object* listInit(Object* o, std::span<Object*> args, ListObject* va, NamedArgs* na)
{
    auto listObject = ListObject::create();
    listObject->items = va->items;
    return listObject;
}

static inline bool listObjectEqual(ListObject* a, ListObject* b, bool notEqual=false)
{
    if (a->items.size() != b->items.size())
    {
        return false;
    }

    for (size_t i = 0; i < a->items.size(); ++i)
    {
        if (((BoolObject*)Object::compare(a->items[i], b->items[i],
            notEqual ? ObjectCompOperator::NE : ObjectCompOperator::EQ))->value == false)
        {
            return false;
        }
    }

    return true;
}

static Object* listComparison(Object* o1, Object* o2, ObjectCompOperator op)
{
    CHECK_LIST(o1);
    CHECK_LIST(o2);
    auto a = (ListObject*)o1;
    auto b = (ListObject*)o2;
    bool result;

    using enum ObjectCompOperator;
    switch (op)
    {
    case EQ: result = listObjectEqual(a, b);       break;
    case NE: result = listObjectEqual(a, b, true); break;
    default:
        return &P_NotImplemented;
    }

    return P_BOOL(result);
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
            auto stringObject = (StringObject*)Object::toString(*it);
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

METHOD_TEMPLATE(listRemove, ListObject)
{
    auto it = std::find_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    if (it != o->items.end())
    {
        o->items.erase(it);
        return &P_true;
    }
    return &P_false;
}

METHOD_TEMPLATE(listRemoveAll, ListObject)
{
    auto erased = std::erase_if(
        o->items,
        [&args](Object* o) { return ((BoolObject*)Object::compare(
            o, args[0], ObjectCompOperator::EQ))->value; }
    );

    return P_BOOL(erased);
}

METHOD_TEMPLATE(listInsert, ListObject)
{
    IntObject* index;
    Object* element;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    argParser.parse(args);

    auto listObject = (ListObject*)o;
    CHECK_INDEX(index->value, listObject);
    listObject->items.insert(listObject->items.begin() + index->value, element);
    return &P_null;
}

METHOD_TEMPLATE(listSetItem, ListObject)
{
    IntObject* index;
    Object* element;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
        {&element, objectObjectType, "елемент"},
    };
    argParser.parse(args);

    auto listObject = (ListObject*)o;
    CHECK_INDEX(index->value, listObject);
    listObject->items[index->value] = element;
    return &P_null;
}

METHOD_TEMPLATE(listSize, ListObject)
{
    return IntObject::create(o->items.size());
}

METHOD_TEMPLATE(listPush, ListObject)
{
    o->items.push_back(args[0]);
    return &P_null;
}

METHOD_TEMPLATE(listReplace, ListObject)
{
    size_t replaceCount = 0;

    for (auto it = o->items.begin(); it != o->items.end(); ++it)
    {
        if (((BoolObject*)Object::compare(*it, args[0], ObjectCompOperator::EQ))->value)
        {
            *it = args[1];
            ++replaceCount;
        }
    }

    return P_BOOL(replaceCount);
}

METHOD_TEMPLATE(listFindItem, ListObject)
{
    auto it = std::find_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)Object::compare(
            obj, args[0], ObjectCompOperator::EQ))->value; }
    );

    size_t index = -1;
    if (it != o->items.end())
    {
        index = it - o->items.begin();
    }
    return IntObject::create(index);
}

METHOD_TEMPLATE(listCopy, ListObject)
{
    auto newListObject = ListObject::create();
    newListObject->items = o->items;
    return newListObject;
}

METHOD_TEMPLATE(listCount, ListObject)
{
    auto count = std::count_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)Object::compare(
            obj, args[0], ObjectCompOperator::EQ))->value; }
    );
    return IntObject::create(count);
}

METHOD_TEMPLATE(listContains, ListObject)
{
    auto it = std::find_if(
        o->items.begin(),
        o->items.end(),
        [&args](Object* obj) { return ((BoolObject*)Object::compare(
            obj, args[0], ObjectCompOperator::EQ))->value; }
    );

    i64 index = -1;
    if (it != o->items.end())
    {
        index = it - o->items.begin();
    }

    return P_BOOL(index != -1);
}

METHOD_TEMPLATE(listReverse, ListObject)
{
    std::reverse(o->items.begin(), o->items.end());
    return &P_null;
}

METHOD_TEMPLATE(listGetItem, ListObject)
{
    IntObject* index;
    static ArgParser argParser{
        {&index, intObjectType, "індекс"},
    };
    argParser.parse(args);

    CHECK_INDEX(index->value, o);
    return o->items[index->value];
}

METHOD_TEMPLATE(listClear, ListObject)
{
    o->items.clear();
    return &P_null;
}

METHOD_TEMPLATE(listSublist, ListObject)
{
    IntObject *start, *count;
    static ArgParser argParser{
        {&start, intObjectType, "початок"},
        {&count, intObjectType, "кількість"},
    };
    argParser.parse(args);

    CHECK_INDEX(start->value, o);
    CHECK_INDEX(start->value + count->value - (count->value == 0 ? 0 : 1), o);
    auto subList = ListObject::create();
    subList->items = std::vector<Object*>{
        o->items.begin() + start->value, o->items.begin() + count->value };
    return subList;
}

METHOD_TEMPLATE(listIterNext, ListIterObject)
{
    if (o->position < o->length)
    {
        return o->iterable[o->position++];
    }
    return &P_endIter;
}

namespace vm
{
    TypeObject listObjectType =
    {
        .base = &objectObjectType,
        .name = "Список",
        .alloc = DEFAULT_ALLOC(ListObject),
        .constructor = new NATIVE_METHOD("конструктор", 0, true, listInit, listObjectType, nullptr),
        .operators =
        {
            .toString = listToString,
            .toBool = listToBool,
            .add = listAdd,
            .getIter = (unaryFunction)listGetIter,
        },
        .comparison = listComparison,
        .attributes =
        {
            OBJECT_METHOD("видалити",   1, false, listRemove,    listObjectType, nullptr),
            OBJECT_METHOD("видалитиВсі",1, false, listRemoveAll, listObjectType, nullptr),
            OBJECT_METHOD("вставити",   2, false, listInsert,    listObjectType, nullptr),
            OBJECT_METHOD("встановити", 2, false, listSetItem,   listObjectType, nullptr),
            OBJECT_METHOD("довжина",    0, false, listSize,      listObjectType, nullptr),
            OBJECT_METHOD("додати",     1, false, listPush,      listObjectType, nullptr),
            OBJECT_METHOD("замінити",   2, false, listReplace,   listObjectType, nullptr),
            OBJECT_METHOD("знайти",     1, false, listFindItem,  listObjectType, nullptr),
            OBJECT_METHOD("копія",      0, false, listCopy,      listObjectType, nullptr),
            OBJECT_METHOD("кількість",  1, false, listCount,     listObjectType, nullptr),
            OBJECT_METHOD("містить",    1, false, listContains,  listObjectType, nullptr),
            OBJECT_METHOD("обернути",   0, false, listReverse,   listObjectType, nullptr),
            OBJECT_METHOD("отримати",   1, false, listGetItem,   listObjectType, nullptr),
            OBJECT_METHOD("очистити",   0, false, listClear,     listObjectType, nullptr),
            OBJECT_METHOD("підсписок",  2, false, listSublist,   listObjectType, nullptr),
        },
    };

    TypeObject listIterObjectType =
    {
        .base = &objectObjectType,
        .name = "ІтераторСписку",
        .alloc = DEFAULT_ALLOC(ListIterObject),
        .attributes =
        {
            OBJECT_METHOD("наступний", 0, false, listIterNext, listIterObjectType, nullptr),
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
