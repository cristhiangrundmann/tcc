#pragma once

#include "lexer.hpp"
#include <vector>

namespace tcc
{
    enum class ExprType
    {
        TAGGED, INTERVAL, GRID, //for intervals
        PLUS, MINUS, JUX, TIMES, DIVIDE,
        UPLUS, UMINUS, UTIMES, UDIVIDE,
        APP, FUNCTION, TOTAL, PARTIAL,
        EXP, COMPONENT,
        CONSTANT, NUMBER, VARIABLE, TUPLE
    };

    struct Parser
    {
        Lexer lexer;
        std::unique_ptr<Table> table = std::make_unique<Table>();
        std::vector<std::vector<Table*>> argList;
        Table *objType{};
        Table *objName{};
        Table *tag{};
        char wrap = 0;
        int tupleSize = 0;

        static constexpr float CE = 2.718281;
        static constexpr float CPI = 3.141592;

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
            INIT(id,        FUNCTION)
        #undef INIT

        Parser();
        void advance(bool match = true);

        typedef void Parse();

        void parseProgram(const char *source);

        Parse 
            parseFDecl, parseParam, parseGrid, parseDefine, parseCurve,
            parseSurface, parseFunction, parsePoint, parseVector, parseDecl,
            parseExpr, parseAdd, parseJux, parseUnary, parseApp, parseFunc,
            parsePow, parseComp, parseFact, parseTuple;

        void parseInt(ExprType type);
        void parseInts(ExprType type);

        void parseMult(bool unary);

        virtual void actSyntaxError(TokenType type);
        virtual void actAdvance();
        virtual void actInt(ExprType type);
        virtual void actOp(ExprType type);
        virtual void actDecl();
    };


}