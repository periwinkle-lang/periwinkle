#ifndef PERIWINKLE_H
#define PERIWINKLE_H

#include <string>

namespace periwinkle
{
    class Periwinkle
    {
    private:
        std::string code;
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
        void printTokens();
        void printDisassemble();
#endif
        Periwinkle(std::string code);
    };
}

#endif
