#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <sstream>
#include <algorithm>

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
}

#endif
