#include "table.hpp"

namespace tcc
{
    TokenType charToken(char c) { return static_cast<TokenType>(c); }

    int alphIndex(char c)
    {
        if(c >= 'a' && c <= 'z') return c - 'a' + 0;
        if(c >= 'A' && c <= 'Z') return c - 'A' + 26;
        if(c >= '0' && c <= '9') return c - '0' + 52;
        return -1;
    }

    const char *getTypeString(TokenType type)
    {
        static char c[4] = {'(', 0, ')', 0};
        if(static_cast<int>(type) < 256)
        {
            c[1] = static_cast<char>(type);
            return &c[0];
        }

        #define CASE(x) case TokenType::x: return #x;
        switch(type)
        {
            CASE(EOI)
            CASE(UNDEFINED)
            CASE(DECLARE)
            CASE(CONSTANT)
            CASE(VARIABLE)
            CASE(NUMBER)
            CASE(FUNCTION)
            default: return "??";
        }
        #undef CASE
    }

    Table *Table::next(char c)
    {
        int index = alphIndex(c);
        if(!children[index].get())
        {
            children[index] = std::make_unique<Table>();
            Table *t = children[index].get();
            t->parent = this;
            t->character = c;
            t->length = length+1;
        }
        return children[index].get();
    }

    Table *Table::procString(const char *text, bool match)
    {
        if(length) throw "Must call procString from root";

        Table *node = this;
        Table *lastMatch = nullptr;

        while(true)
        {
            int index = alphIndex(*text);
            if(index == -1) break;

            node = node->next(*text);
            if(match && node->type != TokenType::UNDEFINED) lastMatch = node;
            text++;
        }

        if(match && lastMatch) return lastMatch;
        return node;
    }

    Table *Table::initString(const char *text, TokenType type)
    {
        Table *t = procString(text, false);
        t->type = type;
        return t;
    }
}