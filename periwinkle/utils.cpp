#include <iostream>
#include <unordered_map>
#include <cassert>
#ifdef _WIN32
    #include <windows.h>
#endif

#include "utils.hpp"
#include "unicode.hpp"

using namespace utils;

std::string utils::escapeString(const std::string& str)
{
    static std::map<char, std::string> charToEscapeChar =
    {
        {'\a', "\\а"}, {'\b', "\\б"}, {'\t', "\\т"}, {'\n', "\\н"}, {'\v', "\\в"}, {'\f', "\\ф"},
        {'\r', "\\р"}, {'\"', "\\\""}, {'\\', "\\\\"},
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

std::string utils::getLineFromString(std::string_view str, int line)
{
    std::istringstream iss(str.data());
    std::string result;
    for (int i = 0; i < line; ++i)
    {
        std::getline(iss, result);
    }
    return result;
}

size_t utils::linenoFromPosition(std::string_view str, size_t position)
{
    return std::count(str.begin(), str.begin() + position, '\n') + 1;
}

size_t utils::positionInLineFromPosition(std::string_view str, size_t position)
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
            strLen += 3;
            i += 3;
        }
    }
}

std::string utils::indent(int width)
{
    return std::string(width, ' ');
}

void utils::throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t position)
{
    auto lineno = utils::linenoFromPosition(source->getText(), position);
    auto positionInLine = utils::positionInLineFromPosition(source->getText(), position);
    throwSyntaxError(source, message, lineno, positionInLine);
}

void utils::throwSyntaxError(periwinkle::ProgramSource* source, std::string message, size_t lineno, size_t col)
{
    std::cerr << "Синтаксична помилка: ";
    std::cerr << message << " (знайдено на " << lineno << " рядку)\n";
    const auto& line = utils::getLineFromString(source->getText(), lineno);
    std::cerr << utils::indent(4) << line << std::endl;
    auto offset = unicode::utf8Size(line.substr(0, col));
    std::cerr << utils::indent(4 + offset) << "^\n";
}

std::unordered_map<std::string, std::pair<std::string, std::string>> words
{
    {"аргумент", {"аргумента", "аргументів"}},
};

std::string utils::wordDeclension(i64 n, const std::string& word)
{
    assert(words.contains(word) && "Такого слова немає в словнику для відмінювання");

    if (n % 10 == 1 && n % 100 != 11)
        return word;

    const auto& w = words.at(word);
    if (n % 10 >= 2 && n % 10 <= 4 && (n % 100 < 10 || n % 100 >= 20))
        return w.first;
    else
        return w.second;
}

std::string utils::trim(std::string_view str)
{
    auto start = str.begin();
    auto end = str.end();

    while (start != end && *start >= 0 && *start <= 255 && std::isspace(*start))
    {
        start++;
    }

    while (end != start && *start >= 0 && *start <= 255 && std::isspace(*(end - 1)))
    {
        end--;
    }

    return std::string{ start, end };
}

std::string utils::ltrim(std::string_view str)
{
    auto start = str.begin();
    auto end = str.end();

    while (start != end && *start >= 0 && *start <= 255 && std::isspace(*start))
    {
        start++;
    }

    return std::string{ start, end };
}

std::string utils::rtrim(std::string_view str)
{
    auto start = str.begin();
    auto end = str.end();

    while (end != start && *start >= 0 && *start <= 255 && std::isspace(*(end - 1)))
    {
        end--;
    }

    return std::string{ start, end };
}

#ifdef _WIN32
std::wstring utils::convertUtf8ToWide(std::string_view str)
{
    int count = MultiByteToWideChar(CP_UTF8, 0, data(str), (int)str.length(), NULL, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, data(str), (int)str.length(), data(wstr), count);
    return wstr;
}

std::string utils::convertWideToUtf8(std::wstring_view wstr)
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
