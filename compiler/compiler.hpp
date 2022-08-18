#pragma once

#include "parser.hpp"
#include <sstream>

namespace tcc
{

    struct Expr
    {
        Parser::ExprType type{};
        std::vector<Expr*> sub;
        Table *name{};
        float number{};
        int tupleSize{};
        Expr *compute{};
    };

    struct Interval
    {
        Parser::ExprType type{};
        Table *tag{};
        char wrap{};
        Expr *sub[3]{};
        float number{};
    };

    struct Obj
    {
        Table *type{};
        Table *name{};
        Expr *sub[2]{};
        std::vector<Interval> intervals;
    };

    struct Subst
    {
        Table *var{};
        Expr *exp{};
    };

    struct Compiler : public Parser
    {
        std::vector<std::unique_ptr<Expr>> expressions;
        std::vector<Obj> objects;
        std::vector<Expr*> expStack;
        std::vector<Interval> intStack;

        void actInt(ExprType type);
        void actOp(ExprType type);
        void actDecl();
        Expr *newExpr(Expr &e);
        Expr *op(ExprType type, Expr *a = nullptr, Expr *b = nullptr, Table *name = nullptr, float number = 0);
        Expr *compute(Expr *e);
        Expr *derivative(Expr *e, Table *var);
        Expr *substitute(Expr *e, std::vector<Subst> &substs);
        void compile(Expr *e, std::stringstream &str, int &v);
        void compile(std::stringstream &str, bool declareOnly = false);
        void compileFunction(Expr *exp, int argIndex, std::stringstream &str, std::string suffix, bool declareOnly);
        void declareFunction(int N, int argIndex, std::stringstream &str, std::string suffix);
        float calc(Expr *e);
    };

};