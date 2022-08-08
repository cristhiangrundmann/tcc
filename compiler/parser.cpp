#include "parser.hpp"
#include <stdio.h>
#include <stdlib.h>

namespace tcc
{
    #define compare(x) (lexer.type == x)
    #define require(x) {if(!compare(x)) syntaxError(x);}
    #define skip(x) {require(charToken(x)); advance();}

    Parser::Parser()
    {
        lexer.table = table.get();
    }

    void Parser::advance(bool match)
    {
        do
        {
            actAdvance();
            lexer.advance(match);
        } while(lexer.type == TokenType::COMMENT);
    }
    
    void Parser::parseProgram(const char *source)
    {
        lexer.source = source;
        lexer.lexeme = source;
        lexer.length = 0;
        lexer.lineno = 0;
        lexer.column = 1;

        advance();
        while(!compare(TokenType::EOI)) parseDecl();
    }

    void Parser::parseInt(char type)
    {
        switch(type)
        {
            wrap = 0;
            tag = {};
            case 't':
                require(TokenType::UNDEFINED);
                tag = lexer.node;
                advance();
                skip(':');
                if(compare(charToken('+')) || compare(charToken('-')))
                {
                    wrap = (char)lexer.type;
                    advance();
                }
            case 'i':
                skip('[');
                parseExpr();
                skip(',');
                parseExpr();
                skip(']');
                break;
            case 'g':
                skip('[');
                parseExpr();
                skip(',');
                parseExpr();
                skip(',');
                parseExpr();
                skip(']');
                break;
        }
        actInt(type);
    }

    void Parser::parseInts(char type)
    {
        if(type == 't')
        {
            size_t size = argList.back().size();
            for(size_t i = 0; i < size; i++)
            {
                parseInt(type);
                if(tag != argList.back()[i])
                    throw std::string("Inconsistent argument list");
                if(i != size-1)
                {
                    require(charToken(','));
                    advance(false);
                }
            }
        }
        else
        {
            parseInt(type);
            if(compare(charToken(',')))
            {
                advance();
                parseInt(type);
            }
        }
    }

    void Parser::parseFDecl()
    {
        lexer.type = TokenType::FUNCTION;
        advance();
        require(charToken('('));
        advance(false);
        std::vector<Table*> args;

        while(true)
        {
            require(TokenType::UNDEFINED);
            Table *arg = lexer.node;
            arg->type = TokenType::VARIABLE;
            args.push_back(arg);
            advance();
            if(!compare(charToken(','))) break;
            advance(false);
        }

        skip(')');
        skip('=');
        parseExpr();

        objName->argsIndex = (int)argList.size();
        argList.push_back(args);
        objName->type = TokenType::FUNCTION;
        if(argList.size()) for(Table *t : argList.back()) t->type = TokenType::UNDEFINED;
    }

    void Parser::parseParam()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip(':');
        parseInts('i');
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseGrid()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip(':');
        parseInts('g');
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseDefine()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip('=');
        parseExpr();
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseCurve()
    {
        parseFDecl();
        require(charToken(','));
        advance(false);
        parseInts('t');
    }

    void Parser::parseSurface()
    {
        parseFDecl();
        require(charToken(','));
        advance(false);
        parseInts('t');
    }

    void Parser::parseFunction()
    {
        parseFDecl();
    }

    void Parser::parsePoint()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip('=');
        parseExpr();
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseVector()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip('=');
        parseExpr();
        skip('@');
        parseExpr();
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseDecl()
    {
        require(TokenType::DECLARE);
        objType = lexer.node;
        advance(false);
        require(TokenType::UNDEFINED);
        objName = lexer.node;

        if(objType == param)           parseParam();
        else if(objType == grid)       parseGrid();
        else if(objType == define)     parseDefine();
        else if(objType == curve)      parseCurve();
        else if(objType == surface)    parseSurface();
        else if(objType == function)   parseFunction();
        else if(objType == point)      parsePoint();
        else if(objType == vector)     parseVector();

        skip(';');
        actDecl();
    }

    void Parser::parseExpr()
    {
        parseAdd();
    }

    void Parser::parseAdd()
    {
        parseJux();

        while(true)
        {
            if(compare(charToken('+')))
            {
                advance();
                parseJux();
                actBinary('+');
            }
            else if(compare(charToken('-')))
            {
                advance();
                parseJux();
                actBinary('-');
            }
            else break;
        }
    }

    void Parser::parseJux()
    {
        parseMult(true);
        while(compare(TokenType::FUNCTION) || compare(TokenType::CONSTANT)
        || compare(TokenType::NUMBER) || compare(TokenType::VARIABLE) || compare(charToken('(')))
        {
            parseMult(false);
            actBinary('j');
        }
    }

    void Parser::parseMult(bool unary)
    {
        if(unary) parseUnary();
        else parseApp();

        while(true)
        {
            if(compare(charToken('*')))
            {
                advance();
                parseUnary();
                actBinary('*');
            }
            else if(compare(charToken('/')))
            {
                advance();
                parseUnary();
                actBinary('/');
            }
            else break;            
        }
    }

    void Parser::parseUnary()
    {
        if(compare(charToken('+')))
        {
            advance();
            parseUnary();
            actUnary('P');
        }
        else if(compare(charToken('-')))
        {
            advance();
            parseUnary();
            actUnary('M');
        }
        else if(compare(charToken('*')))
        {
            advance();
            parseUnary();
            actUnary('T');
        }
        else if(compare(charToken('/')))
        {
            advance();
            parseUnary();
            actUnary('D');
        }
        else parseApp();
    }

    void Parser::parseApp()
    {
        if(compare(TokenType::FUNCTION))
        {
            parseFunc();
            parseUnary();
            actBinary('A');
        }
        else parsePow();
    }

    void Parser::parseFunc()
    {
        require(TokenType::FUNCTION);
        Table *node = lexer.node;

        actUnary('F');
        advance();

        while(true)
        {
            if(compare(charToken('\'')))
            {
                advance();
                actUnary('\'');
            }
            else if(compare(charToken('_')))
            {
                advance(false);
                int l = lexer.length;
                bool ok = false;

                if(node->argsIndex != -1 && lexer.node) while(true)
                {
                    for(Table *arg : argList[node->argsIndex]) if(arg == lexer.node)
                    {
                        ok = true;
                        break;
                    }
                    if(ok) break;
                    if(lexer.node->parent)
                    {
                        lexer.node = lexer.node->parent;
                        if(lexer.length) lexer.length--;
                    }
                    else break;
                }

                if(!ok)
                {
                    lexer.length = l;
                    throw std::string("Expected a function variable");
                }
                lexer.type = TokenType::FUNCTION;
                actBinary('_');
                advance();
            }
            else break;
        }

        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actBinary('E');
        }
    }

    void Parser::parsePow()
    {
        parseComp();
        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actBinary('^');
        }
    }

    void Parser::parseComp()
    {
        parseFact();
        while(compare(charToken('_')))
        {
            advance();
            require(TokenType::NUMBER);
            actBinary('.');
            advance();
        }
    }

    void Parser::parseFact()
    {
        if(compare(TokenType::CONSTANT))
        {
            actUnary('C');
            advance();
        }
        else if(compare(TokenType::NUMBER))
        {
            actUnary('N');
            advance();
        }
        else if(compare(TokenType::VARIABLE))
        {
            actUnary('V');
            advance();
        }
        else parseTuple();
    }

    void Parser::parseTuple()
    {
        skip('(');
        parseAdd();
        while(compare(charToken(',')))
        {
            advance();
            parseAdd();
            actBinary(',');
        }
        skip(')');
    }

    #define UNUSED __attribute__((unused))
        void Parser::actAdvance() {}
        void Parser::actDecl() {}
        void Parser::actInt(UNUSED char type) {}
        void Parser::actBinary(UNUSED char type) {}
        void Parser::actUnary(UNUSED char type) {}
    #undef UNUSED

    void Parser::syntaxError(TokenType type) 
    {
        std::string s1 = getTypeString(type);
        std::string s2 = getTypeString(lexer.type);
        throw std::string("Syntax error: expected " + s1 + " instead of " + s2);
    }

    #undef skip
    #undef require
    #undef compare
}