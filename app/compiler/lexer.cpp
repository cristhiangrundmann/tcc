#include "lexer.hpp"

namespace tcc
{

    void Lexer::advance(bool match)
    {
        number = 0;
        node = nullptr;
        lexeme += length;
        column += length;
        
        while(true)
        {
            //handle white-spaces
            while(*lexeme == ' ' || *lexeme == '\n' || *lexeme == '\t')
            {
                if(*lexeme == '\n') lineno++, column = 1;
                else column++;
                lexeme++;
            }
            if(*lexeme != '#') break;
            //handle comments
            type = TokenType::COMMENT;
            length = 0;
            while(lexeme[length] != '\n' && lexeme[length] != 0) length++;
            return;
        }

        //handle ENF OF INPUT
        if(*lexeme == 0)
        {
            length = 0;
            type = TokenType::EOI;
        }
        else
        {
            //handle numbers
            if((*lexeme >= '0' && *lexeme <= '9') || *lexeme == '.')
            {
                type = TokenType::NUMBER;
                int k = sscanf(lexeme, "%f%n\n", &number, &length);
                if(k != 1) throw "sscanf failure";
                return;
            }

            //handle identifiers
            int index = alphIndex(*lexeme);
            if(index == -1)
            {
                //handle single character symbols
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