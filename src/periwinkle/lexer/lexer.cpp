

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

const static std::vector<std::pair<TokenType, regex>> tokenTypesRegexList
{
    {TokenType::BOOLEAN,       uregex("^(правда)|^(брехня)")},
    {TokenType::REAL,          uregex("^[-]?(([0-9]+[.][0-9]*)|([0-9]*[.][0-9]+))")},
    {TokenType::NUMBER,        uregex("^0|^(\\-?[1-9][0-9]*)")},
    {TokenType::NULL_,         uregex("^нич")},

    {TokenType::PLUS_EQUAL,    uregex("^\\+=")},
    {TokenType::MINUS_EQUAL,   uregex("^\\-=")},
    {TokenType::STAR_EQUAL,    uregex("^\\*=")},
    {TokenType::SLASH_EQUAL,   uregex("^/=")},
    {TokenType::PERCENT_EQUAL, uregex("^%=")},

    {TokenType::PLUS,          uregex("^\\+")},
    {TokenType::MINUS,         uregex("^\\-")},
    {TokenType::SLASH,         uregex("^/")},
    {TokenType::STAR,          uregex("^\\*")},
    {TokenType::PERCENT,       uregex("^%")},
    {TokenType::EQUAL,         uregex("^=")},

    {TokenType::END,           uregex("^кінець")},
    {TokenType::WHILE,         uregex("^поки")},

    {TokenType::ID,            uregex("^[а-яА-ЯїієґЇІЄҐ_][а-яА-ЯїієґЇІЄҐ0-9_]*")},
    {TokenType::LPAR,          uregex("^\\(")},
    {TokenType::RPAR,          uregex("^\\)")},
    {TokenType::COMMA,         uregex("^,")},
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
    }
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    {
        pos++;
        return true;
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
                /*if (kv.first == TokenType::REAL)
                {
                    for (auto& item : result)
                    {
                        for (auto& itemItem : item)
                        {
                            std::cout << itemItem << std::endl;
                        }
                    }
                }*/
                pos += str.size();
                addToken(kv.first, str);
                return true;
            }
        }
        vm::SyntaxException exception("Знайдено невідомий символ", utils::linenoFromPosition(code, pos));
        vm::throwSyntaxException(
            exception, code, utils::positionInLineFromPosition(code, pos));
        exit(1);
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
            vm::SyntaxException exception("Невідомий керуючий символ", utils::linenoFromPosition(code, pos));
            vm::throwSyntaxException(
                exception, code, utils::positionInLineFromPosition(code, pos+1));
            exit(1);
        }
        else if (current == "\"")
        {
            pos++;
            addToken(TokenType::STRING, buffer.str());
            break;
        }
        else if (current == "\0" || current == "\n")
        {
            vm::SyntaxException exception("Відсутні закриваючі лапки", utils::linenoFromPosition(code, pos));
            vm::throwSyntaxException(
                exception, code, utils::positionInLineFromPosition(code, startStringLiteral));
            exit(1);
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
    tokenList.push_back(Token{
        type, text, pos - text.size() + 1, utils::linenoFromPosition(code, pos) });
}

std::vector<Token> Lexer::tokenize()
{
    while (nextToken()) { };
    return tokenList;
}

Lexer::Lexer(std::string code): code(code), codeLength(code.length())
{
}
