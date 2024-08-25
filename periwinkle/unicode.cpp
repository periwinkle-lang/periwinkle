#include <algorithm>

#include "unicode.hpp"
#include "unicode_database.hpp"

using namespace unicode;

static inline unsigned char getValueAtIndex(unsigned int index)
{
    auto it = std::lower_bound(indexes.cbegin(), indexes.cend(), index);

    // Якщо знайдений індекс перевищує шуканий, береться попередній елемент
    if (it == indexes.cend() || *it > index)
        --it;

    size_t idx = std::distance(indexes.cbegin(), it);
    return values[idx];
}

inline UnicodeRecord* getUnicodeRecord(char32_t ch)
{
    auto idx = getValueAtIndex(static_cast<unsigned int>(ch >= 0x110000 ? 0 : ch));
    return &records[idx];
}

bool unicode::isLowercase(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & LOWERCASE;
}

bool unicode::isUppercase(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & UPPERCASE;
}

bool unicode::isTitlecase(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & TITLECASE;
}

bool unicode::hasCase(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & HAS_CASE;
}

bool unicode::isSpace(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & IS_SPACE;
}

bool unicode::isLetter(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & IS_LETTER;
}

bool unicode::isDecimal(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & IS_DECIMAL;
}

bool unicode::isDigit(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & IS_DIGIT;
}

bool unicode::isNumeric(char32_t ch)
{
    return getUnicodeRecord(ch)->flags & IS_NUMERIC;
}

char32_t unicode::toLowercase(char32_t ch)
{
    return ch + getUnicodeRecord(ch)->lowercaseOffset;
}

char32_t unicode::toUppercase(char32_t ch)
{
    return ch + getUnicodeRecord(ch)->uppercaseOffset;
}

char32_t unicode::toTitlecase(char32_t ch)
{
    return ch + getUnicodeRecord(ch)->titlecaseOffset;
}

static inline int getUtf8CharLen(char c)
{
    auto uc = (unsigned char)c;
    if (uc <= 127) return 1;
    if ((uc & 0xE0) == 0xC0) return 2;
    if ((uc & 0xF0) == 0xE0) return 3;
    if ((uc & 0xF8) == 0xF0) return 4;
    return -1;
}

static inline int getUtf16CharLen(char16_t c) {
    if (c >= 0xD800 && c <= 0xDBFF) return 2;
    if (c >= 0xDC00 && c <= 0xDFFF) return 0; // Нижній сурогат
    return 1;
}

// Повертає кількість використаних char
static inline int char8to32(char32_t& c32, const char* c8)
{
    auto length = getUtf8CharLen(*c8);
    switch (length) {
    case 1:
        c32 = 0x7f & *c8;
        break;
    case 2:
        c32 = (*c8 & 0x1f) << 6 | (*(c8 + 1) & 0x3f);
        break;
    case 3:
        c32 = (*c8 & 0xf) << 12 | (*(c8 + 1) & 0x3f) << 6 | (*(c8 + 2) & 0x3f);
        break;
    case 4:
        c32 = (*c8 & 0x7) << 18 | (*(c8 + 1) & 0x3f) << 12 | (*(c8 + 2) & 0x3f) << 6 | (*(c8 + 3) & 0x3f);
        break;
    }
    return length;
}

// Повертає кількість використаних char
static inline int char8to16(char16_t* c16, const char* c8)
{
    char32_t c32;
    int length = char8to32(c32, c8);
    if (length == 4)
    {
        char32_t u = c32 - 0x10000;
        *c16++ = 0xd800 | (u & 0xffc00);
        *c16 = 0xdc00 | (u & 0x3ff);
    }
    else
    {
        *c16 = static_cast<char16_t>(c32);
    }
    return length;
}

static int char32to8(char* c8, char32_t c32);

// Повертає кількість згенерованих char
static inline int char16to8(char* c8, const char16_t* c16)
{
    auto length = getUtf16CharLen(*c16);
    if (length == 1)
    {
        return char32to8(c8, static_cast<char32_t>(*c16));
    }
    else
    {
        char32_t c32 = ((*c16 & 0x3ff) << 10 | (*(c16 + 1) & 0x3ff)) + 0x10000;
        *c8++ = (c32 >> 18) | 0xf0;
        *c8++ = ((c32 >> 12) & 0x3f) | 0x80;
        *c8++ = ((c32 >> 6) & 0x3f) | 0x80;
        *c8 = (c32 & 0x3f) | 0x80;
        return 4;
    }
}

// Повертає кількість використаних char16
static inline int char16to32(char32_t& c32, const char16_t* c16)
{
    auto length = getUtf16CharLen(*c16);
    if (length == 1)
    {
        c32 = static_cast<char32_t>(*c16);
    }
    else
    {
        c32 = ((*c16 & 0x3ff) << 10 | (*(c16 + 1) & 0x3ff)) + 0x10000;
    }
    return length;
}

// Повертає кількість згенерованих char
static inline int char32to8(char* c8, char32_t c32)
{
    if (c32 < 0x80)
    {
        *c8 = c32;
        return 1;
    }
    else if (c32 < 0x800)
    {
        *c8++ = (c32 >> 6) | 0xc0;
        *c8   = (c32 & 0x3f) | 0x80;
        return 2;
    }
    else if (c32 < 0x10000)
    {
        *c8++ = (c32 >> 12) | 0xe0;
        *c8++ = ((c32 >> 6) & 0x3f) | 0x80;
        *c8   = (c32 & 0x3f) | 0x80;
        return 3;
    }
    else
    {
        *c8++ = (c32 >> 18) | 0xf0;
        *c8++ = ((c32 >> 12) & 0x3f) | 0x80;
        *c8++ = ((c32 >> 6) & 0x3f) | 0x80;
        *c8   = (c32 & 0x3f) | 0x80;
        return 4;
    }
}

// Повертає кількість згенерованих char16_t
static inline int char32to16(char16_t* c16, char32_t c32)
{
    if (c32 < 0x10000)
    {
        *c16 = static_cast<char16_t>(c32);
        return 1;
    }
    else
    {
        char32_t u = c32 - 0x10000;
        *c16++ = 0xd800 | (u & 0xffc00);
        *c16 = 0xdc00 | (u & 0x3ff);
        return 2;
    }
}

size_t unicode::utf8Size(std::string_view str)
{
    size_t i, q;
    for (q = 0, i = 0; i < str.length(); q++)
    {
        i += getUtf8CharLen(str[i]);
    }
    return q;
}

size_t unicode::utf16Size(std::u16string_view str)
{
    size_t i, q;
    for (q = 0, i = 0; i < str.length(); q++)
    {
        i += getUtf16CharLen(str[i]);
    }
    return q;
}

std::string unicode::utf8At(std::string_view str, size_t index)
{
    size_t i, q;
    for (q = 0, i = 0; q < index; q++)
    {
        auto len = getUtf8CharLen(str[i]);
        i += len;
    }
    return std::string(str.substr(i, getUtf8CharLen(str[i])));
}

std::u16string unicode::utf16At(std::u16string_view str, size_t index)
{
    size_t i, q;
    for (q = 0, i = 0; q < index; q++)
    {
        auto len = getUtf16CharLen(str[i]);
        i += len;
    }
    return std::u16string(str.substr(i, getUtf16CharLen(str[i])));
}

std::string unicode::toUtf8(std::u16string_view s)
{
    std::string result;
    auto cstr = s.data();
    size_t size = 0;
    char out[4];

    while (size < s.size())
    {
        auto generatedChar = char16to8(out, cstr + size);
        result.append(out, generatedChar);
        size += getUtf16CharLen(*(cstr + size));
    }

    return result;
}

std::string unicode::toUtf8(std::u32string_view s)
{
    std::string result;
    char out[4];

    for (auto c32 : s)
    {
        std::size_t size = char32to8(out, c32);
        result.append(out, size);
    }

    return result;
}

std::string unicode::toUtf8(std::wstring_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        std::u16string_view buf(reinterpret_cast<const char16_t*>(s.data()), s.size());
        return unicode::toUtf8(buf);
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        std::u32string_view buf(reinterpret_cast<const char32_t*>(s.data()), s.size());
        return unicode::toUtf8(buf);
    }
}

std::u16string unicode::toUtf16(std::string_view s)
{
    std::u16string result;
    auto cstr = s.data();
    size_t size = 0;
    char16_t c16[2];

    while (size < s.size())
    {
        size += char8to16(c16, cstr + size);
        result.append(c16, getUtf16CharLen(c16[0]));
    }

    return result;
}

std::u16string unicode::toUtf16(std::u32string_view s)
{
    std::u16string result;
    char16_t out[2];

    for (auto c32 : s)
    {
        std::size_t size = char32to16(out, c32);
        result.append(out, size);
    }

    return result;
}

std::u16string unicode::toUtf16(std::wstring_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        return std::u16string{ reinterpret_cast<const char16_t*>(s.data()), s.size() };
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        std::u32string_view buf(reinterpret_cast<const char32_t*>(s.data()), s.size());
        return unicode::toUtf16(buf);
    }
}

std::u32string unicode::toUtf32(std::string_view s)
{
    std::u32string result;
    auto cstr = s.data();
    char32_t c32;
    size_t size = 0;

    while (size < s.size())
    {
        size += char8to32(c32, cstr + size);
        result += c32;
    }

    return result;
}

std::u32string unicode::toUtf32(std::u16string_view s)
{
    std::u32string result;
    auto cstr = s.data();
    char32_t c32;
    size_t size = 0;

    while (size < s.size())
    {
        size += char16to32(c32, cstr + size);
        result += c32;
    }

    return result;
}

std::u32string unicode::toUtf32(std::wstring_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        std::u16string_view buf(reinterpret_cast<const char16_t*>(s.data()), s.size());
        return unicode::toUtf32(buf);
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        return std::u32string{ reinterpret_cast<const char32_t*>(s.data()), s.size() };
    }
}

std::wstring unicode::toWstring(std::string_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        auto result = unicode::toUtf16(s);
        return std::wstring(reinterpret_cast<const wchar_t*>(result.data()), result.size());
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        auto result = unicode::toUtf32(s);
        return std::wstring(reinterpret_cast<const wchar_t*>(result.data()), result.size());
    }
}

std::wstring unicode::toWstring(std::u16string_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        return std::wstring(reinterpret_cast<const wchar_t*>(s.data()), s.size());
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        auto result = unicode::toUtf32(s);
        return std::wstring(reinterpret_cast<const wchar_t*>(result.data()), result.size());
    }
}

std::wstring unicode::toWstring(std::u32string_view s)
{
    if constexpr (sizeof(wchar_t) == 2)
    {
        auto result = unicode::toUtf16(s);
        return std::wstring(reinterpret_cast<const wchar_t*>(s.data()), s.size());
    }
    else if constexpr (sizeof(wchar_t) == 4)
    {
        return std::wstring(reinterpret_cast<const wchar_t*>(s.data()), s.size());
    }
}
