#ifndef KEYWORD_H
#define KEYWORD_H

#include <string_view>

#define KW const constexpr std::string_view

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
    KW FLOOR_DIV = "//";
    KW FLOOR_DIV_EQUAL = "//=";
    KW MOD = "%";
    KW MOD_EQUAL = "%=";
    KW EQUAL = "=";
    KW EQUAL_EQUAL = "рівно";
    KW AND = "та";
    KW OR = "або";
    KW IS = "є";
    KW IS_NOT = "не є";
    KW LESS = "менше";
    KW LESS_EQUAL = "менше рівно";
    KW GREATER = "більше";
    KW GREATER_EQUAL = "більше рівно";
    KW NOT = "не";
    KW NOT_EQUAL = "нерівно";
    KW POS = "+";
    KW NEG = "-";
    KW K_TRUE = "істина";
    KW K_FALSE = "хиба";
    KW K_NULL = "ніц";
    KW K_NAN = "неЧисло";
    KW K_INF = "нескінченність";
}

#undef KW
#endif
