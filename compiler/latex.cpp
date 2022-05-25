#include "parser.hpp"
#include "style.hpp"
#include <stdio.h>

using namespace tcc;

struct LaTeX : public Parser
{
    const char *current = nullptr;

    void actAdvance()
    {
        if(lexer.length == 0) return;
        for(; current < lexer.lexeme; current++)
        {
            printf("%c", *current);
        }

        Style style;

        switch(lexer.type)
        {
            case TokenType::UNDEFINED: style = Style::UNDEFINED; break;
            case TokenType::DECLARE: style = Style::DECLARE; break;
            case TokenType::CONSTANT: style = Style::CONSTANT; break;
            case TokenType::VARIABLE: style = Style::VARIABLE; break;
            case TokenType::NUMBER: style = Style::NUMBER; break;
            case TokenType::FUNCTION: style = Style::FUNCTION; break;
            case TokenType::COMMENT: style = Style::COMMENT; break;
            default: style = Style::SYMBOL;
        }

        current += lexer.length;

        printf("|");

        const char *str;

        #define CASE(x) case Style::x: str = #x; break;

        switch(style)
        {
            CASE(SYMBOL);
            CASE(COMMENT);
            CASE(DECLARE);
            CASE(FUNCTION);
            CASE(CONSTANT);
            CASE(VARIABLE);
            CASE(NUMBER);
            CASE(UNDEFINED);
            default: str = "SYMBOL";
        }

        #undef CASE

        printf("\\style%s{%.*s}", str, lexer.length, lexer.lexeme);

        printf("|");
    }
};

#define DEFINE(x) { printf("\\definecolor{styleColor%s}{rgb}{%g, %g, %g}\n",\
#x, ST_##x.r/255.0, ST_##x.g/255.0, ST_##x.b/255.0);\
printf("\\newcommand{\\style%s}[1]{%s%s\\textcolor{styleColor%s}{#1}%s%s}\n",\
#x, ST_##x.f & 1 ? "\\textbf{" : "", ST_##x.f & 2 ? "\\textit{" : "",\
#x,\
ST_##x.f & 2 ? "}" : "", ST_##x.f & 1 ? "}" : ""); }

int main(int argc, char *argv[])
{
    if(argc == 1)
    {
        DEFINE(SYMBOL);
        DEFINE(COMMENT);
        DEFINE(DECLARE);
        DEFINE(FUNCTION);
        DEFINE(CONSTANT);
        DEFINE(VARIABLE);
        DEFINE(NUMBER);
        DEFINE(UNDEFINED);
    } 
    else if(argc == 2)
    {
        LaTeX latex;

        try
        {
            latex.current = argv[1];
            latex.parseProgram(argv[1]);
        } 
        catch(const char *error)
        {
            printf("%s\n", error);
        }
    }

    return 0;
}

#undef DEFINE