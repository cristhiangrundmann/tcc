#pragma once

#include "parser.hpp"
#include <stack>

namespace tcc
{

    struct Expr
    {
        ExprType type;
        Expr *sub[2]{};
        Table *name{};
        double number{};
    };

    struct Interval
    {
        ExprType type;
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

    struct Compiler : public Parser
    {
        std::vector<std::unique_ptr<Expr>> expressions;
        std::vector<Obj> objects;
        std::stack<Expr*> expStack;
        std::stack<Interval> intStack;

        Expr *newExpr(Expr &e);
        void actInt(ExprType type);
        void actOp(ExprType type);
        void actDecl();
        Expr *derivative(Expr *e, Table *var);
    };

};