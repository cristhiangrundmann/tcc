#pragma once

#include "lexer.hpp"
#include <vector>

namespace tcc
{

    /*
        Parser
    lexer: a lexer object
    table: a symble table object
    argList: list of (argument list of a function = list of symble table entries for variable names)
    objType: type(keyword for type) of the current object being declared (if any)
    tag: current variable tagged for interval (if any)
    tupleSize: number of tuple elements after parsing it (if any)
    the various INIT's: initializing the symble table with keywords, function names and more
    parseProgram: parse an entire program
        source: pointer to start of input string
    various parseX's: parse an X non-terminal
    various actX: callback for semantic actions of parser
    */
    struct Parser
    {
        /*
            ExprType
        Types for the "symbolic" expressions
        (or intervals)
        */
        enum class ExprType
        {
            TAGGED, GRIDTAGGED, INTERVAL, GRID, //for intervals
            PLUS, MINUS, JUX, TIMES, DIVIDE,
            UPLUS, UMINUS, UTIMES, UDIVIDE,
            APP, FUNCTION, TOTAL, PARTIAL,
            POW, COMPONENT,
            CONSTANT, NUMBER, VARIABLE, TUPLE
        };

        Lexer lexer;
        std::unique_ptr<Table> table = std::make_unique<Table>();
        std::vector<std::vector<Table*>> argList;
        Table *objType{};
        Table *objName{};
        Table *tag{};
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

        virtual ~Parser() = 0;
    };


}