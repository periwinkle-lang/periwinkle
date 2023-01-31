#include "utils.h"

using namespace utils;

std::string utils::escapeString(const std::string& str)
{
    static std::map<char, std::string> charToEscapeChar =
    {
        {'\a', "\\а"}, {'\b', "\\б"}, {'\t', "\\т"}, {'\n', "\\н"}, {'\v', "\\в"}, {'\f', "\\ф"},
        {'\r', "\\р"}, {'\"', "\\\""}, {'\?', "\\\?"}, {'\\', "\\\\"},
    };

    std::stringstream ss;
    for (char c : str)
    {
        if (charToEscapeChar.count(c))
        {
            ss << charToEscapeChar.at(c);
        }
        else
        {
            ss << c;
        }
    }
    return ss.str();
}

std::string utils::getLineFromString(const std::string& str, int line)
{
    std::istringstream iss(str);
    std::string result;
    for (int i = 0; i < line; ++i)
    {
        std::getline(iss, result);
    }
    return result;
}

size_t utils::linenoFromPosition(const std::string& str, size_t position)
{
    return std::count(str.begin(), str.begin() + position, '\n') + 1;
}

size_t utils::positionInLineFromPosition(const std::string& str, size_t position)
{
    auto lineno = linenoFromPosition(str, position);
    if (lineno == 1)
    {
        return position;
    }

    size_t newLinePosition = -1;
    for (size_t i = 0; i < lineno - 1; ++i)
    {
        newLinePosition = str.find('\n', newLinePosition + 1);
    }
    return position - newLinePosition - 1;
}

void utils::replaceTabToSpace(std::string& str)
{
    auto strLen = str.length();
    for (size_t i = 0; i < strLen; ++i)
    {
        if (str[i] == '\t')
        {
            str[i] = ' ';
            str.insert(i + 1, "   ");
            strLen += 4;
        }
    }
}

size_t utils::utf8Size(const std::string& str)
{
    size_t i, q;
    for (q = 0, i = 0; i < str.length(); i++, q++)
    {
        auto c = (unsigned char)str[i];
        if (c <= 127) continue;
        else if ((c & 0xE0) == 0xC0) i += 1;
        else if ((c & 0xF0) == 0xE0) i += 2;
        else if ((c & 0xF8) == 0xF0) i += 3;
        else return 0; // Неправильний utf8
    }
    return q;
}

std::string utils::indent(int width)
{
    return std::string(width, ' ');
}
