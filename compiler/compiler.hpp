#pragma once

#include "parser.hpp"

namespace tcc
{

    struct Expr
    {
        ExprType type{};
        std::vector<Expr*> sub;
        Table *name{};
        double number{};
        int tupleSize{};
        Expr *compute{};

        void print();
    };

    struct Interval
    {
        ExprType type{};
        Table *tag{};
        char wrap{};
        Expr *sub[3]{};
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
        Expr *op(ExprType type, Expr *a = nullptr, Expr *b = nullptr, Table *name = nullptr, double number = 0);
        Expr *compute(Expr *e);
        Expr *derivative(Expr *e, Table *var);
        Expr *substitute(Expr *e, std::vector<Subst> &substs);
    };

};