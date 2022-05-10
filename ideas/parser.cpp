#include "parser.hpp"
#include <stdio.h>
#include <stdlib.h>

namespace tcc
{
    Parser::Parser(const char *source)
    {
        lexer.source = source;
        lexer.lexeme = source;
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
        actAdvance();
        lexer.advance(mode);
    }
    
    void Parser::parseProgram()
    {
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
        actInt();
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
        actIGrid();
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
        actInt();
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
        Table *node = lexer.node;
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

        node->type = TokenType::FUNCTION;
        node->argsIndex = (int)argList.size();
        argList.push_back(args);
        Lexer tmp = lexer;
        lexer.node = node;
        actFDecl();
        lexer = tmp;
    }

    void Parser::parseParam()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        Table *node = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseInts();
        require(charToken(';'));
        advance();
        node->type = TokenType::VARIABLE;
        Lexer tmp = lexer;
        actParam();
        lexer = tmp;
    }

    void Parser::parseGrid()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        Table *node = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseIGrids();
        require(charToken(';'));
        advance();
        node->type = TokenType::VARIABLE;
        Lexer tmp = lexer;
        actGrid();
        lexer = tmp;
    }

    void Parser::parseDefine()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        Table *node = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        node->type = TokenType::VARIABLE; 
        Lexer tmp = lexer;
        actDefine();
        lexer = tmp;    
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
        actCurve();
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
        actSurface();
    }

    void Parser::parseFunction()
    {
        advance(Table::Mode::INSERT);
        parseFDecl();
        require(charToken(';'));
        advance();
        removeArgs();
        actFunction();
    }

    void Parser::parsePoint()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        Table *node = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        node->type = TokenType::VARIABLE;
        Lexer tmp = lexer;
        actPoint();
        lexer = tmp;   
    }

    void Parser::parseVector()
    {
        advance(Table::Mode::INSERT);
        require(TokenType::UNDEFINED);
        Table *node = lexer.node;
        advance();
        require(charToken('='));
        advance();
        parseExpr();
        require(charToken('@'));
        advance();
        parseExpr();
        require(charToken(';'));
        advance();   
        node->type = TokenType::VARIABLE;
        Lexer tmp = lexer;
        actVector();
        lexer = tmp;   
    }

    void Parser::parseDecl()
    {
        require(TokenType::DECLARE);
        
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
                actPlus();
            }
            else if(compare(charToken('-')))
            {
                advance();
                parseJux();
                actMinus();
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
            actJux();
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
                actTimes();
            }
            else if(compare(charToken('/')))
            {
                advance();
                parseUnary();
                actDivide();
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
            actUPlus();
        }
        else if(compare(charToken('-')))
        {
            advance();
            parseUnary();
            actUMinus();
        }
        else if(compare(charToken('*')))
        {
            advance();
            parseUnary();
            actUTimes();
        }
        else if(compare(charToken('/')))
        {
            advance();
            parseUnary();
            actUDivide();
        }
        else parseApp();
    }

    void Parser::parseApp()
    {
        if(compare(TokenType::FUNCTION))
        {
            parseFunc();
            parseUnary();
            actFunc();
        }
        else parsePow();
    }

    void Parser::parseFunc()
    {
        require(TokenType::FUNCTION);
        Table *node = lexer.node;
        advance();

        while(true)
        {
            if(compare(charToken('\'')))
            {
                advance();
                actTotal();
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
                actPartial();
                advance();
            }
            else break;
        }

        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actFPow();
        }
    }

    void Parser::parsePow()
    {
        parseComp();

        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actPow();
        }
    }

    void Parser::parseComp()
    {
        parseFact();

        while(compare(charToken('_')))
        {
            advance();
            require(TokenType::NUMBER);
            actComp();
            advance();
        }
    }

    void Parser::parseFact()
    {
        if(compare(TokenType::CONSTANT))
        {
            actConst();
            advance();
        }
        else if(compare(TokenType::NUMBER))
        {
            actNumber();
            advance();
        }
        else if(compare(TokenType::VARIABLE))
        {
            actVar();
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
            actTuple();
        }
        require(charToken(')'));
        advance();
    }

    #define DUMMY(x) void Parser::x() {}
        DUMMY(actAdvance) 
        DUMMY(actInt) DUMMY(actIGrid) DUMMY(actTInt)
        DUMMY(actFDecl) DUMMY(actParam) DUMMY(actGrid) 
        DUMMY(actDefine) DUMMY(actCurve) DUMMY(actSurface)
        DUMMY(actFunction) DUMMY(actPoint) DUMMY(actVector)
        DUMMY(actPlus) DUMMY(actMinus) DUMMY(actJux) 
        DUMMY(actTimes) DUMMY(actDivide) DUMMY(actUPlus) 
        DUMMY(actUMinus) DUMMY(actUTimes) DUMMY(actUDivide)
        DUMMY(actFunc) DUMMY(actTotal) DUMMY(actPartial) 
        DUMMY(actFPow) DUMMY(actPow) DUMMY(actComp) DUMMY(actConst)
        DUMMY(actNumber) DUMMY(actVar) DUMMY(actTuple)
    #undef DUMMY

    void Parser::comment(int start, int end) {}

    void Parser::syntaxError(TokenType type) 
    {
        printf("Syntax error: expected %s instead of %s\n", 
                getTypeString(type), getTypeString(lexer.type));
        throw 0;
    }
}

using namespace tcc;

struct Tester : public Parser
{
    Tester(const char *source) : Parser(source) {}
    void actParam()
    {
        printf("Param defined!\n");
    }
};

int main()
{
    Tester t("param R = [0, 10];");
    t.parseProgram();
}