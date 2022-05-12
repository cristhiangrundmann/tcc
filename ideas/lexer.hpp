#pragma once
#include "table.hpp"

namespace tcc
{
    struct Lexer
    {
        const char *source = nullptr;
        const char *lexeme = nullptr;
        int length = 0;
        int lineno = 0;
        int column = 0;
        TokenType type;
        double number;
        Table *node = nullptr;
        Table *table = nullptr;

        void advance(Table::Mode mode = Table::Mode::MATCH);
    };
}