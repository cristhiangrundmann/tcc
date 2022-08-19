#pragma once

#include "parser.hpp"
#include <sstream>

namespace tcc
{

    struct SymbExpr
    {
        Parser::ExprType type{};
        std::vector<SymbExpr*> sub{};
        Table *name{};
        float number{};
    };

    struct CompExpr
    {
        enum class ExprType
        {
            PLUS, MINUS, TIMES, DIVIDE,
            UPLUS, UMINUS, UDIVIDE,
            APP, FUNCTION,
            POW, COMPONENT,
            CONSTANT, NUMBER, VARIABLE, TUPLE
        };
        ExprType type{};
        std::vector<CompExpr*> sub{};
        Table *name{};
        float number{};
        int nTuple{};
    };

    struct Interval
    {
        Parser::ExprType type{};
        Table *tag{};
        char wrap{};
        SymbExpr *sub[3]{};
        CompExpr *compSub[3]{};
        float number{};
        float min{}, max{};
    };

    struct Obj
    {
        Table *type{};
        Table *name{};
        SymbExpr *sub[2]{};
        CompExpr *compSub[2]{};
        std::vector<Interval> intervals;
        int nTuple{};
    };

    struct Subst
    {
        Table *var{};
        CompExpr *exp{};
        float number{};
    };

    struct Compiler : public Parser
    {
        std::vector<std::unique_ptr<SymbExpr>> symbExprs;
        std::vector<std::unique_ptr<CompExpr>> compExprs;
        std::vector<SymbExpr*> expStack;
        std::vector<Interval> intStack;
        std::vector<Obj> objects;

        void actInt(ExprType type);
        void actOp(ExprType type);
        void actDecl();
        SymbExpr *newExpr(SymbExpr &e);
        CompExpr *newExpr(CompExpr &e);
        SymbExpr op(Parser::ExprType type, SymbExpr *a = nullptr, SymbExpr *b = nullptr, float number = 0, Table *name = nullptr);
        CompExpr *op(CompExpr::ExprType type, CompExpr *a = nullptr, CompExpr *b = nullptr, float number = 0, Table *name = nullptr, int nTuple = 1);
        CompExpr *_comp(CompExpr *e, unsigned int index, std::vector<Subst> &subs);
        CompExpr *compute(SymbExpr *e, std::vector<Subst> &subs);
        CompExpr *substitute(CompExpr *e, std::vector<Subst> &subs);
        CompExpr *derivative(CompExpr *e, Table *var);
        void compile(CompExpr *e, std::stringstream &str, int &v);
        void compile(std::stringstream &str);
        void header(std::stringstream &str);
        void compileFunction(CompExpr *exp, int argIndex, std::stringstream &str, std::string name);
        void declareFunction(int N, int argIndex, std::stringstream &str, std::string name, bool declareOnly = false);
        float calculate(CompExpr *e, std::vector<Subst> &subs);
    };

};