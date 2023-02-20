#include <map>
#include <algorithm>
#define PCRE2_STATIC
#include <jpcre2.hpp>
#include "lexer.h"
#include "exception.h"

using namespace lexer;

typedef jpcre2::select<char> jp;
using regex = jp::Regex;
#define uregex(RE) regex( RE , PCRE2_UTF)

std::string Lexer::peek(size_t offset)
{
    size_t index = pos + offset;

    if (index >= codeLength)
    {
        return "\0";
    }

    return std::string(1, code.at(index));
}

// Аналогічно метасимволу b(працює лише з ascii символами), але для українських слів
#define WB "(?=[^а-яА-ЯїієґЇІЄҐ]|$)"

const static std::vector<std::pair<TokenType, regex>> tokenTypesRegexList
{
    {TokenType::BOOLEAN,         uregex("^((істина)|(хиба))" WB)},
    {TokenType::REAL,            uregex("^(([0-9]+[.][0-9]*)|([0-9]*[.][0-9]+))")},
    {TokenType::NUMBER,          uregex("^0|^([1-9][0-9]*)")},
    {TokenType::NULL_,           uregex("^нич")},

    {TokenType::PLUS_EQUAL,      uregex("^\\+=")},
    {TokenType::MINUS_EQUAL,     uregex("^\\-=")},
    {TokenType::STAR_EQUAL,      uregex("^\\*=")},
    {TokenType::SLASH_EQUAL,     uregex("^/=")},
    {TokenType::PERCENT_EQUAL,   uregex("^%=")},
    {TokenType::BACKSLASH_EQUAL, uregex("^\\\\=")},
    {TokenType::GREATER_EQUAL,   uregex("^більше=")},
    {TokenType::LESS_EQUAL,      uregex("^менше=")},
    {TokenType::EQUAL_EQUAL,     uregex("^==")},
    {TokenType::NOT_EQUAL,       uregex("^!=")},

    {TokenType::PLUS,            uregex("^\\+")},
    {TokenType::MINUS,           uregex("^\\-")},
    {TokenType::SLASH,           uregex("^/")},
    {TokenType::STAR,            uregex("^\\*")},
    {TokenType::PERCENT,         uregex("^%")},
    {TokenType::BACKSLASH,       uregex("^\\\\")},
    {TokenType::GREATER,         uregex("^більше" WB)},
    {TokenType::LESS,            uregex("^менше" WB)},
    {TokenType::AND,             uregex("^та" WB)},
    {TokenType::NOT,             uregex("^не" WB)},
    {TokenType::EQUAL,           uregex("^=")},

    {TokenType::END,             uregex("^кінець" WB)},
    {TokenType::WHILE,           uregex("^поки" WB)},
    {TokenType::BREAK,           uregex("^завершити" WB)},
    {TokenType::CONTINUE,        uregex("^продовжити" WB)},
    {TokenType::IF,              uregex("^якщо" WB)},
    {TokenType::ELSE_IF,         uregex("^або якщо" WB)},
    {TokenType::OR,              uregex("^або" WB)},
    {TokenType::ELSE,            uregex("^інакше" WB)},
    {TokenType::FUNCTION,        uregex("^функція" WB)},
    {TokenType::RETURN,          uregex("^повернути" WB)},

    {TokenType::ID,              uregex("^[а-яА-ЯїієґЇІЄҐ_][а-яА-ЯїієґЇІЄҐ0-9_]*")},
    {TokenType::LPAR,            uregex("^\\(")},
    {TokenType::RPAR,            uregex("^\\)")},
    {TokenType::COMMA,           uregex("^,")},
    {TokenType::SEMICOLON,       uregex("^;")},
};

bool Lexer::nextToken()
{
    if (pos >= codeLength)
    {
        addToken(TokenType::EOF_);
        return false;
    }

    auto current = peek(0);
    auto ahead = peek(1);

    switch (current.c_str()[0])
    {
    case '"':
    {
        tokenizeString();
        return true;
    }
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    {
        pos++;
        return true;
    }
    case '/':
    {
        if (ahead == "/")
        {
            readSingleLineComment();
            return true;
        }
        else if (ahead == "*")
        {
            readMultiLineComment();
            return true;
        }
        // Так як коментарі та оператор ділення визначені за допомогою одного символу,
        // блок "case '/':" має бути останнім та без "break",
        // щоб управління могло перейти до частини default
    }
    default:
    {
        auto subject = code.substr(pos);
        jp::VecNum result;
        jp::RegexMatch match;
        match.setSubject(&subject)
            .setNumberedSubstringVector(&result);
        for (const auto& kv : tokenTypesRegexList)
        {
            match.setRegexObject(&kv.second);
            auto count = match.match();
            if (count)
            {
                std::string str = result[0][0];
                pos += str.size();
                addToken(kv.first, str);
                return true;
            }
        }
        throwLexerError(
            "Знайдено невідомий символ",
            utils::linenoFromPosition(code, pos),
            utils::positionInLineFromPosition(code, pos) + 1);
    }
    }
}

const static std::vector<std::pair<EscapeSequencesType, regex>> escapeTypesRegexList
{
    {EscapeSequencesType::DOUBLE_QUOTE,    uregex("^\\\\\"")},
    {EscapeSequencesType::AUDIBLE_BELL,    uregex("^(\\\\a)|^(\\\\а)")},
    {EscapeSequencesType::BACKSPACE,       uregex("^(\\\\b)|^(\\\\б)")},
    {EscapeSequencesType::FORM_FEED,       uregex("^(\\\\f)|^(\\\\ф)")},
    {EscapeSequencesType::LINE_FEED,       uregex("^(\\\\n)|^(\\\\н)")},
    {EscapeSequencesType::CARRIAGE_RETURN, uregex("^(\\\\r)|^(\\\\р)")},
    {EscapeSequencesType::HORIZONTAL_TAB,  uregex("^(\\\\t)|^(\\\\т)")},
    {EscapeSequencesType::VERTIVAL_TAB,    uregex("^(\\\\v)|^(\\\\в)")},
    {EscapeSequencesType::NULL_CHARACTER,  uregex("^\\\\0")},
    {EscapeSequencesType::BACKSLASH,       uregex("^\\\\\\\\")},
};

void Lexer::tokenizeString()
{
    auto startStringLiteral = pos;
    pos++; // пропустити лапки
    std::stringstream buffer;
    jp::VecNum result;
    jp::RegexMatch match;

    for (;;)
    {
        continue_1: // Для того, щоб вийти з вкладеного циклу
        auto current = peek(0);
        if (current == "\\")
        {
            auto subject = code.substr(pos);
            match.setSubject(&subject)
                .setNumberedSubstringVector(&result);
            for (const auto& kv : escapeTypesRegexList)
            {
                match.setRegexObject(&kv.second);
                if (match.match())
                {
                    auto &str = result[0][0];
                    pos += (int)str.size();
                    buffer << (char)kv.first;
                    goto continue_1; // Вихід з вкладеного циклу
                }
            }
            throwLexerError("Невідомий керуючий символ",
                utils::linenoFromPosition(code, pos),
                utils::positionInLineFromPosition(code, pos + 1) + 1);
        }
        else if (current == "\"")
        {
            pos++;
            addToken(TokenType::STRING, buffer.str(), startStringLiteral);
            break;
        }
        else if (current == "\0" || current == "\n")
        {
            throwLexerError("Відсутні закриваючі лапки",
                utils::linenoFromPosition(code, pos),
                utils::positionInLineFromPosition(code, startStringLiteral) + 1);
        }
        else
        {
            buffer << current;
            pos++;
        }
    }
}

void Lexer::readSingleLineComment()
{
    pos += 2; // Пропустити скісні риски
    for (;;)
    {
        auto current = peek(0);
        if (current == "\0" || current == "\n" || current == "\r")
        {
            break;
        }
        pos++;
    }
}

void Lexer::readMultiLineComment()
{
    pos += 2; // Пропустити скісну риску та зірочку
    for (;;)
    {
        auto current = peek(0);
        auto ahead = peek(1);
        if (current == "*" && ahead == "/")
        {
            pos+=2;
            break;
        }
        pos++;
    }
}

void Lexer::addToken(TokenType type)
{
    addToken(type, "");
}

void Lexer::addToken(TokenType type, std::string text)
{
    // Поточна позиція вказує на кінець токена, тому потрібно відняти довжину токену
    auto startPosition = pos - text.size();
    addToken(type, text, startPosition);
}

void Lexer::addToken(TokenType type, std::string text, size_t startPosition)
{
    auto positionInLine = utils::positionInLineFromPosition(code, startPosition);
    auto lineno = utils::linenoFromPosition(code, startPosition);
    tokenList.push_back(Token{
        type, text, positionInLine + 1, lineno });
}

void Lexer::throwLexerError(std::string message, size_t lineno, size_t position)
{
    vm::SyntaxException exception(message, lineno, position);
    vm::throwSyntaxException(exception, code);
    exit(1);
}

std::vector<Token> Lexer::tokenize()
{
    while (nextToken()) { };
    return tokenList;
}

Lexer::Lexer(std::string code): code(code), codeLength(code.length())
{
}
