#ifndef STRING_ENUM_H
#define STRING_ENUM_H

#include <algorithm>
#include <climits>
#include <vector>
#include <string>

namespace _stringEnum
{
    static inline std::string& _ltrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c); }));
        return s;
    }

    static const std::vector<std::string> tokenizeEnumString(std::string str)
    {
        auto len = std::count(str.begin(), str.end(), ',') + 1;
        std::vector<std::string> strings(len);

        size_t position = 0;
        for (int i = 0; i < len; ++i)
        {
            auto commaPosition = str.find(',', position);
            if (i == len - 1)
            {
                commaPosition = INT_MAX;
            }
            else if (i > 0)
            {
                commaPosition -= position;
            }
            strings[i] = str.substr(position, commaPosition);
            position += strings[i].size() + 1;
            _ltrim(strings[i]);
        }
        return strings;
    }
}

// Створює перечислення(enum class) та функцію для конвертації членів перечислення в рядок.
// Функція створюється в просторі імен "stringEnum" та має назву enumToString.
// Приклад:
//    STRING_ENUM(Color, Red, Green, Blue) // Перечислення Color з членами Red, Green, Blue
//    std::cout << stringEnum::enumToString(Color::Red) << std::endl;
//    Виведе: "Red"
#define STRING_ENUM(NAME, ...)                               \
    enum class NAME { __VA_ARGS__ };                         \
    namespace stringEnum {                                   \
    const static auto _enum##NAME##ToString =                \
    _stringEnum::tokenizeEnumString( #__VA_ARGS__ );         \
        static inline std::string enumToString(NAME element) \
        {                                                    \
            return _enum##NAME##ToString[(int)element];      \
        }                                                    \
    }

#endif
