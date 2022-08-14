#include "compiler.hpp"

using std::make_unique;
using std::unique_ptr;

#define E(x) ExprType::x

namespace tcc
{

    void Compiler::actInt(ExprType type)
    {
        Expr *e[3]{};
        e[0] = compute(expStack.back(), -1); expStack.pop_back();
        e[1] = compute(expStack.back(), -1); expStack.pop_back();
        if(type == E(GRID))
        {
            e[2] = compute(expStack.back(), -1);
            expStack.pop_back();
        }
        Interval i = {type, tag, wrap, {e[0], e[1], e[2]}};
        intStack.push_back(i);
    }

    void Compiler::actOp(ExprType type)
    {
        Expr exp;
        exp.type = type;
        switch(type)
        {
            case E(PARTIAL):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                exp.name = lexer.node;
                break;
            case E(COMPONENT):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                exp.number = lexer.number;
                if((int)exp.number != exp.number || (int)exp.number < 1)
                    throw std::string("Component must be a positive integer");
                break;
            case E(PLUS):
            case E(MINUS):
            case E(TIMES):
            case E(DIVIDE):
            case E(JUX):
            case E(APP):
            case E(EXP):
            {
                Expr *e0 = expStack.back();
                expStack.pop_back();
                Expr *e1 = expStack.back();
                expStack.pop_back();
                exp.sub.push_back(e1);
                exp.sub.push_back(e0);
                break;
            }
            case E(TUPLE):
            {
                if(tupleSize < 2) return;
                for(int i = 0; i < tupleSize; i++)
                    exp.sub.push_back(expStack[expStack.size()-tupleSize+i]);
                for(int i = 0; i < tupleSize; i++)
                    expStack.pop_back();
                break;
            }
            case E(UPLUS):
            case E(UMINUS):
            case E(UTIMES):
            case E(UDIVIDE):
            case E(TOTAL):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                break;
            case E(NUMBER):
                exp.number = lexer.number;
                break;
            case E(CONSTANT):
            case E(VARIABLE):
            case E(FUNCTION):
                exp.name = lexer.node;
                break;
            default:
                throw std::string("Invalid expression type");
        }
        expStack.push_back(newExpr(exp));
    }

    void Compiler::actDecl()
    {
        Obj obj;
        obj.type = objType;
        obj.name = objName;
        obj.name->objIndex = (int)objects.size();

        if(objType == param || objType == grid)
        {
            while(!intStack.empty())
            {
                obj.intervals.push_back(intStack.back());
                intStack.pop_back();
            }
        }
        else if(objType == define)
        {
            obj.sub[0] = compute(expStack.back(), -1);
            expStack.pop_back();
        }
        else if(objType == curve || objType == surface || objType == function)
        {
            obj.sub[0] = compute(expStack.back(), obj.name->argsIndex);
            expStack.pop_back();
            if(objType != function) while(!intStack.empty())
            {
                obj.intervals.push_back(intStack.back());
                intStack.pop_back();
            }
        }
        else if(objType == point || objType == vector)
        {
            obj.sub[0] = compute(expStack.back(), -1);
            expStack.pop_back();
            if(objType == vector)
            {
                obj.sub[1] = compute(expStack.back(), -1);
                expStack.pop_back();
            }
        }
        else throw std::string("Invalid declaration type");
        objects.push_back(obj);
    }

    Expr *Compiler::newExpr(Expr &e)
    {
        printf("%ld expressions\n", expressions.size());
        fflush(stdout);
        expressions.push_back(std::make_unique<Expr>(e));
        return expressions.back().get();
    }

    Expr *Compiler::op(ExprType type, Expr *a, Expr *b, Table *name, double number)
    {
        Expr e;
        e.type = type;
        if(a) e.sub.push_back(a);
        if(a && b) e.sub.push_back(b);
        e.name = name;
        e.number = number;
        return newExpr(e);
    }

    #define COMP(x, y) compute(op(E(COMPONENT), x, nullptr, nullptr, y), argsIndex)

    Expr *Compiler::compute(Expr *e, int argsIndex)
    {
        switch(e->type)
        {
            case E(NUMBER):
                e->tupleSize = 1;
                return e;
            case E(CONSTANT):
            {
                if(e->name->objIndex == -1)
                {
                    e->tupleSize = 1;
                    return e;
                }
                Obj obj = objects[e->name->objIndex];
                if(obj.type == define) return compute(obj.sub[0], argsIndex);
                if(obj.type == param || obj.type == grid)
                {
                    e->tupleSize = obj.intervals.size();
                    return e;
                }
                if(obj.type == point || obj.type == vector)
                    return compute(obj.sub[0], argsIndex);
            }
            case E(VARIABLE):
            {
                e->tupleSize = 0;
                if(argsIndex != -1)
                {
                    for(int i = 0; i < (int)argList[argsIndex].size(); i++)
                        if(argList[argsIndex][i] == e->name)
                        {
                            e->tupleSize = 1;
                            break;
                        }
                    if(!e->tupleSize) throw std::string("Variable not found");
                }
                return e;
            }
            case E(FUNCTION):
            {
                if(e->name->objIndex == -1)
                {
                    e->tupleSize = 1;
                    return e;
                }
                return compute(objects[e->name->objIndex].sub[0], -1);
            }
            case E(COMPONENT):
            {
                Expr *a = compute(e->sub[0], argsIndex);
                if(a->tupleSize)
                {
                    if(e->number > a->tupleSize)
                        throw std::string("Invalid tuple index");
                    if(a->type == E(TUPLE))
                        return compute(a->sub[e->number-1], argsIndex);
                }
                return op(E(COMPONENT), a, nullptr, nullptr, e->number);
            }
            case E(PLUS):
            case E(MINUS):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *e1 = compute(e->sub[1], argsIndex);
                if(!e0->tupleSize || !e1->tupleSize)
                    return op(e->type, e0, e1);
                if(e0->tupleSize != e1->tupleSize)
                    throw std::string("Tuple size mismatch");
                if(e0->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return r;
                }
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < t->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1), COMP(e1, i+1)), argsIndex));
                return t;
            }
            case E(TIMES):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *e1 = compute(e->sub[1], argsIndex);
                if(!e0->tupleSize || !e1->tupleSize)
                    return op(e->type, e0, e1);
                if(e0->tupleSize == 1 && e1->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return r;
                }
                if(e0->tupleSize == 1 || e1->tupleSize == 1)
                {
                    Expr *a, *b;
                    if(e0->tupleSize == 1)
                    {
                        a = e0;
                        b = e1;
                    }
                    else
                    {
                        a = e1;
                        b = e0;
                    }
                    Expr *t = op(E(TUPLE));
                    t->tupleSize = b->tupleSize;
                    for(int i = 0; i < t->tupleSize; i++)
                        t->sub.push_back(compute(op(e->type, a, COMP(b, i+1)), argsIndex));
                    return t;
                }
                if(e0->tupleSize != 3 || e1->tupleSize != 3)
                    throw std::string("Invalid cross-product");
                Expr *a1 = COMP(e0, 1);
                Expr *a2 = COMP(e0, 2);
                Expr *a3 = COMP(e0, 3);
                Expr *b1 = COMP(e1, 1);
                Expr *b2 = COMP(e1, 2);
                Expr *b3 = COMP(e1, 3);
                Expr *z1 = op(E(MINUS), op(E(TIMES), a2, b3), op(E(TIMES), a3, b2));
                Expr *z2 = op(E(MINUS), op(E(TIMES), a3, b1), op(E(TIMES), a1, b3));
                Expr *z3 = op(E(MINUS), op(E(TIMES), a1, b2), op(E(TIMES), a2, b1));
                Expr *t = op(E(TUPLE));
                t->sub.push_back(z1);
                t->sub.push_back(z2);
                t->sub.push_back(z3);
                t = compute(t, argsIndex);
                return t;
            }
            case E(DIVIDE):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *e1 = compute(e->sub[1], argsIndex);
                if(e1->tupleSize > 1)
                    throw std::string("Cannot divide tuples");
                if(!e0->tupleSize || !e1->tupleSize)
                    return op(e->type, e0, e1);
                if(e0->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = e0->tupleSize;
                    return r;
                }
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < e0->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1), e1), argsIndex));
                return t;
            }
            case E(JUX):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *e1 = compute(e->sub[1], argsIndex);
                if(!e0->tupleSize || !e1->tupleSize)
                    return op(e->type, e0, e1);
                if(e0->tupleSize == 1 && e1->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return r;
                }
                if(e0->tupleSize == 1 || e1->tupleSize == 1)
                {
                    Expr *a, *b;
                    if(e0->tupleSize == 1)
                    {
                        a = e0;
                        b = e1;
                    }
                    else
                    {
                        a = e1;
                        b = e0;
                    }
                    Expr *t = op(E(TUPLE));
                    t->tupleSize = b->tupleSize;
                    for(int i = 0; i < t->tupleSize; i++)
                        t->sub.push_back(compute(op(e->type, a, COMP(b, i+1)), argsIndex));
                    return t;
                }
                if(e0->tupleSize != e1->tupleSize)
                    throw std::string("Inconsistent tuple size");

                std::vector<Expr*> terms;
                for(int i = 0; i < e0->tupleSize; i++)
                {
                    Expr *a = op(e->type, 
                    op(E(COMPONENT), e0, nullptr, nullptr, i+1),
                    op(E(COMPONENT), e1, nullptr, nullptr, i+1));
                    terms.push_back(a);
                }
                Expr *sum = terms[0];
                for(int i = 1; i < e0->tupleSize; i++)
                {
                    Expr *nSum = op(e->type, sum, terms[1]);
                    sum = nSum;
                }
                return compute(sum, argsIndex);
            }
            case E(UPLUS):
            case E(UMINUS):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                if(e0->tupleSize < 2) return op(e->type, e0);
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < e0->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1)), argsIndex));
                return t;
            }
            case E(UTIMES):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                if(!e0->tupleSize) return op(e->type, e0);
                return compute(op(E(JUX), e0, e0), argsIndex);
            }
            case E(UDIVIDE):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                if(e0->tupleSize > 1)
                    throw std::string("Cannot invert tuples");
                Expr *r = op(e->type, e0);
                r->tupleSize = e0->tupleSize;
                return r;
            }
            case E(APP):
            {
                Expr *f = e->sub[0];
                while(f->type != E(FUNCTION)) f = f->sub[0];
                int argCount = 1;
                if(f->name->argsIndex != -1)
                    argCount = argList[f->name->argsIndex].size();
                
                std::vector<Subst> substs;
                if(argCount == 1)
                {
                    Table *name = nullptr;
                    if(f->name->argsIndex != -1)
                        name = argList[f->name->argsIndex][0];
                    substs.push_back({name, compute(e->sub[1], argsIndex)});
                }
                else
                {
                    if(e->sub[1]->type != E(TUPLE))
                        throw std::string("Expected a tuple of arguments");
                    if(e->sub[1]->sub.size() != (unsigned int)argCount)
                        throw std::string("Wrong number of arguments");
                    for(int i = 0; i < argCount; i++)
                        substs.push_back({
                            argList[f->name->argsIndex][i],
                            compute(e->sub[1]->sub[i], argsIndex)});
                }
                Expr *e0 = compute(e->sub[0], -1);
                Expr *r = substitute(e0, substs);
                if(f->name->argsIndex != -1)
                    r = compute(r, argsIndex);
                else
                    r->tupleSize = 1;
                return r;
            }
            case E(TOTAL):
            case E(PARTIAL):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *r = compute(derivative(e0, e->name), argsIndex);
                r->tupleSize = e0->tupleSize;
                return r;
            }
            case E(EXP):
            {
                Expr *e0 = compute(e->sub[0], argsIndex);
                Expr *e1 = compute(e->sub[1], argsIndex);
                if(e0->tupleSize > 1 || e1->tupleSize > 1)
                    throw std::string("Cannot exponentiate tuples");
                Expr *r = op(E(EXP), e0, e1);
                r->tupleSize = e0->tupleSize;
                return r;
            }
            case E(TUPLE):
            {
                Expr *t = op(E(TUPLE));
                t->tupleSize = e->sub.size();
                for(int i = 0; i < t->tupleSize; i++)
                    t->sub.push_back(compute(e->sub[i], argsIndex));
                return t;
            }
            default:
                throw std::string("Invalid expression type");
        }
    }

    #undef COMP

    void Expr::print()
    {
    }

    Expr *Compiler::derivative(Expr *e, Table *var)
    {
        if(!e->tupleSize)
            throw std::string("Cannot derive unknown type");

        switch(e->type)
        {
            case E(NUMBER):
            case E(COMPONENT):
                return op(E(NUMBER), nullptr, nullptr, nullptr, 0);
            case E(CONSTANT):
            {
                if(e->tupleSize == 1)
                    return op(E(NUMBER), nullptr, nullptr, nullptr, 0);
                Expr *t = op(E(TUPLE));
                for(int i = 0; i < e->tupleSize; i++)
                    t->sub.push_back(op(E(NUMBER), nullptr, nullptr, nullptr, 0));
                return t;
            }
            case E(VARIABLE):
                return op(E(NUMBER), nullptr, nullptr, nullptr, var == e->name ? 1 : 0);
            case E(FUNCTION):
            {
                
            }
            case E(PLUS):
            case E(MINUS):
            {

            }
            case E(TIMES):
            case E(JUX):
            {

            }
            case E(DIVIDE):
            {

            }
            case E(UPLUS):
            case E(UMINUS):
                return op(e->type, derivative(e->sub[0], var));
            case E(UTIMES):
                throw std::string("Invalid expression type");
            case E(UDIVIDE):
            {
                Expr *a = op(E(EXP), e->sub[0], op(E(NUMBER), nullptr, nullptr, nullptr, -2));
                return op(E(UMINUS), op(E(UDIVIDE), a));
            }
            case E(APP):
            {
                Expr *d = derivative(e->sub[0], nullptr);
                Expr *d2 = derivative(e->sub[1], var);
                Expr *a = op(E(TIMES), op(E(APP), d, e->sub[1]), d2);
                return a;
            }
            case E(TOTAL):
            case E(PARTIAL):
                throw std::string("Derivatives should be gone");
            case E(EXP):
            {
                if(e->sub[1]->type == E(NUMBER))
                {
                    ///
                    return nullptr;
                }
                ///g'(t) = g(t) [h'(t)lnf(t) + h(t)f'(t)/f(t)]
                return nullptr;
            }
            case E(TUPLE):
            { 
                Expr *t = op(E(TUPLE));
                for(Expr *exp : e->sub)
                    t->sub.push_back(derivative(exp, var));
                return t;
            }
            case E(TAGGED):
            case E(GRID):
            case E(INTERVAL):
                throw std::string("Invalid expression type");
        }
        return nullptr;
    }

    Expr *Compiler::substitute(Expr *e, std::vector<Subst> &substs)
    {
        switch(e->type)
        {
            case E(NUMBER):
            case E(CONSTANT):
                return e;
            case E(FUNCTION):
                if(substs.size() != 1)
                    throw std::string("Must have 1 argument");
                if(substs[0].exp->tupleSize > 1)
                    throw std::string("Must have 1 argument");
                return op(E(APP), e, substs[0].exp);
            case E(VARIABLE):
                for(Subst s : substs)
                    if(s.var == e->name) return s.exp;
                throw std::string("Variable definition missing");
            case E(COMPONENT):
                return op(e->type, substitute(e->sub[0], substs),
                nullptr, nullptr, e->number);
            case E(PLUS):
            case E(MINUS):
            case E(TIMES):
            case E(DIVIDE):
            case E(JUX):
            case E(EXP):
                return op(e->type,
                    substitute(e->sub[0], substs),
                    substitute(e->sub[1], substs));
            case E(APP):
                if(e->sub[0]->type != E(FUNCTION))
                    throw std::string("Must be a FUNCTION");
                return op(e->type, e->sub[0], substitute(e->sub[1], substs));
            case E(UPLUS):
            case E(UMINUS):
            case E(UTIMES):
            case E(UDIVIDE):
                return op(e->type, substitute(e->sub[0], substs));
            case E(TOTAL):
            case E(PARTIAL):
                throw std::string("Derivatives should be gone");
            case E(TUPLE):
            {
                Expr *t = op(E(TUPLE));
                for(int i = 0; i < (int)e->sub.size(); i++)
                    t->sub.push_back(substitute(e->sub[i], substs));
                return t;
            }
            default:
                throw std::string("Invalid expression type");
        }
    }

};

#undef E