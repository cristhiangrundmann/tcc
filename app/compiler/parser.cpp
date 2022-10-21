#include "parser.hpp"
#include <stdio.h>
#include <stdlib.h>

namespace tcc
{
    #define compare(x) (lexer.type == x)
    #define require(x) {if(!compare(x)) actSyntaxError(x);}
    #define skip(x) {require(charToken(x)); advance();}

    Parser::~Parser() {}

    Parser::Parser()
    {
        lexer.table = table.get();
    }

    void Parser::advance(bool match)
    {
        do
        {
            //advances token, skip comments
            actAdvance();
            lexer.advance(match);
        } while(lexer.type == TokenType::COMMENT);
    }
    
    //main function, initializes vars and first token
    void Parser::parseProgram(const char *source)
    {
        lexer.source = source;
        lexer.lexeme = source;
        lexer.length = 0;
        lexer.lineno = 1;
        lexer.column = 1;

        //first token and keep parsing declarations until EOI
        advance();
        while(!compare(TokenType::EOI)) parseDecl();
    }

    void Parser::parseInt(ExprType type)
    {
        switch(type)
        {
            tag = {};
            case ExprType::TAGGED:
                require(TokenType::UNDEFINED);
                tag = lexer.node;
                advance();
                skip(':');

                skip('[');
                parseExpr();
                skip(',');
                parseExpr();
                
                if(compare(charToken(',')))
                {
                    skip(',');
                    parseExpr();
                    skip(']');
                    actInt(ExprType::GRIDTAGGED);
                    return;
                }
                skip(']');
                break;
            case ExprType::INTERVAL:
                skip('[');
                parseExpr();
                skip(',');
                parseExpr();
                skip(']');
                break;
            case ExprType::GRID:
                skip('[');
                parseExpr();
                skip(',');
                parseExpr();
                skip(',');
                parseExpr();
                skip(']');
                break;
            default: throw std::string("Invalid interval type");
        }
        actInt(type);
    }

    void Parser::parseInts(ExprType type)
    {
        if(type == ExprType::TAGGED)
        {
            size_t size = argList.back().size();
            for(size_t i = 0; i < size; i++)
            {
                parseInt(type);
                //check tags consistency
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
            //parse a list of intervals
            parseInt(type);
            while(compare(charToken(',')))
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

        //parse variable list
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

        //parse function definition
        skip('=');
        parseExpr();

        objName->argIndex = (int)argList.size();
        argList.push_back(args);

        //remove variables from symbol table, they are out-of-scope now
        if(argList.size()) for(Table *t : argList.back()) t->type = TokenType::UNDEFINED;
    }

    void Parser::parseParam()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip(':');
        parseInts(ExprType::INTERVAL);
        objName->type = TokenType::CONSTANT;
    }

    void Parser::parseGrid()
    {
        lexer.type = TokenType::CONSTANT;
        advance();
        skip(':');
        parseInts(ExprType::GRID);
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
        parseInts(ExprType::TAGGED);
        objName->type = TokenType::FUNCTION;
    }

    void Parser::parseSurface()
    {
        parseFDecl();
        require(charToken(','));
        advance(false);
        parseInts(ExprType::TAGGED);
        objName->type = TokenType::FUNCTION;
    }

    void Parser::parseFunction()
    {
        parseFDecl();
        objName->type = TokenType::FUNCTION;
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

        //object name must be UNDEFINED, that is, a new identifier
        require(TokenType::UNDEFINED);
        objName = lexer.node;

        //handle all declarations
        if(objType == param)           parseParam();
        else if(objType == grid)       parseGrid();
        else if(objType == define)     parseDefine();
        else if(objType == curve)      parseCurve();
        else if(objType == surface)    parseSurface();
        else if(objType == function)   parseFunction();
        else if(objType == point)      parsePoint();
        else if(objType == vector)     parseVector();

        //above parse functions must set a new type for the object name
        //so that it can be used from now on

        skip(';');
        actDecl();
    }

    //below are mathematical expression parsing
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
                actOp(ExprType::PLUS);
            }
            else if(compare(charToken('-')))
            {
                advance();
                parseJux();
                actOp(ExprType::MINUS);
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
            actOp(ExprType::JUX);
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
                actOp(ExprType::TIMES);
            }
            else if(compare(charToken('/')))
            {
                advance();
                parseUnary();
                actOp(ExprType::DIVIDE);
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
            actOp(ExprType::UPLUS);
        }
        else if(compare(charToken('-')))
        {
            advance();
            parseUnary();
            actOp(ExprType::UMINUS);
        }
        else if(compare(charToken('*')))
        {
            advance();
            parseUnary();
            actOp(ExprType::UTIMES);
        }
        else if(compare(charToken('/')))
        {
            advance();
            parseUnary();
            actOp(ExprType::UDIVIDE);
        }
        else parseApp();
    }

    void Parser::parseApp()
    {
        if(compare(TokenType::FUNCTION))
        {
            parseFunc();
            parseUnary();
            actOp(ExprType::APP);
        }
        else parsePow();
    }

    void Parser::parseFunc()
    {
        require(TokenType::FUNCTION);
        Table *node = lexer.node;

        actOp(ExprType::FUNCTION);
        advance();

        while(true)
        {
            if(compare(charToken('\'')))
            {
                advance();
                actOp(ExprType::TOTAL);
            }
            else if(compare(charToken('_')))
            {
                advance(false);
                int l = lexer.length;
                bool ok = false;

                //check if variable belongs to function
                if(node->argIndex != -1 && lexer.node) while(true)
                {
                    for(Table *arg : argList[node->argIndex]) if(arg == lexer.node)
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
                actOp(ExprType::PARTIAL);
                advance();
            }
            else break;
        }

        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actOp(ExprType::POW);
        }
    }

    void Parser::parsePow()
    {
        parseComp();
        if(compare(charToken('^')))
        {
            advance();
            parseUnary();
            actOp(ExprType::POW);
        }
    }

    void Parser::parseComp()
    {
        parseFact();
        while(compare(charToken('_')))
        {
            advance();
            require(TokenType::NUMBER);
            actOp(ExprType::COMPONENT);
            advance();
        }
    }

    void Parser::parseFact()
    {
        if(compare(TokenType::CONSTANT))
        {
            actOp(ExprType::CONSTANT);
            advance();
        }
        else if(compare(TokenType::NUMBER))
        {
            actOp(ExprType::NUMBER);
            advance();
        }
        else if(compare(TokenType::VARIABLE))
        {
            actOp(ExprType::VARIABLE);
            advance();
        }
        else parseTuple();
    }

    void Parser::parseTuple()
    {
        char b0 = *lexer.lexeme;
        char b1;

        //allow (), [] or {}
        if(b0 == '(') b1 = ')';
        else if(b0 == '[') b1 = ']';
        else if(b0 == '{') b1 = '}';
        else require(charToken('('));

        skip(b0);
        parseAdd();
        int lTupleSize = 1;
        //handle possible tuple
        while(compare(charToken(',')))
        {
            advance();
            parseAdd();
            lTupleSize++;
        }
        skip(b1);
        tupleSize = lTupleSize;
        actOp(ExprType::TUPLE);
    }

    //semantic actions, default as empty
    void Parser::actAdvance() {}
    void Parser::actDecl() {}
    void Parser::actInt(ExprType) {}
    void Parser::actOp(ExprType) {}

    //Default error callback
    void Parser::actSyntaxError(TokenType type) 
    {
        std::string s1 = getTypeString(type);
        std::string s2 = getTypeString(lexer.type);
        throw std::string("Syntax error: expected " + s1 + " instead of " + s2 +
        ", \nError at Ln " + std::to_string(lexer.lineno) + ", Col " + std::to_string(lexer.column));
    }

    #undef skip
    #undef require
    #undef compare
}