#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <memory>

namespace utils
{
    std::string escapeString(const std::string& str);
    std::string getLineFromString(const std::string& str, int line);
    size_t linenoFromPosition(const std::string& str, size_t position);

    // Визначає позицію в стрічці із загальної позиції символа в стрічці
    size_t positionInLineFromPosition(const std::string& str, size_t position);
    void replaceTabToSpace(std::string& str);

    // Повертає кількість символів в utf8 стрічці
    size_t utf8Size(const std::string& str);

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

#ifdef _WIN32
    std::wstring convertUtf8ToWide(const std::string& str);
    std::string convertWideToUtf8(const std::wstring& wstr);
#endif

    std::string readline();
}

#endif
