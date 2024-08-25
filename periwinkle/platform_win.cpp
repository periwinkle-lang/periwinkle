#include <Windows.h>

#include "platform.hpp"
#include "unicode.hpp"

using namespace platform;

std::string platform::readline()
{
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

    // Видалення символів переносу рядка з кінця рядка
    line.erase(line.find_last_not_of(L"\r\n") + 1);

    return unicode::toUtf8(line);
}
