#ifndef STRING_VECTOR_OBJECT_H
#define STRING_VECTOR_OBJECT_H

#include <vector>

#include "object.hpp"

namespace vm
{
    extern TypeObject stringVectorObjectType;

    // Допоміжний об'єкт для передачі через стек списку зі стрічками
    struct StringVectorObject : Object
    {
        std::vector<std::string> value;

        static StringVectorObject* create(const std::vector<std::string>& value);
    };
}

#endif
