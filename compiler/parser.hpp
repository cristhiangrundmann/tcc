#pragma once

#include "lexer.hpp"
#include <vector>

namespace tcc
{
    struct Parser
    {
        Lexer lexer;
        std::unique_ptr<Table> table = std::make_unique<Table>();
        std::vector<std::vector<Table*>> argList;
        Table *objType{};
        Table *objName{};
        Table *tag{};
        char wrap = 0;

        #define INIT(x, y) const Table *x = table->initString(#x, TokenType::y);
            INIT(param,     DECLARE)
            INIT(grid,      DECLARE)
            INIT(define,    DECLARE)
            INIT(curve,     DECLARE)
            INIT(surface,   DECLARE)
            INIT(function,  DECLARE)
            INIT(point,     DECLARE)
            INIT(vector,    DECLARE)
            INIT(e,         CONSTANT)
            INIT(pi,        CONSTANT)
            INIT(sin,       FUNCTION)
            INIT(cos,       FUNCTION)
            INIT(tan,       FUNCTION)
            INIT(exp,       FUNCTION)
            INIT(log,       FUNCTION)
            INIT(sqrt,      FUNCTION)
        #undef INIT

        Parser();
        void advance(bool match = true);
        void removeArgs();

        typedef void Parse();

        void parseProgram(const char *source);

        Parse 
            parseFDecl, parseParam, parseGrid, parseDefine, parseCurve,
            parseSurface, parseFunction, parsePoint, parseVector, parseDecl,
            parseExpr, parseAdd, parseJux, parseUnary, parseApp, parseFunc,
            parsePow, parseComp, parseFact, parseTuple;

        void parseInt(char type);
        void parseInts(char type);

        void parseMult(bool unary);

        virtual void syntaxError(TokenType type);
        virtual void actAdvance();
        virtual void actInt(char type);
        virtual void actDecl();
        virtual void actBinary(char type);
        virtual void actUnary(char type);
    };


}