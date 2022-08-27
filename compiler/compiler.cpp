#include "compiler.hpp"
#include <cmath>
#include <GL/glew.h>

#define S(x) Parser::ExprType::x
#define C(x) CompExpr::ExprType::x

namespace tcc
{

    void Compiler::actInt(ExprType type)
    {
        SymbExpr *e[3]{};
        e[0] = expStack.back(); expStack.pop_back();
        e[1] = expStack.back(); expStack.pop_back();
        Interval i;
        if(type == S(GRID))
        {
            e[2] = expStack.back();
            expStack.pop_back();
            i = {type, tag, wrap, {e[2], e[1], e[0]}};
        }
        else
            i = {type, tag, wrap, {e[1], e[0], nullptr}};
        
        intStack.push_back(i);
    }

    void Compiler::actOp(ExprType type)
    {
        SymbExpr exp;
        exp.type = type;
        switch(type)
        {
            case S(PARTIAL):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                exp.name = lexer.node;
                break;
            case S(COMPONENT):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                exp.number = lexer.number;
                if((int)exp.number != exp.number || (int)exp.number < 1)
                    throw std::string("Component must be a positive integer");
                break;
            case S(PLUS):
            case S(MINUS):
            case S(TIMES):
            case S(DIVIDE):
            case S(JUX):
            case S(APP):
            case S(POW):
            {
                SymbExpr *e0 = expStack.back();
                expStack.pop_back();
                SymbExpr *e1 = expStack.back();
                expStack.pop_back();
                exp.sub.push_back(e1);
                exp.sub.push_back(e0);
                break;
            }
            case S(TUPLE):
            {
                if(tupleSize < 2) return;
                for(int i = 0; i < tupleSize; i++)
                    exp.sub.push_back(expStack[expStack.size()-tupleSize+i]);
                for(int i = 0; i < tupleSize; i++)
                    expStack.pop_back();
                break;
            }
            case S(UPLUS):
            case S(UMINUS):
            case S(UTIMES):
            case S(UDIVIDE):
            case S(TOTAL):
                exp.sub.push_back(expStack.back());
                expStack.pop_back();
                break;
            case S(NUMBER):
                exp.number = lexer.number;
                break;
            case S(CONSTANT):
            case S(VARIABLE):
            case S(FUNCTION):
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
            int size = intStack.size();
            for(int i = 0; i < size; i++)
                obj.intervals.push_back(intStack[i]);
            for(int i = 0; i < size; i++)
                intStack.pop_back();
        }
        else if(objType == define)
        {
            obj.sub[0] = expStack.back();
            expStack.pop_back();
        }
        else if(objType == curve || objType == surface || objType == function)
        {
            obj.sub[0] = expStack.back();
            expStack.pop_back();
            if(objType != function) 
            {
                int size = intStack.size();
                for(int i = 0; i < size; i++)
                    obj.intervals.push_back(intStack[i]);
                for(int i = 0; i < size; i++)
                    intStack.pop_back();
            }
        }
        else if(objType == point)
        {
            obj.sub[0] = expStack.back();
            expStack.pop_back();
        }
        else if(objType == vector)
        {
            obj.sub[1] = expStack.back();
            expStack.pop_back();
            obj.sub[0] = expStack.back();
            expStack.pop_back();
        }
        else throw std::string("Invalid declaration type");
        objects.push_back(obj);
    }

    SymbExpr *Compiler::newExpr(SymbExpr &e)
    {
        symbExprs.push_back(std::make_unique<SymbExpr>(e));
        return symbExprs.back().get();
    }

    CompExpr *Compiler::newExpr(CompExpr &e)
    {
        compExprs.push_back(std::make_unique<CompExpr>(e));
        return compExprs.back().get();
    }

    CompExpr *Compiler::op(CompExpr::ExprType type, CompExpr *a, CompExpr *b, float number, Table *name, int nTuple)
    {
        switch(type)
        {
            case C(PLUS):
            case C(MINUS):
            {
                if(a->type == C(NUMBER) && b->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, a->number + (type == C(PLUS) ? 1 : -1)*b->number);
                if(a->type == C(NUMBER) && a->number == 0)
                    return op(type == C(PLUS) ? C(UPLUS) : C(UMINUS), b);
                if(b->type == C(NUMBER) && b->number == 0)
                    return a;
                break;
            }
            case C(TIMES):
            {
                if(a->type == C(NUMBER) && b->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, a->number * b->number);
                if(a->type == C(NUMBER) && a->number == 0)
                    return op(C(NUMBER), nullptr, nullptr, 0);
                if(b->type == C(NUMBER) && b->number == 0)
                    return op(C(NUMBER), nullptr, nullptr, 0);
                if(a->type == C(NUMBER) && a->number == 1)
                    return b;
                if(b->type == C(NUMBER) && b->number == 1)
                    return a;
                break;
            }
            case C(DIVIDE):
            {
                if(a->type == C(NUMBER) && b->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, a->number / b->number);
                if(a->type == C(NUMBER) && a->number == 0)
                    return op(C(NUMBER), nullptr, nullptr, 0);
                if(b->type == C(NUMBER) && b->number == 1)
                    return a;
                break;
            }
            case C(UPLUS): return a;
            case C(UMINUS):
            {
                if(a->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, -a->number);
                if(a->type == C(UMINUS))
                    return a->sub[0];
                break;
            }
            case C(UDIVIDE):
            {
                if(a->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, 1.0 / a->number);
                break;
            }
            case C(APP):
            {
                if(a->type != C(FUNCTION))
                    throw std::string("Expected a function");
                if(b->type == C(NUMBER))
                {
                    if(a->name == sin) return op(C(NUMBER), nullptr, nullptr, std::sin(b->number));
                    if(a->name == cos) return op(C(NUMBER), nullptr, nullptr, std::cos(b->number));
                    if(a->name == tan) return op(C(NUMBER), nullptr, nullptr, std::tan(b->number));
                    if(a->name == exp) return op(C(NUMBER), nullptr, nullptr, std::exp(b->number));
                    if(a->name == log) return op(C(NUMBER), nullptr, nullptr, std::log(b->number));
                    if(a->name == sqrt) return op(C(NUMBER), nullptr, nullptr, std::sqrt(b->number));
                    if(a->name == id) return op(C(NUMBER), nullptr, nullptr, b->number);
                    throw std::string("Unknown function");
                }
                break;
            }
            case C(POW):
            {
                if(a->type == C(NUMBER) && b->type == C(NUMBER))
                    return op(C(NUMBER), nullptr, nullptr, std::pow(a->number, b->number));
                if(a->type == C(NUMBER) && a->number == 0)
                    return op(C(NUMBER), nullptr, nullptr, 0);
                if(b->type == C(NUMBER) && b->number == 1)
                    return a;
                break;
            }
            case C(CONSTANT):
            {
                if(name == e) return op(C(NUMBER), nullptr, nullptr, Parser::CE);
                if(name == pi) return op(C(NUMBER), nullptr, nullptr, Parser::CPI);
                break;
            }
            default: break;
        }
        CompExpr e{};
        e.type = type;
        if(a) e.sub.push_back(a);
        if(a && b) e.sub.push_back(b);
        e.name = name;
        e.number = number;
        e.nTuple = nTuple;
        return newExpr(e);
    }


    SymbExpr Compiler::op(Parser::ExprType type, SymbExpr *a, SymbExpr *b, float number, Table *name)
    {
        SymbExpr e{};
        e.type = type;
        if(a) e.sub.push_back(a);
        if(a && b) e.sub.push_back(b);
        e.name = name;
        e.number = number;
        return e;
    }

    CompExpr *Compiler::_comp(CompExpr *e, unsigned int index, std::vector<Subst> &subs)
    {
        if(e->type == C(TUPLE))
        {
            if(e->sub.size() < index) throw std::string("Invalid index");
            return e->sub[index-1];
        }
        else
        {
            if(e->type == C(VARIABLE))
            {
                for(Subst &s : subs)
                    if(s.var == e->name)
                    {
                        std::vector<Subst> subs2;
                        CompExpr *c = _comp(s.exp, index, subs2);
                        return op(C(COMPONENT), e, nullptr, index, nullptr, c->nTuple);
                    }
                return op(C(COMPONENT), e, nullptr, index);
            }
            else if(e->type == C(CONSTANT))
            {
                if(e->name->objIndex == -1) throw std::string("Invalid index");
                Obj &o = objects[e->name->objIndex];
                if(o.type == param || o.type == grid)
                {
                    if(o.intervals.size() < index) throw std::string("Invalid index");
                    return op(C(COMPONENT), e, nullptr, index);
                }
                else throw std::string("Invalid type");
            }
            else throw std::string("Invalid type");
        }
    }

    CompExpr *Compiler::compute(SymbExpr *e, std::vector<Subst> &subs)
    {
        switch(e->type)
        {
            case S(CONSTANT):
            {
                if(e->name->objIndex == -1) return op(C(CONSTANT), nullptr, nullptr, 0, e->name);
                Obj &o = objects[e->name->objIndex];
                if(o.type == define) return compute(o.sub[0], subs);
                if(o.type == param || o.type == grid)
                    return op(C(CONSTANT), nullptr, nullptr, 0, e->name, o.intervals.size());
                if(o.type == point || o.type == vector)
                    return compute(o.sub[0], subs);
                throw std::string("Invalid type");
            }
            case S(NUMBER): return op(C(NUMBER), nullptr, nullptr, e->number);
            case S(VARIABLE):
            {
                for(Subst &s : subs)
                    if(s.var == e->name)
                        return op(C(VARIABLE), nullptr, nullptr, 0, e->name, s.exp->nTuple);
                return op(C(VARIABLE), nullptr, nullptr, 0, e->name);
            }
            case S(FUNCTION):
            {
                if(e->name->objIndex == -1) return op(C(FUNCTION), nullptr, nullptr, 0, e->name);
                return compute(objects[e->name->objIndex].sub[0], subs);
            }
            case S(APP):
            {
                SymbExpr *f = e->sub[0];
                while(f->type != S(FUNCTION)) f = f->sub[0];
                unsigned int argCount = 1;
                if(f->name->argIndex != -1)
                    argCount = argList[f->name->argIndex].size();     
                std::vector<Subst> subs2;
                if(argCount == 1)
                {
                    Table *name = nullptr;
                    if(f->name->argIndex != -1)
                        name = argList[f->name->argIndex][0];
                    CompExpr *c = compute(e->sub[1], subs);
                    subs2.push_back({name, c});
                }
                else
                {
                    if(e->sub[1]->type != S(TUPLE)) throw std::string("Expected a tuple");
                    if(e->sub[1]->sub.size() != argCount) throw std::string("Wrong number of arguments");
                    for(unsigned int i = 0; i < argCount; i++)
                    {
                        CompExpr *c = compute(e->sub[1]->sub[i], subs);
                        Table *name = argList[f->name->argIndex][i];
                        subs2.push_back({name, c});
                    }
                }
                return substitute(compute(e->sub[0], subs2), subs2);  
            }
            case S(COMPONENT):
            {
                SymbExpr *s = e->sub[0];
                if(s->type == S(TUPLE))
                {
                    if(s->sub.size() < e->number) throw std::string("Invalid index");
                    return compute(s->sub[e->number-1], subs);
                }
                return _comp(compute(s, subs), e->number, subs);
            }
            case S(PLUS):
            case S(MINUS):
            {
                CompExpr::ExprType type = e->type == S(PLUS) ? C(PLUS) : C(MINUS);
                CompExpr *c0 = compute(e->sub[0], subs);
                CompExpr *c1 = compute(e->sub[1], subs);
                if(c0->nTuple != c1->nTuple)
                    throw std::string("Tuple size mismatch");
                if(c0->nTuple == 1)
                    return op(type, c0, c1);
                CompExpr *c = op(C(TUPLE));
                c->nTuple = c0->nTuple;
                for(int i = 0; i < c->nTuple; i++)
                    c->sub.push_back(op(type, _comp(c0, i+1, subs), _comp(c0, i+1, subs)));
                return c;
            }
            case S(JUX):
            {
                CompExpr *c0 = compute(e->sub[0], subs);
                CompExpr *c1 = compute(e->sub[1], subs);
                if(c0->nTuple == 1 && c1->nTuple == 1)
                    return op(C(TIMES), c0, c1);
                if(c0->nTuple == 1 || c1->nTuple == 1)
                {
                    CompExpr *a, *b;
                    if(c0->nTuple == 1)
                        a = c0, b = c1;
                    else
                        a = c1, b = c0;
                    CompExpr *c = op(C(TUPLE));
                    c->nTuple = b->nTuple;
                    for(int i = 0; i < c->nTuple; i++)
                    {
                        CompExpr *t = _comp(b, i+1, subs);
                        c->sub.push_back(op(C(TIMES), a, t));
                    }
                    return c;
                }
                if(c0->nTuple != c1->nTuple)
                    throw std::string("Inconsistent tuple size");

                std::vector<CompExpr*> terms;
                for(int i = 0; i < c0->nTuple; i++)
                    terms.push_back(op(C(TIMES), _comp(c0, i+1, subs), _comp(c1, i+1, subs)));

                CompExpr *sum = terms[0];
                for(int i = 1; i < c0->nTuple; i++)
                {
                    CompExpr *nSum = op(C(PLUS), sum, terms[i]);
                    sum = nSum;
                }
                sum->nTuple = c0->nTuple;
                return sum;
            }
            case S(TIMES):
            {
                CompExpr *c0 = compute(e->sub[0], subs);
                CompExpr *c1 = compute(e->sub[1], subs);
                if(c0->nTuple == 1 && c1->nTuple == 1)
                    return op(C(TIMES), c0, c1);
                if(c0->nTuple == 1 || c1->nTuple == 1)
                {
                    CompExpr *a, *b;
                    if(c0->nTuple == 1)
                        a = c0, b = c1;
                    else
                        a = c1, b = c0;
                    CompExpr *c = op(C(TUPLE));
                    c->nTuple = b->nTuple;
                    for(int i = 0; i < c->nTuple; i++)
                        c->sub.push_back(op(C(TIMES), a, _comp(b, i+1, subs)));
                    return c;
                }
                if(c0->nTuple != 3 || c1->nTuple != 3)
                    throw std::string("Invalid cross-product");
                CompExpr *a1 = _comp(c0, 1, subs);
                CompExpr *a2 = _comp(c0, 2, subs);
                CompExpr *a3 = _comp(c0, 3, subs);
                CompExpr *b1 = _comp(c1, 1, subs);
                CompExpr *b2 = _comp(c1, 2, subs);
                CompExpr *b3 = _comp(c1, 3, subs);
                CompExpr *z1 = op(C(MINUS), op(C(TIMES), a2, b3), op(C(TIMES), a3, b2));
                CompExpr *z2 = op(C(MINUS), op(C(TIMES), a3, b1), op(C(TIMES), a1, b3));
                CompExpr *z3 = op(C(MINUS), op(C(TIMES), a1, b2), op(C(TIMES), a2, b1));
                CompExpr *c = op(C(TUPLE));
                c->sub.push_back(z1);
                c->sub.push_back(z2);
                c->sub.push_back(z3);
                c->nTuple = 3;
                return c;
            }
            case S(DIVIDE):
            {
                CompExpr *c0 = compute(e->sub[0], subs);
                CompExpr *c1 = compute(e->sub[1], subs);

                if(c1->nTuple > 1)
                    throw std::string("Cannot divide tuples");
                if(c0->nTuple == 1)
                    return op(C(DIVIDE), c0, c1);
                CompExpr *c = op(C(TUPLE));
                c->nTuple = c0->nTuple;
                for(int i = 0; i < c0->nTuple; i++)
                    c->sub.push_back(op(C(DIVIDE), _comp(c0, i+1, subs), c1));
                return c;
            }
            case S(UPLUS):
            case S(UMINUS):
            {
                CompExpr::ExprType type = e->type == S(UPLUS) ? C(UPLUS) : C(UMINUS);
                CompExpr *c0 = compute(e->sub[0], subs);
                if(c0->nTuple == 1)
                    return op(type, c0);
                CompExpr *c = op(C(TUPLE));
                c->nTuple = c0->nTuple;
                for(int i = 0; i < c0->nTuple; i++)
                    c->sub.push_back(op(type, _comp(c0, i+1, subs)));
                return c;
            }
            case S(UTIMES):
            {
                SymbExpr s = op(S(JUX), e->sub[0], e->sub[0]);
                return compute(&s, subs);
            }
            case S(UDIVIDE):
            {
                CompExpr *c0 = compute(e->sub[0], subs);
                if(c0->nTuple > 1)
                    throw std::string("Cannot invert tuples");
                return op(C(UDIVIDE), c0);
            }
            case S(POW):
            {
                CompExpr *c0 = compute(e->sub[0], subs);
                CompExpr *c1 = compute(e->sub[1], subs);
                
                if(c0->nTuple > 1 || c1->nTuple > 1)
                    throw std::string("Cannot exponentiate tuples");
                return op(C(POW), c0, c1);
            }
            case S(TUPLE):
            {
                CompExpr *c = op(C(TUPLE));
                c->nTuple = (int)e->sub.size();
                for(SymbExpr *s : e->sub)
                    c->sub.push_back(compute(s, subs));
                return c;
            }
            case S(TOTAL):
            case S(PARTIAL):
                return derivative(compute(e->sub[0], subs), e->name);
            default:
                throw std::string("Invalid expression type");
        }
    }

    CompExpr *Compiler::substitute(CompExpr *e, std::vector<Subst> &subs)
    {
        switch(e->type)
        {
            case C(CONSTANT):
            case C(NUMBER):
                return e;
            case C(VARIABLE):
            {
                for(Subst &s : subs)
                    if(s.var == e->name)
                        return s.exp;
                throw std::string("Variable definition missing");
            }
            case C(COMPONENT): return _comp(substitute(e->sub[0], subs), e->number, subs);
            case C(PLUS):
            case C(MINUS):
            case C(TIMES):
            case C(DIVIDE):
            case C(POW):
                return op(e->type, substitute(e->sub[0], subs), substitute(e->sub[1], subs));
            case C(UPLUS):
            case C(UMINUS):
            case C(UDIVIDE):
                return op(e->type, substitute(e->sub[0], subs));
            case C(APP):
            {
                if(e->sub[0]->type != C(FUNCTION)) throw std::string("Invalid type");
                return op(C(APP), e->sub[0], substitute(e->sub[1], subs));
            }
            case C(FUNCTION):
            {
                if(subs.size() != 1)
                    throw std::string("Must have 1 argument");
                if(subs[0].exp->nTuple > 1)
                    throw std::string("Must be a scalar");
                return op(C(APP), e, subs[0].exp);
            }
            case C(TUPLE):
            {
                CompExpr *c = op(C(TUPLE));
                c->nTuple = e->nTuple;
                for(CompExpr *e : e->sub)
                    c->sub.push_back(substitute(e, subs));
                return c;
            }
        }
    }

    #define FUN(x) op(C(FUNCTION), nullptr, nullptr, 0, x)

    CompExpr *Compiler::derivative(CompExpr *e, Table *var)
    {
        switch(e->type)
        {
            case C(CONSTANT):
            {
                CompExpr *z = op(C(NUMBER), nullptr, nullptr, 0);
                if(e->name->objIndex == -1) return z;
                Obj &o = objects[e->name->objIndex];
                if(o.type == param || o.type == grid)
                {
                    CompExpr *c = op(C(TUPLE));
                    c->nTuple = (int)o.intervals.size();
                    for(int i = 0; i < c->nTuple; i++)
                        c->sub.push_back(z);
                    return c;
                }
                throw std::string("Invalid type");
            }
            case C(NUMBER): return op(C(NUMBER), nullptr, nullptr, 0);
            case C(VARIABLE):
            {
                if(!var) return op(C(NUMBER), nullptr, nullptr, 1);
                return op(C(NUMBER), nullptr, nullptr, e->name == var ? 1 : 0);
            }
            case C(COMPONENT):
            {
                std::vector<Subst> subs;
                return _comp(derivative(e->sub[0], var), e->number, subs);
            }
            case C(PLUS):
            case C(MINUS):
                return op(e->type, derivative(e->sub[0], var), derivative(e->sub[1], var));
            case C(TIMES):
            {
                CompExpr *d0 = derivative(e->sub[0], var);
                CompExpr *d1 = derivative(e->sub[1], var);
                return op(C(PLUS), op(C(TIMES), d0, e->sub[1]), op(C(TIMES), d1, e->sub[0]));
            }
            case C(DIVIDE):
            {
                CompExpr *d0 = derivative(e->sub[0], var);
                CompExpr *d1 = derivative(e->sub[1], var);
                CompExpr *t = op(C(MINUS), op(C(TIMES), d0, e->sub[1]), op(C(TIMES), d1, e->sub[0]));
                return op(C(DIVIDE), t, op(C(POW), e->sub[1],
                op(C(NUMBER), nullptr, nullptr, 2)));
            }
            case C(POW):
            {
                CompExpr *a = op(C(TIMES), derivative(e->sub[1], var), op(C(APP), FUN(log), e->sub[0]));
                CompExpr *b = op(C(TIMES), e->sub[1], op(C(DIVIDE), derivative(e->sub[0], var), e->sub[0]));
                return op(C(TIMES), op(C(PLUS), a, b), e);
            }
            case C(UPLUS):
            case C(UMINUS):
                return op(e->type, derivative(e->sub[0], var));
            case C(UDIVIDE):
            {
                CompExpr *a = op(C(POW), e->sub[0], op(C(NUMBER), nullptr, nullptr, -2));
                CompExpr *d = derivative(e->sub[0], var);
                return op(C(TIMES), op(C(UMINUS), a), d);
            }
            case C(APP):
            {
                CompExpr *d = derivative(e->sub[0], nullptr);
                CompExpr *d2 = derivative(e->sub[1], var);
                std::vector<Subst> subs;
                subs.push_back({nullptr, e->sub[1]});
                return op(C(TIMES), substitute(d, subs), d2);
            }
            case C(FUNCTION):
            {
                if(e->name == sin) return FUN(cos);
                if(e->name == cos) return op(C(UMINUS), FUN(sin));
                if(e->name == tan)
                {
                    CompExpr *sec = op(C(UDIVIDE), FUN(cos));
                    return op(C(TIMES), sec, sec);
                }
                if(e->name == exp) return e;
                if(e->name == log) return op(C(UDIVIDE), FUN(id));
                if(e->name == sqrt)
                    return op(C(UDIVIDE), op(C(TIMES), 
                    op(C(NUMBER), nullptr, nullptr, 2), e));
                if(e->name == id) return op(C(NUMBER), nullptr, nullptr, 1);
            }
            case C(TUPLE):
            {
                CompExpr *c = op(C(TUPLE));
                c->nTuple = (int)e->sub.size();
                for(CompExpr *exp : e->sub)
                    c->sub.push_back(derivative(exp, var));
                return c;
            }
        }
    }

    #undef FUN

    void Compiler::compile(CompExpr *e, std::stringstream &str, int &v)
    {
        switch(e->type)
        {
            case C(NUMBER):
                str << "\tfloat v" << ++v << "=" << e->number << ";\n";
                break;
            case C(VARIABLE):
                str << "\tfloat v" << ++v << "=V" << e->name->getString() << ";\n";
                break;
            case C(CONSTANT):
                if(e->nTuple == 1)
                    str << "\tfloat v" << ++v << "=C" << e->name->getString() << ";\n";
                else
                {
                    for(int i = 0; i < e->nTuple; i++)
                        str << "\tfloat v" << ++v << "=C" << e->name->getString() << "_" << i+1 << ";\n";                    
                }
                break;
            case C(COMPONENT):
            {
                if(e->sub[0]->type != C(CONSTANT)) throw std::string("Invalid expression");
                Table *name = e->sub[0]->name;
                if(!name) throw std::string("Invalid expression");
                if(name->objIndex == -1) throw std::string("Invalid expression");
                Obj o = objects[name->objIndex];
                if(o.type != grid && o.type != param) throw std::string("Invalid expression");
                str << "\tfloat v" << ++v << "=C" << e->sub[0]->name->getString() << "_" << e->number << ";\n";
                break;
            }
            case C(PLUS):
            case C(MINUS):
            case C(TIMES):
            case C(DIVIDE):
            {
                char symb;
                if(e->type == C(PLUS)) symb = '+';
                if(e->type == C(MINUS)) symb = '-';
                if(e->type == C(TIMES)) symb = '*';
                if(e->type == C(DIVIDE)) symb = '/';
                compile(e->sub[0], str, v);
                int a = v;
                compile(e->sub[1], str, v);
                int b = v;
                str << "\tfloat v" << ++v << "=v"
                    << a << symb << "v" << b << ";\n";
                break;
            }
            case C(POW):
            {
                compile(e->sub[0], str, v);
                int a = v;
                if(e->sub[1]->type == C(NUMBER))
                {
                    float num = e->sub[1]->number;
                    if(num == (int)num && num >= -10 && num <= 10)
                    {
                        char symb = '*';
                        if(num < 0) symb = '/';

                        str << "\tfloat v" << ++v << "=1";
                        for(int i = 0; i < std::abs(num); i++)
                            str << symb << "v" << a;
                        str << ";\n";
                        break;   
                    }
                }
                compile(e->sub[1], str, v);
                int b = v;
                str << "\tfloat v" << ++v << "=pow(v"
                    << a << ",v" << b << ");\n";
                break;
            }
            case C(APP):
            {
                compile(e->sub[1], str, v);
                int a = v;
                if(e->sub[0]->name == id) break;
                str << "\tfloat v" << ++v << "=" << e->sub[0]->name->getString() << "(v"
                    << a << ");\n";
                break;
            }
            case C(UPLUS):
            case C(UMINUS):
            {
                char symb;
                if(e->type == C(UPLUS)) symb = '+';
                if(e->type == C(UMINUS)) symb = '-';
                compile(e->sub[0], str, v);
                int a = v;
                str << "\tfloat v" << ++v << "=" << symb << "v" << a << ";\n";
                break;
            }
            case C(FUNCTION):
                break;
            case C(UDIVIDE):
            {
                compile(e->sub[0], str, v);
                int k = v;
                str << "\tfloat v" << ++v << "=" << "1.0/v" << k << ";\n";
                break;
            }
            case C(TUPLE):
            {
                std::vector<int> indices;
                for(int i = 0; i < (int)e->sub.size(); i++)
                {
                    if(e->sub[i]->nTuple != 1)
                        throw std::string("Invalid tuple");
                    compile(e->sub[i], str, v);
                    indices.push_back(v);
                }
                for(int i = 0; i < (int)e->sub.size(); i++)
                    str << "\tfloat v" << ++v << "=" << "v" << indices[i] << ";\n";
                break;
            }
        }
    }


    static float quadData[] = 
    {
        -1.0f, -1.0f,
        +1.0f, -1.0f,
        +1.0f, +1.0f,

        -1.0f, -1.0f,
        -1.0f, +1.0f,
        +1.0f, +1.0f,
    };

    static float lineData[] = {0, 1};

    void Compiler::compile(const char *source)
    {
        if(compiled) throw std::string("Program is already compiled");
        compiled = true;

        parseProgram(source);
        std::vector<Subst> subs;

        std::stringstream hdr;
        header(hdr);
        
        glGenBuffers(1, &block.ID);
        glBindBuffer(GL_UNIFORM_BUFFER, block.ID);
        glBufferData(GL_UNIFORM_BUFFER, blockSize, NULL, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, block.ID);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        {
            glGenVertexArrays(1, &quad.ID);
            quad.buffers.push_back(new Buffer);
            Buffer *buf = quad.buffers.back();
            glGenBuffers(1, &buf->ID);
            glBindVertexArray(quad.ID);
            glBindBuffer(GL_ARRAY_BUFFER, buf->ID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }

        {
            glGenVertexArrays(1, &line.ID);
            line.buffers.push_back(new Buffer);
            Buffer *buf = line.buffers.back();
            glGenBuffers(1, &buf->ID);
            glBindVertexArray(line.ID);
            glBindBuffer(GL_ARRAY_BUFFER, buf->ID);
            glBufferData(GL_ARRAY_BUFFER, sizeof(lineData), lineData, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);
        }
        
        glGenFramebuffers(1, &frame.ID);
        frame.textures.push_back(new Texture);
        Texture *tex = frame.textures.back();
        tex->create({512, 512}, GL_RGB, GL_UNSIGNED_BYTE);
        glBindFramebuffer(GL_FRAMEBUFFER, frame.ID);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->ID, 0);
        uint b = GL_COLOR_ATTACHMENT0;
        glNamedFramebufferDrawBuffers(frame.ID, 1, &b);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        defaultFrag.compile(GL_FRAGMENT_SHADER, 
        "#version 460 core\n"
        "layout (location = 0) out vec4 color;\n"
        "void main()\n{\n"
        "color = vec4(1, 1, 1, 1);\n"
        "\n}\n"
        );

        defaultVert.compile(GL_VERTEX_SHADER, 
        "#version 460 core\n"
        "layout (location = 0) in vec2 pos;\n"
        "out vec2 opos;\n"
        "void main()\n{\n"
        "gl_Position = vec4(pos, 0, 1);\n"
        "opos = pos;\n"
        "\n}\n"
        );

        for(int objIndex = 0; objIndex < (int)objects.size(); objIndex++)
        {
            Obj &o = objects[objIndex];
            if(o.type == param)
            {
                for(Interval &i : o.intervals)
                {
                    i.compSub[0] = compute(i.sub[0], subs);
                    i.compSub[1] = compute(i.sub[1], subs);
                    dependencies(i.compSub[0], o.grids);
                    dependencies(i.compSub[1], o.grids);
                    i.min = calculate(i.compSub[0], subs);
                    i.max = calculate(i.compSub[1], subs);
                    if(i.max < i.min) throw std::string("Degenerate interval");
                    i.number = i.min;
                }
            }
            if(o.type == grid)
            {
                for(Interval &i : o.intervals)
                {
                    i.compSub[0] = compute(i.sub[0], subs);
                    i.compSub[1] = compute(i.sub[1], subs);
                    i.compSub[2] = compute(i.sub[2], subs);
                    dependencies(i.compSub[0], o.grids);
                    dependencies(i.compSub[1], o.grids);
                    dependencies(i.compSub[2], o.grids);
                    i.min = calculate(i.compSub[0], subs);
                    i.max = calculate(i.compSub[1], subs);
                    if(i.max < i.min) throw std::string("Degenerate interval");
                    i.number = floor(calculate(i.compSub[2], subs));
                    if(i.number < 1) throw std::string("Grids must have at least 1 point");
                }
            }
            if(o.type == curve)
            {
                std::stringstream str;
                if(argList[o.name->argIndex].size() != 1)
                    throw std::string("Curves should have 1 parameter");
                o.intervals[0].compSub[0] = compute(o.intervals[0].sub[0], subs);
                o.intervals[0].compSub[1] = compute(o.intervals[0].sub[1], subs);
                dependencies(o.intervals[0].compSub[0], o.grids);
                dependencies(o.intervals[0].compSub[1], o.grids);
                o.intervals[0].min = calculate(o.intervals[0].compSub[0], subs);
                o.intervals[0].max = calculate(o.intervals[0].compSub[1], subs);
                o.compSub[0] = compute(o.sub[0], subs);
                dependencies(o.compSub[0], o.grids, true);
                o.nTuple = o.compSub[0]->nTuple;
                if(o.nTuple != 2 && o.nTuple != 3)
                    throw std::string("Curves should be in 2d or 3d space");

                if(o.nTuple != 3) return;
                str << "#version 460 core\n" << hdr.str();

                SymbExpr ct = op(S(TOTAL), o.sub[0]);
                compileFunction(o.compSub[0], o.name->argIndex, str, "c");
                compileFunction(compute(&ct, subs), o.name->argIndex, str, "c_t");
                
                str << "layout (location = 0) in float t;\n";
                str << "void main()\n{\n";
                str << "gl_Position = camera*vec4(";
                str << "c(t), 1);\n}\n";
                
                o.program.shaders.push_back(new Shader);
                Shader *sh = o.program.shaders.back();
                sh->compile(GL_VERTEX_SHADER, str.str().c_str());
                o.program.ID = glCreateProgram();
                glAttachShader(o.program.ID, defaultFrag.ID);
                o.program.link();
                o.array.create1DGrid(20, o.intervals[0]);
            }

            if(o.type == surface)
            {
                std::stringstream str;
                if(argList[o.name->argIndex].size() != 2)
                    throw std::string("Surface should have 2 parameters");
                o.intervals[0].compSub[0] = compute(o.intervals[0].sub[0], subs);
                o.intervals[0].compSub[1] = compute(o.intervals[0].sub[1], subs);
                o.intervals[1].compSub[0] = compute(o.intervals[1].sub[0], subs);
                o.intervals[1].compSub[1] = compute(o.intervals[1].sub[1], subs);
                dependencies(o.intervals[0].compSub[0], o.grids);
                dependencies(o.intervals[0].compSub[1], o.grids);
                dependencies(o.intervals[1].compSub[0], o.grids);
                dependencies(o.intervals[1].compSub[1], o.grids);
                o.intervals[0].min = calculate(o.intervals[0].compSub[0], subs);
                o.intervals[0].max = calculate(o.intervals[0].compSub[1], subs);
                o.intervals[1].min = calculate(o.intervals[1].compSub[0], subs);
                o.intervals[1].max = calculate(o.intervals[1].compSub[1], subs);
                o.compSub[0] = compute(o.sub[0], subs);
                dependencies(o.compSub[0], o.grids, true);
                o.nTuple = o.compSub[0]->nTuple;

                if(o.nTuple != 3)
                    throw std::string("Surfaces should be in 3d space");
                
                str << "#version 460 core\n" << hdr.str();

                SymbExpr s_u = op(S(PARTIAL), o.sub[0], nullptr, 0, argList[o.name->argIndex][0]);
                SymbExpr s_v = op(S(PARTIAL), o.sub[0], nullptr, 0, argList[o.name->argIndex][1]);
                SymbExpr s_uu = op(S(PARTIAL), &s_u, nullptr, 0, argList[o.name->argIndex][0]);
                SymbExpr s_uv = op(S(PARTIAL), &s_u, nullptr, 0, argList[o.name->argIndex][1]);
                SymbExpr s_vv = op(S(PARTIAL), &s_v, nullptr, 0, argList[o.name->argIndex][1]);
                
                compileFunction(o.compSub[0], o.name->argIndex, str, "s");
                
                str << "layout (location = 0) in vec2 uv;\n";
                str << "out vec2 opos;\n";
                str << "void main()\n{\n";
                str << "opos = uv;\n";
                str << "gl_Position = camera*vec4(";
                str << "s(uv.x, uv.y), 1);\n}\n";

                {
                    o.program.shaders.push_back(new Shader);
                    Shader *sh = o.program.shaders.back();

                    sh->compile(GL_VERTEX_SHADER, str.str().c_str());
                    o.program.ID = glCreateProgram();
                    glAttachShader(o.program.ID, defaultFrag.ID);
                    o.program.link();
                    o.array.create2DGrid(20, 20, o.intervals[0], o.intervals[1]);
                }

                /*str = std::stringstream();

                str << "#version 460 core\n" << hdr.str();

                compileFunction(o.compSub[0], o.name->argIndex, str, "s");
                compileFunction(compute(&s_u, subs), o.name->argIndex, str,  "s_u");
                compileFunction(compute(&s_v, subs), o.name->argIndex, str,  "s_v");
                compileFunction(compute(&s_uu, subs), o.name->argIndex, str, "s_uu");
                compileFunction(compute(&s_uv, subs), o.name->argIndex, str, "s_uv");
                compileFunction(compute(&s_vv, subs), o.name->argIndex, str, "s_vv");

                str << "float E(float u, float v)   {return dot(s_u(u,v),s_u(u,v));}\n";
                str << "float E_u(float u, float v) {return 2*dot(s_u(u,v),s_uu(u,v));}\n";
                str << "float E_v(float u, float v) {return 2*dot(s_u(u,v),s_uv(u,v));}\n";

                str << "float F(float u, float v)   {return dot(s_u(u,v),s_v(u,v));}\n";
                str << "float F_u(float u, float v) {return dot(s_uu(u,v),s_v(u,v)) + dot(s_u(u,v),s_uv(u,v));}\n";
                str << "float F_v(float u, float v) {return dot(s_uv(u,v),s_v(u,v)) + dot(s_u(u,v),s_vv(u,v));}\n";

                str << "float G(float u, float v)   {return dot(s_v(u,v),s_v(u,v));}\n";
                str << "float G_u(float u, float v) {return 2*dot(s_v(u,v),s_uv(u,v));}\n";
                str << "float G_v(float u, float v) {return 2*dot(s_v(u,v),s_vv(u,v));}\n";

                str << "layout (location = 0) out vec4 color;\n";
                str << "in vec2 opos;\n";
                str << "void main()\n{\n";
                str << ""


                printf("%s\n", str.str().c_str());

                {
                    o.program2.shaders.push_back(new Shader);
                    Shader *sh = o.program.shaders.back();
                    sh->compile(GL_FRAGMENT_SHADER, str.str().c_str());
                    o.program2.ID = glCreateProgram();
                    glAttachShader(o.program2.ID, defaultVert.ID);
                    o.program2.link();
                }*/
            }

            if(o.type == point)
            {
                std::stringstream str;
                o.compSub[0] = compute(o.sub[0], subs);
                dependencies(o.compSub[0], o.grids, true);
                o.nTuple = o.compSub[0]->nTuple;
                if(o.nTuple != 2 && o.nTuple != 3)
                    throw std::string("Points must be in 2d or 3d space");

                if(o.nTuple != 3) return;
                str << "#version 460 core\n" << hdr.str();

                compileFunction(o.compSub[0], -1, str, "p");

                str << "void main()\n{\n";
                str << "gl_Position = camera*vec4(";
                str << "p(), 1);\n}\n";

                o.program.shaders.push_back(new Shader);
                o.program.shaders[0]->compile(GL_VERTEX_SHADER, str.str().c_str());
                o.program.shaders.push_back(new Shader);
                o.program.shaders[1]->compile(GL_FRAGMENT_SHADER, 
                "#version 460 core\n"
                "layout (location = 0) out vec4 color;\n"
                "void main()\n{\n"
                "float k = 1;\n"
                "if(length(gl_PointCoord - vec2(0.5, 0.5)) > 0.5) k = 0;\n"
                "color = vec4(k, k, 1-k, 1);\n"
                "\n}\n");

                o.program.link();
            }

            if(o.type == vector)
            {
                std::stringstream str;
                o.compSub[0] = compute(o.sub[0], subs);
                o.compSub[1] = compute(o.sub[1], subs);
                dependencies(o.compSub[0], o.grids, true);
                dependencies(o.compSub[1], o.grids, true);
                o.nTuple = o.compSub[0]->nTuple;
                if(o.nTuple != 2 && o.nTuple != 3)
                    throw std::string("Vectors must be in 2d or 3d space");
                if(o.compSub[1]->nTuple != o.nTuple)
                    throw std::string("Inconsistent space dimensions");

                if(o.nTuple != 3) return;
                str << "#version 460 core\n" << hdr.str();

                compileFunction(o.compSub[0], -1, str, "v");
                compileFunction(o.compSub[1], -1, str, "v_org");

                str << "layout (location = 0) in float t;\n";
                str << "void main()\n{\n";
                str << "gl_Position = camera*vec4(v_org()+t*v(), 1);\n}\n";

                o.program.shaders.push_back(new Shader);
                o.program.shaders[0]->compile(GL_VERTEX_SHADER, str.str().c_str());
                o.program.shaders.push_back(new Shader);
                o.program.shaders[1]->compile(GL_FRAGMENT_SHADER, 
                "#version 460 core\n"
                "layout (location = 0) out vec4 color;\n"
                "void main()\n{\n"
                "color = vec4(0, 0, 1, 1);\n"
                "\n}\n");

                o.program.link();
            }
        }
    }

    void Compiler::header(std::stringstream &str)
    {
        blockSize = 64;
        str << "layout (std140, binding = 0) uniform Header\n{\n\tmat4 camera;\n";
    
        for(Obj &o : objects)
        {
            if(o.type == param || o.type == grid)
            {
                for(int i = 0; i < (int)o.intervals.size(); i++)
                {
                    str << "\tfloat C" << o.name->getString();
                    if((int)o.intervals.size() != 1)
                        str << "_" << i+1;
                    str  << ";\n";
                    o.intervals[i].offset = blockSize;
                    blockSize += 4;
                }
            }
        }
        str << "};\n";
    }

    void Compiler::compileFunction(CompExpr *exp, int argIndex, std::stringstream &str, std::string name)
    {
        int N = exp->nTuple;
        declareFunction(N, argIndex, str, name);

        str << "\n{\n";
        int v = 0;
        compile(exp, str, v);

        if(N == 1)
            str << "\treturn v" << v << ";\n";
        else if(N == 2)
            str << "\treturn vec2(v" << v-1 << ", v" << v << ");\n";
        else if(N == 3)
            str << "\treturn vec3(v" << v-2 << ", v" << v-1 << ", v" << v << ");\n";

        str << "}\n";
    }

    void Compiler::declareFunction(int N, int argIndex, std::stringstream &str, std::string name, bool declareOnly)
    {
        if(N == 1)
            str << "float ";
        else if(N == 2 || N == 3)
            str << "vec" << N << " ";
        else throw std::string("Cannot compile tuples of more than 4 components");

        str << name << "(";
        if(argIndex != -1)
        {
            int args = argList[argIndex].size();
            for(int i = 0; i < args; i++)
            {
                str << "float V" << argList[argIndex][i]->getString();
                if(i < args-1) str << ", ";
            }
        }
        str << ")";
        if(declareOnly) str << ";\n";
    }

    float Compiler::calculate(CompExpr *e, std::vector<Subst> &subs)
    {
        switch(e->type)
        {
            case C(PLUS): return calculate(e->sub[0], subs) + calculate(e->sub[1], subs);
            case C(MINUS): return calculate(e->sub[0], subs) - calculate(e->sub[1], subs);
            case C(TIMES): return calculate(e->sub[0], subs) * calculate(e->sub[1], subs);
            case C(DIVIDE): return calculate(e->sub[0], subs) / calculate(e->sub[1], subs);
            case C(UPLUS): return +calculate(e->sub[0], subs);
            case C(UMINUS): return -calculate(e->sub[0], subs);
            case C(UDIVIDE): return 1.0/calculate(e->sub[0], subs);
            case C(APP):
            {
                if(e->sub[0]->type != C(FUNCTION))
                    throw std::string("Expected a function");
                Table *name = e->sub[0]->name;
                if(name == sin) return std::sin(calculate(e->sub[1], subs));
                if(name == cos) return std::cos(calculate(e->sub[1], subs));
                if(name == tan) return std::tan(calculate(e->sub[1], subs));
                if(name == exp) return std::exp(calculate(e->sub[1], subs));
                if(name == log) return std::log(calculate(e->sub[1], subs));
                if(name == sqrt) return std::sqrt(calculate(e->sub[1], subs));
                if(name == id) return calculate(e->sub[1], subs);
                throw std::string("Invalid function");
            }
            case C(POW): return std::pow(calculate(e->sub[0], subs), calculate(e->sub[1], subs));
            case C(COMPONENT):
            {
                if(e->sub[0]->type != C(CONSTANT)) throw std::string("Invalid component");
                CompExpr *c = e->sub[0];
                if(c->name->objIndex == -1)
                    throw std::string("Invalid constant");
                Obj &o = objects[c->name->objIndex];
                if(o.type == param || o.type == grid)
                    return o.intervals[e->number-1].number;
                throw std::string("Invalid constant");   
            }
            case C(CONSTANT):
            {
                if(e->name->objIndex == -1)
                {
                    if(e->name == this->e) return Parser::CE;
                    if(e->name == this->pi) return Parser::CPI;
                    throw std::string("Invalid constant");
                }
                Obj &o = objects[e->name->objIndex];
                if(o.type == param || o.type == grid)
                    return o.intervals[0].number;
                throw std::string("Invalid constant");
            }
            case C(NUMBER): return e->number;
            case C(VARIABLE):
            {
                for(Subst &s : subs)
                    if(s.var == e->name) return s.number;
                throw std::string("Variable definition missing");
            }
            default:
                throw std::string("Invalid type");
        }
    }

    void Compiler::dependencies(CompExpr *e, std::vector<int> &grids, bool allow)
    {
        switch(e->type)
        {
            case C(PLUS):
            case C(MINUS):
            case C(TIMES):
            case C(DIVIDE):
            case C(POW):
                dependencies(e->sub[0], grids, allow);
                dependencies(e->sub[1], grids, allow);
                break;
            case C(UPLUS):
            case C(UMINUS):
            case C(UDIVIDE):
                dependencies(e->sub[0], grids, allow);
                break;
            case C(APP):
                dependencies(e->sub[1], grids, allow);
                break;
            case C(CONSTANT):
                if(e->name->objIndex == -1) return;
                if(!allow)
                    throw std::string("Invalid dependency");
                if(objects[e->name->objIndex].type == grid)
                {
                    for(int k : grids)
                        if(k == e->name->objIndex)
                            return;
                    grids.push_back(e->name->objIndex);
                }
                break;
            case C(COMPONENT):
                if(!allow)
                    throw std::string("Invalid dependency");
                if(objects[e->sub[0]->name->objIndex].type == grid)
                {
                    for(int k : grids)
                        if(k == e->sub[0]->name->objIndex)
                            return;
                    grids.push_back(e->sub[0]->name->objIndex);
                }
                break;
            case C(FUNCTION):
            case C(VARIABLE):
            case C(NUMBER): break;
            case C(TUPLE):
                for(CompExpr *c : e->sub)
                    dependencies(c, grids, allow);
                break;
        }
    }

    Framebuffer::~Framebuffer()
    {
        for(Texture *t : textures)
            delete t;
        textures.clear();
        if(ID) glDeleteFramebuffers(1, &ID);
    }

    void Shader::compile(uint type, const char *source)
    {
        ID = glCreateShader(type);
        glShaderSource(ID, 1, &source, NULL);
        glCompileShader(ID);
        int success;
        static char infoLog[512];
        glGetShaderiv(ID, GL_COMPILE_STATUS, &success);
        if(!success)
        {
            glGetShaderInfoLog(ID, 512, NULL, infoLog);
            throw std::string("Cannot compile shader: ") + infoLog;
        }
    }

    Shader::~Shader()
    {
        if(ID) glDeleteShader(ID);
    }

    void Program::link()
    {
        if(!ID) ID = glCreateProgram();
        for(Shader *s : shaders)
            glAttachShader(ID, s->ID);
        glLinkProgram(ID);
        int success;
        static char infoLog[512];
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if(!success) 
        {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            throw std::string("Cannot link shader: ") + infoLog;
        }
    }

    Program::~Program()
    {
        for(Shader *s : shaders)
            delete s;
        shaders.clear();
        if(ID) glDeleteProgram(ID);
    }

    void Texture::create(Size s, uint base, uint type)
    {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, base, s.width, s.height, 0, base, type, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    Texture::~Texture()
    {
        if(ID) glDeleteTextures(1, &ID);
    }

    Buffer::~Buffer()
    {
        if(ID) glDeleteBuffers(1, &ID);
    }

    
    void Array::create2DGrid(uint nx, uint ny, Interval &i, Interval &j)
    {
        float *data = new float[nx*ny*2];
        for(uint x = 0; x < nx; x++)
        for(uint y = 0; y < ny; y++)
        {
            data[2*(x + y*nx)+0] = i.min + (i.max - i.min)*x/(nx-1);
            data[2*(x + y*nx)+1] = j.min + (j.max - j.min)*y/(ny-1);
        }

        uint *eData = new uint[(nx-1)*(ny-1)*6];

        for(uint x = 0; x < nx-1; x++)
        for(uint y = 0; y < ny-1; y++)
        {
            eData[6*(x + y*(nx-1))+0] = x+0 + (y+0)*nx;
            eData[6*(x + y*(nx-1))+1] = x+0 + (y+1)*nx;
            eData[6*(x + y*(nx-1))+2] = x+1 + (y+0)*nx;
            eData[6*(x + y*(nx-1))+3] = x+1 + (y+0)*nx;
            eData[6*(x + y*(nx-1))+4] = x+0 + (y+1)*nx;
            eData[6*(x + y*(nx-1))+5] = x+1 + (y+1)*nx;
        }

        glGenVertexArrays(1, &ID);
        glBindVertexArray(ID);

        {
            buffers.push_back(new Buffer);
            Buffer *buf = buffers.back();
            glGenBuffers(1, &buf->ID);
            glBindBuffer(GL_ARRAY_BUFFER, buf->ID);
            glBufferData(GL_ARRAY_BUFFER, nx*ny*2*sizeof(float), data, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }

        {
            buffers.push_back(new Buffer);
            Buffer *buf = buffers.back();
            glGenBuffers(1, &buf->ID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->ID);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (nx-1)*(ny-1)*6*sizeof(uint), eData, GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
        i.number = nx;
        j.number = ny;
        delete[] data;
        delete[] eData;
    }
    

    void Array::create1DGrid(uint nx, Interval &i)
    {
        float *data = new float[nx];

        for(uint x = 0; x < nx; x++)
            data[x] = i.min + (i.max-i.min)*x/(nx-1);

        glGenVertexArrays(1, &ID);
        glBindVertexArray(ID);
        buffers.push_back(new Buffer);
        Buffer *buf = buffers.back();
        glGenBuffers(1, &buf->ID);
        glBindBuffer(GL_ARRAY_BUFFER, buf->ID);
        glBufferData(GL_ARRAY_BUFFER, nx*sizeof(float), data, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 1 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        i.number = nx;
        delete[] data;
    }

    Array::~Array()
    {
        for(Buffer *b : buffers)
            delete b;
        buffers.clear();
        if(ID) glDeleteVertexArrays(1, &ID);
    }
};

#undef S
#undef C