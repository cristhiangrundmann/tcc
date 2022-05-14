#include "lexer.hpp"

namespace tcc
{

    void Lexer::advance(bool match)
    {
        number = 0;
        node = nullptr;
        lexeme += length;
        
        while(true)
        {
            while(*lexeme == ' ' || *lexeme == '\n' || *lexeme == '\t')
            {
                if(*lexeme == '\n') lineno++, column = 0;
                else column++;
                lexeme++;
            }
            if(*lexeme != '#') break;

            type = TokenType::COMMENT;
            length = 0;
            while(lexeme[length] != '\n' && lexeme[length] != 0) length++;
            return;
        }

        if(*lexeme == 0)
        {
            length = 0;
            type = TokenType::EOI;
        }
        else
        {
            if((*lexeme >= '0' && *lexeme <= '9') || *lexeme == '.')
            {
                type = TokenType::NUMBER;
                int k = sscanf(lexeme, "%lf%n\n", &number, &length);
                if(k != 1) throw "sscanf failure";
                return;
            }

            int index = alphIndex(*lexeme);
            if(index == -1)
            {
                length = 1;
                type = charToken(*lexeme);
                return;
            }

            node = table->procString(lexeme, match);
            length = node->length;
            type = node->type;
        }
    }
}