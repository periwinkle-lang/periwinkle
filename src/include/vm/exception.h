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
        int lineno;
    public:
        int getLineno() const { return lineno; };
        virtual std::string toString() = 0;
        Exception(std::string message, int lineno) : message(message), lineno(lineno) {};
    };

    class SyntaxException : public Exception
    {
    public:
        virtual std::string toString() final;
        SyntaxException(std::string message, int lineno) : Exception(message, lineno) {};
    };

    void throwSyntaxException(SyntaxException exception, const std::string& code, size_t position = 0);
}

#endif
