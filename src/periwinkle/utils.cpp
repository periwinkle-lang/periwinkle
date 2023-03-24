#include <iostream>
#ifdef _WIN32
    #include <windows.h>
#endif

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

static int getUtf8CharLen(char c)
{
    auto uc = (unsigned char)c;
    if (uc <= 127) return 1;
    else if ((uc & 0xE0) == 0xC0) return 2;
    else if ((uc & 0xF0) == 0xE0) return 3;
    else if ((uc & 0xF8) == 0xF0) return 4;
    return -1;
}

size_t utils::utf8Size(const std::string& str)
{
    size_t i, q;
    for (q = 0, i = 0; i < str.length(); q++)
    {
        i += getUtf8CharLen(str[i]);
    }
    return q;
}

std::string utils::utf8At(const std::string& str, size_t index)
{
    size_t i, q;
    for (q = 0, i = 0; q < index; q++)
    {
        auto len = getUtf8CharLen(str[i]);
        i += len;
    }
    return str.substr(i, getUtf8CharLen(str[i]));
}

std::string utils::indent(int width)
{
    return std::string(width, ' ');
}

#ifdef _WIN32
std::wstring utils::convertUtf8ToWide(const std::string& str)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, data(str), (int)str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, data(str), (int)str.length(), data(wstr), count);
    return wstr;
}

std::string utils::convertWideToUtf8(const std::wstring& wstr)
{
    int count = WideCharToMultiByte(CP_UTF8, 0, data(wstr), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string str(count, 0);
    WideCharToMultiByte(CP_UTF8, 0, data(wstr), -1, data(str), count, NULL, NULL);
    return str;
}
#endif

std::string utils::readline()
{
#ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    const DWORD chunkSize = 256;
    WCHAR buffer[chunkSize] = { 0 };
    DWORD charsRead = 0;
    std::wstring line;

    while (true) {
        if (!ReadConsoleW(hStdin, buffer, chunkSize, &charsRead, NULL)) {
            break;
        }

        line += std::wstring(buffer, charsRead);
        if (line.back() == L'\n' || line.back() == L'\r') {
            break;
        }
    }

    // Видалення символів переносу рядка з кінця стрічки
    line.erase(line.find_last_not_of(L"\r\n") + 1);

    return utils::convertWideToUtf8(line);
#else
    std::string line;
    std::getline(std::cin, line);
    return line;
#endif
}
