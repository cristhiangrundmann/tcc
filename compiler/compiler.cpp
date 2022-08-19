#include "compiler.hpp"
#include <cmath>

#define S(x) Parser::ExprType::x
#define C(x) CompExpr::ExprType::x

namespace tcc
{

    void Compiler::actInt(ExprType type)
    {
        SymbExpr *e[3]{};
        e[0] = expStack.back(); expStack.pop_back();
        e[1] = expStack.back(); expStack.pop_back();
        if(type == S(GRID))
        {
            e[2] = expStack.back();
            expStack.pop_back();
        }
        Interval i = {type, tag, wrap, {e[0], e[1], e[2]}};
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
            while(!intStack.empty())
            {
                obj.intervals.push_back(intStack.back());
                intStack.pop_back();
            }
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
            if(objType != function) while(!intStack.empty())
            {
                obj.intervals.push_back(intStack.back());
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
                    return op(C(NUMBER), nullptr, nullptr, a->number + b->number);
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
                str << "\tfloat v" << ++v << "=(float)" << e->number << ";\n";
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

    void Compiler::compile(std::stringstream &str)
    {
        for(Obj &o : objects)
        {
            if(o.type == curve)
            {
                int args = argList[o.name->argIndex].size();
                if(args != 1)
                    throw std::string("Curves should have 1 parameter");

                std::vector<Subst> subs;
                SymbExpr *c = o.sub[0];
                CompExpr *cc = compute(c, subs);

                int N = cc->nTuple;
                o.nTuple = N;
                if(N != 2 && N != 3)
                    throw std::string("Curves should be in 2d or 3d space");

                SymbExpr ct = op(S(TOTAL), c);

                compileFunction(cc, o.name->argIndex, str, o.name->getString());
                compileFunction(compute(&ct, subs), o.name->argIndex, str, o.name->getString() + "_t");
            }
            if(o.type == surface)
            {
                int args = argList[o.name->argIndex].size();

                if(args != 2)
                    throw std::string("Surface should have 2 parameters");

                std::vector<Subst> subs;

                SymbExpr *s = o.sub[0];
                CompExpr *cs = compute(s, subs);

                int N = cs->nTuple;
                o.nTuple = N;
                if(N != 3)
                    throw std::string("Surfaces should be in 3d space");

                SymbExpr s_u = op(S(PARTIAL), s, nullptr, 0, argList[o.name->argIndex][0]);
                SymbExpr s_v = op(S(PARTIAL), s, nullptr, 0, argList[o.name->argIndex][1]);
                SymbExpr s_uu = op(S(PARTIAL), &s_u, nullptr, 0, argList[o.name->argIndex][0]);
                SymbExpr s_uv = op(S(PARTIAL), &s_u, nullptr, 0, argList[o.name->argIndex][1]);
                SymbExpr s_vv = op(S(PARTIAL), &s_v, nullptr, 0, argList[o.name->argIndex][1]);

                compileFunction(cs, o.name->argIndex, str, o.name->getString());
                compileFunction(compute(&s_u, subs), o.name->argIndex, str, o.name->getString() + "_u");
                compileFunction(compute(&s_v, subs), o.name->argIndex, str, o.name->getString() + "_v");
                compileFunction(compute(&s_uu, subs), o.name->argIndex, str, o.name->getString() + "_uu");
                compileFunction(compute(&s_uv, subs), o.name->argIndex, str, o.name->getString() + "_uv");
                compileFunction(compute(&s_vv, subs), o.name->argIndex, str, o.name->getString() + "_vv");

            }
            if(o.type == function)
            {
                std::vector<Subst> subs;
                CompExpr *cf = compute(o.sub[0], subs);
                try
                {
                    int args = argList[o.name->argIndex].size();
                    if(args != 1 && args != 2)
                        throw std::string("Functions should have 1 or 2 parameters");
                    int N = cf->nTuple;
                    o.nTuple = N;
                    if(N != 1) throw std::string("Function is not plottable");

                    std::stringstream buf;
                    compileFunction(cf, o.name->argIndex, buf, o.name->getString());
                    str << buf.str();
                } catch(std::string &error){}
            }
            if(o.type == point)
            {
                std::vector<Subst> subs;
                CompExpr *cp = compute(o.sub[0], subs);
                int N = cp->nTuple;
                o.nTuple = N;
                if(N != 2 && N != 3) throw std::string("Points must be in 2d or 3d space");

                compileFunction(cp, -1, str, o.name->getString());
            }

            if(o.type == vector)
            {
                std::vector<Subst> subs;
                CompExpr *cv = compute(o.sub[0], subs);
                CompExpr *cv2 = compute(o.sub[1], subs);
                int N = cv->nTuple;
                o.nTuple = N;
                if(N != 2 && N != 3) throw std::string("Vectors must be in 2d or 3d space");
                if(cv2->nTuple != N) throw std::string("Inconsistent space dimensions");

                compileFunction(cv, -1, str, o.name->getString());
                compileFunction(cv2, -1, str, o.name->getString()+"_org");
            }
        }
    }

    void Compiler::header(std::stringstream &str)
    {
        for(Obj &o : objects)
        {
            if(o.type == param || o.type == grid)
            {
                for(int i = 0; i < (int)o.intervals.size(); i++)
                {
                    str << "uniform float C" << o.name->getString();
                    if((int)o.intervals.size() != 1)
                        str << "_" << i+1;
                    str  << ";\n";
                }
            }
            if(o.type == curve)
            {
                int N = o.nTuple;
                declareFunction(N, o.name->argIndex, str, o.name->getString(), true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_t", true);
                str << "uniform float F" << o.name->getString() + "_mint;\n";
                str << "uniform float F" << o.name->getString() + "_maxt;\n";
            }
            if(o.type == surface)
            {
                int N = o.nTuple;
                declareFunction(N, o.name->argIndex, str, o.name->getString(), true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_u", true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_v", true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_uu", true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_uv", true);
                declareFunction(N, o.name->argIndex, str, o.name->getString() + "_vv", true);
                str << "uniform float F" << o.name->getString() + "_minu;\n";
                str << "uniform float F" << o.name->getString() + "_maxu;\n";
                str << "uniform float F" << o.name->getString() + "_minv;\n";
                str << "uniform float F" << o.name->getString() + "_maxv;\n";
            }
            if(o.type == function)
            {
                int N = o.nTuple;
                if(N != 1) continue;
                int args = argList[o.name->argIndex].size();
                if(args != 1 && args != 2) continue;
                declareFunction(N, o.name->argIndex, str, o.name->getString(), true);
            }
            if(o.type == point)
            {
                int N = o.nTuple;
                declareFunction(N, -1, str, o.name->getString(), true);
            }

            if(o.type == vector)
            {
                int N = o.nTuple;
                declareFunction(N, -1, str, o.name->getString(), true);
                declareFunction(N, -1, str, o.name->getString()+"_org", true);
            }
        }
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
            str << "float";
        else if(N == 2 || N == 3)
            str << "vec" << N;
        else throw std::string("Cannot compile tuples of more than 4 components");

        str << " F" << name << "(";
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
                    return o.intervals[e->number].number;
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
            default:
                throw std::string("Invalid type");
        }
    }
};

#undef S
#undef C