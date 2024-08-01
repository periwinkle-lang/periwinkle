#ifndef STRING_OBJECT_H
#define STRING_OBJECT_H

#include <optional>

#include "object.hpp"

namespace vm
{
    extern TypeObject stringObjectType, stringIterObjectType;

    struct StringObject : Object
    {
        std::u32string value;

        std::string asUtf8() const;
        static StringObject* create(const std::string& value);
        static StringObject* create(const std::u32string& value);
    };

    extern StringObject P_emptyStr;
    std::optional<i64> stringObjectToInt(StringObject* str, int base = 10);

    struct StringIterObject : Object
    {
        size_t position;
        size_t length;
        std::u32string iterable;

        static StringIterObject* create(const std::u32string& iterable);
    };
}

#endif
