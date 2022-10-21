#pragma once
#include "table.hpp"

namespace tcc
{
    /*
        Lexer
    
    Reads a token from input string
    source: pointer to start of input
    lexeme: points to start of current token inside `source'
    length: length of current token, in chars
    lineno: line number of token
    column: column number of token
    type: type of current token
    number: number as afloat (if token is NUMBER)
    node: pointer to symble table (if token is a `a-zA-Z0-9' string)
    table: root os symble table, which is a trie

    advance: advances to next token
        match: passed along
    */
    struct Lexer
    {
        const char *source{};
        const char *lexeme{};
        int length = 0;
        int lineno = 0;
        int column = 0;
        TokenType type = TokenType::UNDEFINED;
        float number{};
        Table *node{};
        Table *table{};

        void advance(bool match = true);
    };
}