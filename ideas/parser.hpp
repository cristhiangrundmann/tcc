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
        Table *objType = nullptr;
        Table *objName = nullptr;
        bool multUnary;

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
        void require(TokenType type);
        bool compare(TokenType type);
        void advance(Table::Mode mode = Table::Mode::MATCH);
        void removeArgs();

        typedef void Parse();

        void parseProgram(const char *source);

        Parse 
            parseInt, parseInts, parseIGrid, parseIGrids, parseTInt,
            parseTInts, parseFDecl, parseParam, parseGrid, parseDefine, parseCurve,
            parseSurface, parseFunction, parsePoint, parseVector, parseDecl,
            parseExpr, parseAdd, parseJux, parseMult, parseUnary, parseApp, parseFunc,
            parsePow, parseComp, parseFact, parseTuple;

        virtual void syntaxError(TokenType type);
        virtual void actAdvance();
        virtual void actInt(char c);
        virtual void actDecl();
        virtual void actBinary(char c);
        virtual void actUnary(char c);
    };


}