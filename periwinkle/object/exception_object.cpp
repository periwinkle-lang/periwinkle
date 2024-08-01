#include <span>

#include "exception_object.hpp"
#include "string_object.hpp"
#include "utils.hpp"
#include "code_object.hpp"
#include "native_method_object.hpp"
#include "argument_parser.hpp"

using namespace vm;

#define EXCEPTION_EXTEND(baseType, exc, excName, excOperators) \
    TypeObject exc##ObjectType =                        \
    {                                                   \
        .base = &baseType,                              \
        .name = excName,                                \
        .size = sizeof(ExceptionObject),                \
        .alloc = exceptionAlloc,                        \
        .dealloc = exceptionDealloc,                    \
        .constructor = &exceptionInitMethod,            \
        .operators = excOperators,                      \
    };

static DefaultParameters exceptionInitDefaults = { {"повідомлення"}, {&P_emptyStr} };

METHOD_TEMPLATE(exceptionInit, TypeObject)
{
    StringObject* message;
    ArgParser argParser{
        {&message, stringObjectType, "повідомлення"},
    };
    if (!argParser.parse(args, &exceptionInitDefaults, na)) return nullptr;
    return ExceptionObject::create(o, message->asUtf8());
}

auto exceptionInitMethod = NATIVE_METHOD(
    "конструктор", 1, false, reinterpret_cast<nativeMethod>(exceptionInit), ExceptionObjectType, nullptr);

static Object* exceptionAlloc()
{
    auto o = new ExceptionObject;
    return (Object*)o;
}

static void exceptionDealloc(Object* o)
{
    delete (ExceptionObject*)o;
}

static Object* exceptionToString(Object* a)
{
    auto exception = (ExceptionObject*)a;
    return StringObject::create(exception->message);
}

namespace vm
{
    TypeObject ExceptionObjectType =
    {
        .base = &objectObjectType,
        .name = "Виняток",
        .size = sizeof(ExceptionObject),
        .alloc = exceptionAlloc,
        .dealloc = exceptionDealloc,
        .constructor = &exceptionInitMethod,
        .operators =
        {
            .toString = exceptionToString,
        },
    };

    EXCEPTION_EXTEND(ExceptionObjectType, NameError, "ПомилкаІмені",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, TypeError, "ПомилкаТипу",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, NotImplementedError, "ПомилкаРеалізації",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, AttributeError, "ПомилкаАтрибута",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, IndexError, "ПомилкаІндексу",
        { .toString = exceptionToString });

    EXCEPTION_EXTEND(ExceptionObjectType, DivisionByZeroError, "ПомилкаДіленняНаНуль",
        { .toString = exceptionToString } );

    EXCEPTION_EXTEND(ExceptionObjectType, ValueError, "ПомилкаЗначення",
        { .toString = exceptionToString } );

    EXCEPTION_EXTEND(ExceptionObjectType, InternalError, "ВнутрішняПомилка",
        { .toString = exceptionToString });

    ExceptionObject P_NotImplemented{ {{&NotImplementedErrorObjectType}} };

    std::string vm::ExceptionObject::formatStackTrace() const
    {
        std::stringstream format;

        for (auto item = stackTrace.cbegin(); item != stackTrace.cend(); item++)
        {
            i64 line = item->lineno;
            if (item == stackTrace.cbegin() && lineno)
                line = lineno;
            format << "    \"";
            if (item->source->hasFile()) format << item->source->getPath().relative_path().string();
            else format << item->source->getFilename();
            format << "\" на лінії " << line;
            if (!item->functionName.empty())
                format << " в " << item->functionName;
            format << "\n        ";
            format << utils::trim(utils::getLineFromString(item->source->getText(), line));
            format << "\n";
        }
        return format.str();
    }

    void vm::ExceptionObject::addStackTraceItem(vm::Frame* frame, i64 lineno)
    {
        stackTrace.emplace_back(frame->codeObject->source, lineno, frame->codeObject->name);
    }


    ExceptionObject* vm::ExceptionObject::create(TypeObject* type, const std::string& message)
    {
        auto exceptionObject = (ExceptionObject*)allocObject(type);
        exceptionObject->message = message;
        return exceptionObject;
    }

    bool isException(TypeObject* type)
    {
        auto t = type;
        for (;;)
        {
            if (&ExceptionObjectType == t)
              return true;
            t = t->base;
            if (t == &objectObjectType || t == nullptr)
                return false;
        }
    }
}
