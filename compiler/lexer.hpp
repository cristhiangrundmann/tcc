#pragma once
#include "table.hpp"

namespace tcc
{
    struct Lexer
    {
        const char *source{};
        const char *lexeme{};
        int length = 0;
        int lineno = 0;
        int column = 0;
        TokenType type = TokenType::UNDEFINED;
        double number{};
        Table *node{};
        Table *table{};

        void advance(bool match = true);
    };
}