#include <algorithm>

#include "unicode.hpp"
#include "unicode_database.hpp"

using namespace unicode;

inline unsigned char get_value_at_index(unsigned int index)
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
    auto idx = get_value_at_index(static_cast<unsigned int>(ch >= 0x110000 ? 0 : ch));
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

static int getUtf8CharLen(char c)
{
    auto uc = (unsigned char)c;
    if (uc <= 127) return 1;
    else if ((uc & 0xE0) == 0xC0) return 2;
    else if ((uc & 0xF0) == 0xE0) return 3;
    else if ((uc & 0xF8) == 0xF0) return 4;
    return -1;
}

// Першим аргументом приймає змінну, в яку записує результат конвертації,
// другим аргументом приймає посилання на стрічку.
// Повертає кількість char, які були сконвертовані
static inline int char8to32(char32_t& c32, const char* str)
{
    c32 = 0xff & (*str);
    auto length = getUtf8CharLen(*str);
    switch (length) {
    case 1:
        break;
    case 2:
        str++;
        c32 = ((c32 << 6) & 0x7ff) + ((*str) & 0x3f);
        break;
    case 3:
        str++;
        c32 = ((c32 << 12) & 0xffff) + ((0xff & (*str) << 6) & 0xfff);
        str++;
        c32 += (*str) & 0x3f;
        break;
    case 4:
        str++;
        c32 = ((c32 << 18) & 0x1fffff) + ((0xff & (*str) << 12) & 0x3ffff);
        str++;
        c32 += (0xff & (*str) << 6) & 0xfff;
        str++;
        c32 += (*str) & 0x3f;
        break;
    }
    str++;
    return length;
}

// Першим аргументом приймає змінну, в яку записує результат конвертації,
// другим аргументом приймає посилання на char32 символ.
// Повертає кількість char, які записані в змінній
static inline int char32to8(char* c8, char32_t c32)
{
    if (c32 < 0x80)
    {
        *(c8++) = c32;
        return 1;
    }
    else if (c32 < 0x800)
    {
        *(c8++) = (c32 >> 6) | 0xc0;
        *(c8++) = (c32 & 0x3f) | 0x80;
        return 2;
    }
    else if (c32 < 0x10000)
    {
        *(c8++) = (c32 >> 12) | 0xe0;
        *(c8++) = ((c32 >> 6) & 0x3f) | 0x80;
        *(c8++) = (c32 & 0x3f) | 0x80;
        return 3;
    }
    else
    {
        *(c8++) = (c32 >> 18) | 0xf0;
        *(c8++) = ((c32 >> 12) & 0x3f) | 0x80;
        *(c8++) = ((c32 >> 6) & 0x3f) | 0x80;
        *(c8++) = (c32 & 0x3f) | 0x80;
        return 4;
    }
}

size_t unicode::utf8Size(const std::string& str)
{
    size_t i, q;
    for (q = 0, i = 0; i < str.length(); q++)
    {
        i += getUtf8CharLen(str[i]);
    }
    return q;
}

std::string unicode::utf8At(const std::string& str, size_t index)
{
    size_t i, q;
    for (q = 0, i = 0; q < index; q++)
    {
        auto len = getUtf8CharLen(str[i]);
        i += len;
    }
    return str.substr(i, getUtf8CharLen(str[i]));
}

std::u32string unicode::utf8to32(const std::string& s)
{
    std::u32string result;
    auto cstr = s.c_str();
    size_t size = 0;

    while (size < s.size())
    {
        char32_t c32;
        size += char8to32(c32, cstr + size);
        result += c32;
    }

    return result;
}

std::string unicode::utf32to8(const std::u32string& s)
{
    std::string result;
    char out[4]{};

    for (auto c32 : s)
    {
        std::size_t size = char32to8(out, c32);
        result.append(out, size);
    }

    return result;
}

