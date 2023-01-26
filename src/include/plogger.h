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

        // Якщо збірка релізна, то перепризначається метод, щоб нічого не виводилось
#ifndef DEBUG
        void preMessage() override {}
#endif

        template <typename T>
        PloggerDebug& operator<<(T const& obj)
        {
#ifdef DEBUG
            dest << obj;
#endif
            return *this;
        }

        PloggerDebug& operator<<(std::ostream& (*os)(std::ostream&))
        {
#ifdef DEBUG
            dest << os;
#endif
            return *this;
        }

        // Спрацьовує, коли повідомлення повністю вивелось
        ~PloggerDebug() 
        { 
#ifdef DEBUG
            dest << std::endl;
#endif
        }
    };

    // Допоміжний клас для створення екземплярів логеру
    template <typename loggerT>
    class Starter
    {
    private:
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
    public:
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
    };

    // Виводить повідомлення в консоль та завершує програму
    // !!!Використовувати лише для помилок, які можуть виникати при неправильній розробці,
    // кінцевий користувач програми не повинен бачити цих повідомлень!!!
    [[maybe_unused]] static Starter<PloggerFatal> fatal;
    
    // Виводить повідомлення в консоль, але лише в debug збірці
    [[maybe_unused]] static Starter<PloggerDebug> debug;
}

#endif
