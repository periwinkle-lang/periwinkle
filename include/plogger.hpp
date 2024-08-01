#ifndef PLOGGER_H
#define PLOGGER_H

#include <iostream>
#include <string>
#include <source_location>

namespace plog
{
    class Plogger
    {
    protected:
        std::ostream& dest;
        const std::source_location& location;
        std::string prefix;

        // Повертає лише назву файлу, на вхід отримує шлях
        constexpr const char* getFileName(const char* path) {
            const char* file = path;
            while (*path++) {
                if (*path == '/' || *path == '\\') {
                    file = ++path;
                }
            }
            return file;
        }
    public:
        Plogger(std::ostream& dest, const std::source_location& location, std::string prefix)
            : dest(dest), location(location), prefix(prefix) {}

        virtual void preMessage()
        {
            dest << "[" << prefix << "]" << "[" << location.function_name() << "]" << "["
                << getFileName(location.file_name()) << ":" << location.line() << "]: ";
        }

        template <typename T>
        Plogger& operator<<(T const& obj)
        {
            dest << obj;
            return *this;
        }

        // Перевизначення для маніпуляторів(по типу endl, setw та інші)
        Plogger& operator<<(std::ostream& (*os)(std::ostream&))
        {
            dest << os;
            return *this;
        }
    };

    class PloggerFatal : public Plogger
    {
    public:
        PloggerFatal(const std::source_location& location)
            : Plogger(std::cerr, location, "FATAL") {}

        // Спрацьовує, коли повідомлення повністю вивелось, при цьому аварійно завершує програму
        [[noreturn]] ~PloggerFatal()
        {
            dest << std::endl;
            exit(EXIT_FAILURE);
        }
    };

    class PloggerDebug : public Plogger
    {
    public:
        PloggerDebug(const std::source_location& location)
            : Plogger(std::cout, location, "DEBUG") {}

        // Спрацьовує, коли повідомлення повністю вивелось
        ~PloggerDebug()
        {
            dest << std::endl;
        }
    };

    class PloggerAssert : public Plogger
    {
    private:
        bool condition;
    public:
        PloggerAssert(const std::source_location& location, bool condition)
            : Plogger(std::cerr, location, "ASSERT"), condition(condition) {}

        template <typename T>
        Plogger& operator<<(T const& obj)
        {
            if (!condition) dest << obj;
            return *this;
        }

        Plogger& operator<<(std::ostream& (*os)(std::ostream&))
        {
            if (!condition) dest << os;
            return *this;
        }

        ~PloggerAssert()
        {
            if (!condition)
            {
                dest << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    };

    // Допоміжна структура, яка дозволяє отримати правильний source_location в перевантажених операторах,
    // так як в них неможливо додати аргумент за замовчуванням
    template <typename T>
    struct located_reference
    {
        located_reference(const T& v, std::source_location l = std::source_location::current())
            : location(l), value(v) {};

        std::source_location location;
        T& value;
    };

    // Допоміжний клас для створення екземплярів логеру
    template <typename loggerT>
    class Starter
    {
    public:
#ifdef DEBUG
        template <typename T>
        friend loggerT operator<<(located_reference<plog::Starter<loggerT>&> starter, T const& obj)
        {
            loggerT logger(starter.location);
            logger.preMessage();
            logger << obj;
            return logger;
        }

        // Перевизначення для маніпуляторів
        friend loggerT operator<<(located_reference<plog::Starter<loggerT>&> starter,
            std::ostream& (*os)(std::ostream&))
        {
            loggerT logger(starter.location);
            logger.preMessage();
            logger << os;
            return logger;
        }
#else
        // Якщо збірка release, то нічого не працює
        template <typename T> Starter& operator<<(T const& obj) { return *this; }
        Starter& operator<<(std::ostream& (*os)(std::ostream&)) { return *this; }
#endif
    };

    class AssertStarter
    {
#ifdef DEBUG
    private:
        bool condition;
    public:
        template <typename T>
        friend PloggerAssert operator<<(located_reference<plog::AssertStarter&> starter, T const& obj)
        {
            PloggerAssert logger(starter.location, starter.value.condition);
            if (!starter.value.condition)
            {
                logger.preMessage();
                logger << obj;
            }
            return logger;
        }

        friend PloggerAssert operator<<(located_reference<plog::AssertStarter&> starter,
            std::ostream& (*os)(std::ostream&))
        {
            PloggerAssert logger(starter.location, starter.value.condition);
            if (!starter.value.condition)
            {
                logger.preMessage();
                logger << os;
            }
            return logger;
        }

        AssertStarter& operator()(bool condition)
        {
            this->condition = condition;
            return *this;
        }
#else
    public:
        template <typename T> AssertStarter& operator<<(T const& obj) { return *this; }
        AssertStarter& operator<<(std::ostream& (*os)(std::ostream&)) { return *this; }
        AssertStarter& operator()(bool condition) { return *this; }
#endif
    };

    // Виводить повідомлення в консоль та завершує програму. Працює лише в debug збірці.
    [[maybe_unused]] static Starter<PloggerFatal> fatal;

    // Виводить повідомлення в консоль. Працює лише в debug збірці.
    // Працює як cout:
    //     plog::debug << "Повідомлення";
    [[maybe_unused]] static Starter<PloggerDebug> debug;

    // Аналог assert(). Працює лише в debug збірці.
    // Приклад:
    //     plog::passert(condition) << "Повідомлення";
    [[maybe_unused]] static AssertStarter passert;
}

#endif
