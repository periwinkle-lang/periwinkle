#ifndef KEYWORD_H
#define KEYWORD_H

#include <string_view>

#define KW static constexpr auto

namespace Keyword
{
    KW ADD = "+";
    KW ADD_EQUAL = "+=";
    KW SUB = "-";
    KW SUB_EQUAL = "-=";
    KW MUL = "*";
    KW MUL_EQUAL = "*=";
    KW DIV = "/";
    KW DIV_EQUAL = "/=";
    KW FLOOR_DIV = "\\";
    KW FLOOR_DIV_EQUAL = "\\=";
    KW MOD = "%";
    KW MOD_EQUAL = "%=";
    KW EQUAL = "=";
    KW EQUAL_EQUAL = "==";
    KW AND = "та";
    KW OR = "або";
    KW IS = "є";
    KW LESS = "менше";
    KW LESS_EQUAL = "менше=";
    KW GREATER = "більше";
    KW GREATER_EQUAL = "більше=";
    KW NOT = "не";
    KW NOT_EQUAL = "!=";
}

#undef KW
#endif
