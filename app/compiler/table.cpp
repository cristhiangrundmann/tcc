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

    std::string getTypeString(TokenType type)
    {
        if(static_cast<int>(type) < 256)
        {
            char c = static_cast<char>(type);
            return std::string("`") + c + "`";
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
            t->str = str + c;
            if(t->length > 64) throw std::string("Identifier is too big");
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