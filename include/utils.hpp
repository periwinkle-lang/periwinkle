#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <memory>

#include "exports.hpp"
#include "program_source.hpp"
#include "types.hpp"

namespace utils
{
    std::string escapeString(const std::string& str);
    std::string getLineFromString(std::string_view str, int line);
    size_t linenoFromPosition(std::string_view str, size_t position);

    // Визначає позицію в рядку із загальної позиції символа в рядку
    size_t positionInLineFromPosition(std::string_view str, size_t position);
    void replaceTabToSpace(std::string& str);

    // Відступ на задану ширину, заповнює відступ пробілами.
    // Наприклад:
    //     std::cout << "id =" << indent(10) << "10";
    // Виведе:
    //     id =          10
    std::string indent(int width);

    void throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t position);
    void throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t lineno, size_t col);

    // Відмнює слова в залежності від числа
    std::string wordDeclension(i64 n, const std::string& word);

    std::string trim(std::string_view str);
    std::string ltrim(std::string_view str);
    std::string rtrim(std::string_view str);

#ifdef _WIN32
    API std::wstring convertUtf8ToWide(std::string_view str);
    API std::string convertWideToUtf8(std::wstring_view wstr);
#endif

    std::string readline();
}

#endif
