#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include "string_enum.h"

namespace lexer
{
    STRING_ENUM(
        TokenType,
        BOOLEAN, REAL, NUMBER, STRING, NULL_,
        PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL,
        PLUS, MINUS, SLASH, STAR, PERCENT, EQUAL,
        END, WHILE,
        ID, LPAR, RPAR, COMMA, EOF_,
    );

    struct Token
    {
        TokenType tokenType;
        std::string text;
        size_t position;
        int lineno;
    };

    enum class EscapeSequencesType: char
    {
        DOUBLE_QUOTE    = '"',
        AUDIBLE_BELL    = '\a',
        BACKSPACE       = '\b',
        FORM_FEED       = '\f',
        LINE_FEED       = '\n',
        CARRIAGE_RETURN = '\r',
        HORIZONTAL_TAB  = '\t',
        VERTIVAL_TAB    = '\v',
        NULL_CHARACTER  = '\0',
        BACKSLASH       = '\\',
    };

    class Lexer
    {
    private:
        size_t pos = 0;
        std::string code;
        const size_t codeLength;
        std::vector<Token> tokenList;

        std::string peek(size_t offset);
        bool nextToken();
        void tokenizeString();
        void readSingleLineComment();
        void readMultiLineComment();
        void addToken(TokenType type);
        void addToken(TokenType type, std::string text);
    public:
        std::vector<Token> tokenize();
        Lexer(std::string code);
    };
};

#endif
