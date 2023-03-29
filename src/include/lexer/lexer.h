#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include "string_enum.h"

namespace lexer
{
    STRING_ENUM(
        TokenType,
        BOOLEAN, REAL, NUMBER, STRING, NULL_,
        PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL, PERCENT_EQUAL, BACKSLASH_EQUAL,
        GREATER_EQUAL, LESS_EQUAL, EQUAL_EQUAL, NOT_EQUAL,
        PLUS, MINUS, SLASH, STAR, PERCENT, BACKSLASH, GREATER, LESS, AND, OR, NOT, EQUAL,
        END, WHILE, BREAK, CONTINUE, IF, ELSE_IF, ELSE, FUNCTION, RETURN, EACH, EACH_FROM,
        ID, LPAR, RPAR, COMMA, SEMICOLON, ELLIPSIS, DOT, SHEBANG, EOF_,
    );

    struct Token
    {
        TokenType tokenType;
        std::string text;
        size_t positionInLine; // Вказує на позицію символа в рядку, нумерація починається з одиниці
        size_t lineno; // Вказує на номер рядка, нумерація починається з одиниці
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
        // Якщо текст токена було змінено в ході токенізації,
        //  потрібно зберегти початкову позицію токена та передати її.
        void addToken(TokenType type, std::string text, size_t startPosition);
        [[noreturn]] void throwLexerError(std::string message, size_t lineno, size_t position);
    public:
        std::vector<Token> tokenize();
        Lexer(std::string code);
    };
};

#endif
