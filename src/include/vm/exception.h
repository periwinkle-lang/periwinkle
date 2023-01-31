#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <iostream>
#include <string>
#include <sstream>
#include "utils.h"

namespace vm
{
    class Exception
    {
    protected:
        std::string message;
        size_t lineno;
    public:
        size_t getLineno() const { return lineno; };
        virtual std::string toString() const = 0;
        Exception(std::string message, size_t lineno) : message(message), lineno(lineno) {};
    };

    class SyntaxException : public Exception
    {
    private:
        size_t position;
    public:
        virtual std::string toString() const final;
        size_t getPosition() const { return position; }
        SyntaxException(std::string message, size_t lineno, size_t position)
            : Exception(message, lineno), position(position) {};

    };

    void throwSyntaxException(const SyntaxException& exception, const std::string& code);
}

#endif
