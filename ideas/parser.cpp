#include "parser.hpp"
#include <stdio.h>
#include <stdlib.h>

namespace tcc
{
    Parser::Parser()
    {
        lexer.table = table.get();
    }

    void Parser::require(TokenType type)
    {
         if(!compare(type)) syntaxError(type);
    }

    bool Parser::compare(TokenType type)
    {
        return lexer.type == type;
    }

    void Parser::advance(Table::Mode mode)
    {
        do
        {
            actAdvance();
            lexer.advance(mode);
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

    void Parser::parseInt()
    {
        require(charToken('['));
        advance();
        parseExpr();
        require(charToken(','));
        advance();
        parseExpr();
        require(charToken(']'));
        advance();
        actInt('i');
    }

    void Parser::parseTInts()
    {
        parseTInt();
        if(compare(charToken(',')))
        {
            advance();
            parseTInt();
        }
    }

    void Parser::parseIGrid()
    {
        require(charToken('['));
        advance();
        parseExpr();
        require(charToken(','));
        advance();
        parseExpr();
        require(charToken(','));
        advance();
        parseExpr();
        require(charToken(']'));
        advance();
        actInt('g');
    }

    void Parser::parseIGrids()
    {
        parseIGrid();
        if(compare(charToken(',')))
        {
            advance();
            parseIGrid();
        }
    }

    void Parser::parseTInt()
    {
        require(TokenType::VARIABLE);
        advance();
        require(charToken(':'));
        advance();
        if(compare(charToken('+')) || compare(charToken('-'))) advance();
        parseInt();
        actInt('t');
    }

    void Parser::parseInts()
    {
        parseInt();
        if(compare(charToken(',')))
        {
            advance();
            parseInt();
        }
    }

    void Parser::removeArgs()
    {
        if(argList.size()) for(Table *t : argList.back()) t->type = TokenType::UNDEFINED;
    }

    void Parser::parseFDecl()
    {
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        lexer.type = TokenType::FUNCTION;
        advance();
        require(charToken('('));
        advance(Table::Mode::INSERT);
        std::vector<Table*> args;

        while(true)
        {
            require(TokenType::UNDEFINED);
            Table *arg = lexer.node;
            arg->type = TokenType::VARIABLE;
            args.push_back(arg);
            advance();

            if(!compare(charToken(','))) break;
            advance(Table::Mode::INSERT);
        }

        require(charToken(')'));
        advance();

        require(charToken('='));
        advance();

        parseExpr();

        objName->type = TokenType::FUNCTION;
        objName->argsIndex = (int)argList.size();
        argList.push_back(args);
    }

    void Parser::parseParam()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        advance();
        require(charToken(':'));
        advance();
        parseInts();
        require(charToken(';'));
        advance();
        objName->type = TokenType::VARIABLE;
        actDecl();
    }

    void Parser::parseGrid()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        advance();
        require(charToken(':'));
        advance();
        parseIGrids();
        require(charToken(';'));
        advance();
        objName->type = TokenType::VARIABLE;
        actDecl();
    }

    void Parser::parseDefine()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        objName->type = TokenType::VARIABLE; 
        actDecl();
    }

    void Parser::parseCurve()
    {
        advance(Table::Mode::INSERT);
        parseFDecl();
        require(charToken(','));
        advance();
        parseTInts();
        require(charToken(';'));
        advance();
        removeArgs();
        actDecl();
    }

    void Parser::parseSurface()
    {
        advance(Table::Mode::INSERT);
        parseFDecl();
        require(charToken(','));
        advance();
        parseTInts();
        require(charToken(';'));
        advance();
        removeArgs();
        actDecl();
    }

    void Parser::parseFunction()
    {
        advance(Table::Mode::INSERT);
        parseFDecl();
        require(charToken(';'));
        advance();
        removeArgs();
        actDecl();
    }

    void Parser::parsePoint()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        objName->type = TokenType::VARIABLE;
        actDecl();
    }

    void Parser::parseVector()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        objName = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken('@'));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        objName->type = TokenType::VARIABLE;
        actDecl();
    }

    void Parser::parseDecl()
    {
        require(TokenType::DECLARE);
        
        objType = lexer.node;
        
        if(lexer.node == param)           parseParam();
        else if(lexer.node == grid)       parseGrid();
        else if(lexer.node == define)     parseDefine();
        else if(lexer.node == curve)      parseCurve();
        else if(lexer.node == surface)    parseSurface();
        else if(lexer.node == function)   parseFunction();
        else if(lexer.node == point)      parsePoint();
        else if(lexer.node == vector)     parseVector();
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
        multUnary = true;
        parseMult();

        while(compare(TokenType::FUNCTION) || compare(TokenType::CONSTANT)
        || compare(TokenType::NUMBER) || compare(TokenType::VARIABLE) || compare(charToken('(')))
        {
            multUnary = false;
            parseMult();
            actBinary(' ');
        }
    }

    void Parser::parseMult()
    {
        if(multUnary) parseUnary();
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
            actBinary('+');
        }
        else if(compare(charToken('-')))
        {
            advance();
            parseUnary();
            actBinary('-');
        }
        else if(compare(charToken('*')))
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
        else parseApp();
    }

    void Parser::parseApp()
    {
        if(compare(TokenType::FUNCTION))
        {
            parseFunc();
            parseUnary();
            actBinary('f');
        }
        else parsePow();
    }

    void Parser::parseFunc()
    {
        require(TokenType::FUNCTION);
        Table *node = lexer.node;

        actUnary('f');

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
                advance(Table::Mode::MAX);
                bool ok = false;

                if(node->argsIndex != -1 && lexer.node)
                {
                    while(true)
                    {
                        for(Table *arg : argList[node->argsIndex])
                        {
                            if(arg == lexer.node)
                            {
                                ok = true;
                                break;
                            }
                        }
                        if(ok) break;

                        if(lexer.node->parent)
                        {
                            lexer.node = lexer.node->parent;
                            if(lexer.length) lexer.length--;
                        }
                        else break;
                    }

                }
                if(!ok)
                {
                    lexer.length = 1;
                    syntaxError(TokenType::VARIABLE);
                }
                lexer.type = TokenType::FUNCTION;
                actBinary('d');
                advance();
            }
            else break;
        }

        if(compare(charToken('e')))
        {
            advance();
            parseUnary();
            actBinary('e');
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
            actBinary('_');
            advance();
        }
    }

    void Parser::parseFact()
    {
        if(compare(TokenType::CONSTANT))
        {
            actUnary('c');
            advance();
        }
        else if(compare(TokenType::NUMBER))
        {
            actUnary('n');
            advance();
        }
        else if(compare(TokenType::VARIABLE))
        {
            actUnary('v');
            advance();
        }
        else parseTuple();
    }

    void Parser::parseTuple()
    {
        require(charToken('('));
        advance();
        parseAdd();
        while(compare(charToken(',')))
        {
            advance();
            parseAdd();
            actBinary(',');
        }
        require(charToken(')'));
        advance();
    }

    void Parser::actAdvance() {}
    void Parser::actDecl() {}
    void Parser::actInt(char c) {}
    void Parser::actBinary(char c) {}
    void Parser::actUnary(char c) {}

    void Parser::syntaxError(TokenType type) 
    {
        static char msg[128];
        sprintf(msg, "Syntax error: expected %s instead of %s\n", 
                getTypeString(type), getTypeString(lexer.type));

        throw msg;
    }
}