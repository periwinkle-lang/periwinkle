#ifndef PERIWINKLE_H
#define PERIWINKLE_H

#include <string>
#include <filesystem>

#include "object.hpp"
#include "exception_object.hpp"
#include "exports.hpp"
#include "program_source.hpp"

namespace periwinkle
{
    class API Periwinkle
    {
    private:
        ProgramSource* source;
        vm::ExceptionObject* currentException = nullptr;
    public:
        // Повертає версію як число, 2 цифри на значення.
        //  Наприклад: версія 1.10.2, то повернеться чило 11002
        static int getVersionAsInt();
        // Повертає версію в форматі major.minor.patch.
        static std::string getVersionAsString();
        static int majorVersion();
        static int minorVersion();
        static int patchVersion();
        vm::Object* execute();

        // Встановлює помилку, type повинен бути або ExceptionObjectType, або його підкласом,
        // інакше буде викинута "ВнутрішняПомилка"
        void setException(vm::TypeObject* type, const std::string& message);
        // Встановлює помику, переданий об'єкт повинен бути ExceptionObject, або його підкласом,
        // інакше буде викинута "ВнутрішняПомилка"
        void setException(vm::Object* o);
        // Повертає об'єкт помилки або nullptr, якщо помилка не була викинута
        vm::ExceptionObject* exceptionOccurred() const;
        void exceptionClear();
        void printException() const;


#ifdef DEBUG
        void printDisassemble();
#endif
        Periwinkle(const std::string& code);
        Periwinkle(const std::filesystem::path& path);
        Periwinkle(const ProgramSource& source);
        ~Periwinkle();
    };
}

periwinkle::Periwinkle* getCurrentState();

#endif
