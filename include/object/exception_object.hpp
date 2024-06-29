#ifndef EXCEPTION_OBJECT_H
#define EXCEPTION_OBJECT_H

#include "object.hpp"
#include "program_source.hpp"
#include "vm.hpp"

namespace vm
{
    struct Frame;

    extern TypeObject ExceptionObjectType;
    extern TypeObject NameErrorObjectType;
    extern TypeObject TypeErrorObjectType;
    extern TypeObject NotImplementedErrorObjectType;
    extern TypeObject AttributeErrorObjectType;
    extern TypeObject IndexErrorObjectType;
    extern TypeObject DivisionByZeroErrorObjectType;
    extern TypeObject ValueErrorObjectType;
    extern TypeObject InternalErrorObjectType;

    struct StackTraceItem
    {
        periwinkle::ProgramSource* source;
        i64 lineno;
        std::string functionName;
    };

    // Для всіх винятків буде використовуватись лише ця структура,
    // буде змінюватись лише тип об'єкта в полі objectType
    struct ExceptionObject : Object
    {
        std::string message; // Повідомлення винятку
        std::vector<StackTraceItem> stackTrace;
        // Рядок, на якій було викинуто помилку, потрібно для правильного відображення рядка,
        // при обробці помилок
        i64 lineno = 0;

        std::string formatStackTrace() const;
        void addStackTraceItem(Frame* frame, i64 lineno);

        static ExceptionObject* create(TypeObject* type, const std::string& message);
    };

    extern ExceptionObject P_NotImplemented;
    bool isException(TypeObject* type);
}

#endif
