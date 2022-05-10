#include "lexer.hpp"

namespace tcc
{

    void Lexer::advance(Table::Mode mode)
    {
        number = 0;
        node = nullptr;
        lexeme += length;
        
        while(true)
        {
            while(*lexeme == ' ' || *lexeme == '\n' || *lexeme == '\t')
            {
                if(*lexeme == '\n') lineno++, column=1;
                else column++;
                lexeme++;
            }
            if(*lexeme != '#') break;
            int start = lexeme-source;
            while(*lexeme != '\n' && *lexeme != 0) lexeme++;
            int end = lexeme-source;
            if(commentCallback) commentCallback(start, end);
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

            node = table->procString(lexeme, mode);
            length = node->length;
            type = node->type;

            if(length == 0) length = 1; //???
        }
    }
}