#pragma once

#include "lexer.hpp"
#include <vector>

namespace tcc
{
    struct Parser
    {
        Lexer lexer;
        std::unique_ptr<Table> table = std::make_unique<Table>();;
        std::vector<std::vector<Table*>> argList;

        #define INIT(x, y) Table *x = table->initString(#x, TokenType::y);
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

        Parser(const char *source);
        void require(TokenType type);
        bool compare(TokenType type);
        void advance(Table::Mode mode = Table::Mode::MATCH);
        void removeArgs();
        virtual void comment(int start, int end);
        virtual void syntaxError(TokenType type);

        typedef void Parse();
        typedef void Action();

        Parse 
            parseProgram, parseInt, parseInts, parseIGrid, parseIGrids, parseTInt,
            parseTInts, parseFDecl, parseParam, parseGrid, parseDefine, parseCurve,
            parseSurface, parseFunction, parsePoint, parseVector, parseDecl,
            parseExpr, parseAdd, parseJux, parseUnary, parseApp, parseFunc,
            parsePow, parseComp, parseFact, parseTuple;

        void parseMult(bool unary);

        virtual Action
            actAdvance,
            actInt, actIGrid, actTInt,
            actFDecl, actParam, actGrid, actDefine, actCurve, actSurface,
            actFunction, actPoint, actVector, actPlus, actMinus, actJux, 
            actTimes, actDivide, actUPlus, actUMinus, actUTimes, actUDivide,
            actFunc, actTotal, actPartial, actFPow, actPow, actComp, actConst,
            actNumber, actVar, actTuple;
    };


}