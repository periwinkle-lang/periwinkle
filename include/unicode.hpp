#ifndef UNICODE_HPP
#define UNICODE_HPP

#include <string>

namespace unicode
{
    struct UnicodeRecord
    {
        int lowercaseOffset;
        int uppercaseOffset;
        int titlecaseOffset;
        unsigned short flags;
    };

    bool isLowercase(char32_t ch);
    bool isUppercase(char32_t ch);
    bool isTitlecase(char32_t ch);
    bool hasCase(char32_t ch);

    /* Поветрає істину, ящко символ має категорію Zs */
    bool isSpace(char32_t ch);

    /* Повертає істину, якщо символ має категорії: Lu, Ll, Lt, Lm, Lo */
    bool isLetter(char32_t ch);
    bool isDecimal(char32_t ch);
    bool isDigit(char32_t ch);
    bool isNumeric(char32_t ch);

    /* Повертає символ у нижньому регістрі.
    Якщо символ вже знаходиться в нижньому регістрі або не має еквіваленту в нижньому регістрі,
    повертає його без змін. */
    char32_t toLowercase(char32_t ch);

    /* Повертає символ у верхньому регістрі.
    Якщо символ вже знаходиться у верхньому регістрі або не має еквіваленту у верхньому регістрі,
    повертає його без змін. */
    char32_t toUppercase(char32_t ch);

    /* Повертає символ у заголовному регістрі.
    Якщо символ вже знаходиться у заголовному регістрі або не має еквіваленту у заголовному регістрі,
    повертає його без змін. */
    char32_t toTitlecase(char32_t ch);

    // Повертає кількість символів в utf8 рядку
    size_t utf8Size(const std::string& str);

    // Повертає utf8 символ по індексу
    std::string utf8At(const std::string& str, size_t index);

    std::u32string utf8to32(const std::string& s);
    std::string utf32to8(const std::u32string& s);
}

#endif
