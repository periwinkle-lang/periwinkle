#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <memory>

#include "exports.hpp"
#include "program_source.hpp"

namespace utils
{
    std::string escapeString(const std::string& str);
    std::string getLineFromString(std::string_view str, int line);
    size_t linenoFromPosition(std::string_view str, size_t position);

    // Визначає позицію в стрічці із загальної позиції символа в стрічці
    size_t positionInLineFromPosition(std::string_view str, size_t position);
    void replaceTabToSpace(std::string& str);

    // Повертає кількість символів в utf8 стрічці
    size_t utf8Size(const std::string& str);

    // Повертає utf8 символ по індексу
    std::string utf8At(const std::string& str, size_t index);

    std::u32string utf8to32(const std::string& s);
    std::string utf32to8(const std::u32string& s);

    // Відступ на задану ширину, заповнює відступ пробілами.
    // Наприклад:
    //     std::cout << "id =" << indent(10) << "10";
    // Виведе:
    //     id =          10
    std::string indent(int width);

    // Форматує стрічки за допомогою std::snprintf
    template<typename ... Args>
    std::string format(const std::string& format, Args ... args)
    {
        auto size = (size_t)std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // + 1 для '\0'
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args ...);
        return std::string(buf.get(), buf.get() + size - 1); // - 1, щоб видалити '\0'
    }

    void throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t position);
    void throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t lineno, size_t col);

#ifdef _WIN32
    API std::wstring convertUtf8ToWide(std::string_view str);
    API std::string convertWideToUtf8(std::wstring_view wstr);
#endif

    std::string readline();
}

#endif
