#ifndef PERIWINKLE_H
#define PERIWINKLE_H

#include <string>
#include <filesystem>

#include "exports.hpp"
#include "program_source.hpp"

namespace periwinkle
{
    class API Periwinkle
    {
    private:
        ProgramSource* source;
    public:
        // Повертає версію як число, 2 цифри на значення.
        //  Наприклад: версія 1.10.2, то повернеться чило 11002
        static int getVersionAsInt();
        // Повертає версію в форматі major.minor.patch.
        static std::string getVersionAsString();
        static int majorVersion();
        static int minorVersion();
        static int patchVersion();
        void execute();
#ifdef DEBUG
        void printDisassemble();
#endif
        Periwinkle(const std::string& code);
        Periwinkle(const std::filesystem::path& path);
        Periwinkle(const ProgramSource& source);
    };
}

#endif
