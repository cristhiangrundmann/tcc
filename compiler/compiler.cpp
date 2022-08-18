#include "compiler.hpp"
#include <cmath>

using std::make_unique;
using std::unique_ptr;

#define E(x) Parser::ExprType::x

namespace tcc
{

    void Compiler::actInt(ExprType type)
    {
        Expr *e[3]{};
        e[0] = compute(expStack.back()); expStack.pop_back();
        e[1] = compute(expStack.back()); expStack.pop_back();
        if(type == E(GRID))
        {
            e[2] = compute(expStack.back());
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
            case E(POW):
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

    Expr *Compiler::newExpr(Expr &e)
    {
        expressions.push_back(std::make_unique<Expr>(e));
        return expressions.back().get();
    }

    Expr *Compiler::op(ExprType type, Expr *a, Expr *b, Table *name, float number)
    {
        Expr e;
        e.type = type;
        if(a) e.sub.push_back(a);
        if(a && b) e.sub.push_back(b);
        e.name = name;
        e.number = number;
        return newExpr(e);
    }

    #define COMP(x, y) compute(op(E(COMPONENT), x, nullptr, nullptr, y))

    Expr *Compiler::compute(Expr *e)
    {
        if(e->compute) return e->compute;
        switch(e->type)
        {
            case E(NUMBER):
                e->tupleSize = 1;
                return e->compute = e;
            case E(CONSTANT):
            {
                if(e->name->objIndex == -1)
                {
                    e->tupleSize = 1;
                    return e->compute = e;
                }
                Obj obj = objects[e->name->objIndex];
                if(obj.type == define) return e->compute = compute(obj.sub[0]);
                if(obj.type == param || obj.type == grid)
                {
                    e->tupleSize = obj.intervals.size();
                    return e->compute = e;
                }
                if(obj.type == point || obj.type == vector)
                    return e->compute = compute(obj.sub[0]);
            }
            case E(VARIABLE):
            {
                e->tupleSize = 1;
                return e->compute = e;
            }
            case E(FUNCTION):
            {
                if(e->name->objIndex == -1)
                {
                    e->tupleSize = 1;
                    return e->compute = e;
                }
                return e->compute = compute(objects[e->name->objIndex].sub[0]);
            }
            case E(COMPONENT):
            {
                Expr *a = compute(e->sub[0]);
                if((e->number > a->tupleSize) && (a->type != E(VARIABLE)))
                    throw std::string("Invalid tuple index");
                if(a->type == E(TUPLE))
                    return e->compute = compute(a->sub[e->number-1]);
                Expr *r = op(E(COMPONENT), a, nullptr, nullptr, e->number);
                r->tupleSize = 1;
                return e->compute = r->compute = r;
            }
            case E(PLUS):
            case E(MINUS):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *e1 = compute(e->sub[1]);

                if((e0->type == E(NUMBER)) && (e1->type == E(NUMBER)))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, e0->number + (e->type == E(PLUS) ? 1 : -1)*e1->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->type == E(NUMBER) && e0->number == 0)
                {
                    if(e->type == E(PLUS)) return e1;
                    return op(E(UMINUS), e1);
                }
                if(e1->type == E(NUMBER) && e1->number == 0) return e->compute = e0; 

                if(e0->tupleSize != e1->tupleSize)
                    throw std::string("Tuple size mismatch");
                if(e0->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return e->compute = r->compute = r;
                }
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < t->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1), COMP(e1, i+1))));
                return e->compute = t->compute = t;
            }
            case E(TIMES):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *e1 = compute(e->sub[1]);

                if((e0->type == E(NUMBER)) && (e1->type == E(NUMBER)))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, e0->number * e1->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->type == E(NUMBER) && e0->number == 0) return e->compute = e0;
                if(e1->type == E(NUMBER) && e1->number == 0) return e->compute = e1; 
                if(e0->type == E(NUMBER) && e0->number == 1) return e->compute = e1;
                if(e1->type == E(NUMBER) && e1->number == 1) return e->compute = e0;

                if(e0->tupleSize == 1 && e1->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return e->compute = r->compute = r;
                }
                if(e0->tupleSize == 1 || e1->tupleSize == 1)
                {
                    Expr *a, *b;
                    if(e0->tupleSize == 1)
                        a = e0, b = e1;
                    else
                        a = e1, b = e0;
                    Expr *t = op(E(TUPLE));
                    t->tupleSize = b->tupleSize;
                    for(int i = 0; i < t->tupleSize; i++)
                        t->sub.push_back(compute(op(e->type, a, COMP(b, i+1))));
                    return e->compute = t->compute = t;
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
                return e->compute = compute(t);
            }
            case E(DIVIDE):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *e1 = compute(e->sub[1]);

                if((e0->type == E(NUMBER)) && (e1->type == E(NUMBER)))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, e0->number / e1->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->type == E(NUMBER) && e0->number == 0) return e->compute = e0;
                if(e1->type == E(NUMBER) && e1->number == 1) return e->compute = e0; 

                if(e1->tupleSize > 1)
                    throw std::string("Cannot divide tuples");
                if(e0->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = e0->tupleSize;
                    return e->compute = r->compute = r;
                }
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < e0->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1), e1)));
                return e->compute = t->compute = t;
            }
            case E(JUX):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *e1 = compute(e->sub[1]);

                if((e0->type == E(NUMBER)) && (e1->type == E(NUMBER)))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, e0->number * e1->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->type == E(NUMBER) && e0->number == 0) return e->compute = e0;
                if(e1->type == E(NUMBER) && e1->number == 0) return e->compute = e1;
                if(e0->type == E(NUMBER) && e0->number == 1) return e->compute = e1;
                if(e1->type == E(NUMBER) && e1->number == 1) return e->compute = e0;             

                if(e0->tupleSize == 1 && e1->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0, e1);
                    r->tupleSize = 1;
                    return e->compute = r->compute = r;
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
                        t->sub.push_back(compute(op(e->type, a, COMP(b, i+1))));
                    return e->compute = t->compute = t;
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
                    Expr *nSum = op(E(PLUS), sum, terms[i]);
                    sum = nSum;
                }
                sum->tupleSize = sum->sub.size();
                return e->compute = compute(sum);
            }
            case E(UPLUS):
            case E(UMINUS):
            {
                Expr *e0 = compute(e->sub[0]);

                if(e0->type == E(NUMBER))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, (e->type == E(UPLUS) ? 1 : -1)*e0->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->tupleSize == 1)
                {
                    Expr *r = op(e->type, e0);
                    r->tupleSize = 1;
                    return e->compute = r->compute = r;
                }
                Expr *t = op(E(TUPLE));
                t->tupleSize = e0->tupleSize;
                for(int i = 0; i < e0->tupleSize; i++)
                    t->sub.push_back(compute(op(e->type, COMP(e0, i+1))));
                return e->compute = t->compute = t;
            }
            case E(UTIMES):
            {
                Expr *e0 = compute(e->sub[0]);
                return e->compute = compute(op(E(JUX), e0, e0));
            }
            case E(UDIVIDE):
            {
                Expr *e0 = compute(e->sub[0]);

                if(e0->type == E(NUMBER))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, 1.0/e0->number);
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }

                if(e0->tupleSize > 1)
                    throw std::string("Cannot invert tuples");
                Expr *r = op(e->type, e0);
                r->tupleSize = e0->tupleSize;
                return e->compute = r->compute = r;
            }
            case E(APP):
            {
                Expr *f = e->sub[0];
                while(f->type != E(FUNCTION)) f = f->sub[0];
                int argCount = 1;
                if(f->name->argIndex != -1)
                    argCount = argList[f->name->argIndex].size();
                
                std::vector<Subst> substs;
                if(argCount == 1)
                {
                    Table *name = nullptr;
                    if(f->name->argIndex != -1)
                        name = argList[f->name->argIndex][0];
                    substs.push_back({name, compute(e->sub[1])});
                }
                else
                {
                    Expr *t = compute(e->sub[1]);
                    if(t->type != E(TUPLE))
                        throw std::string("Expected a tuple of arguments");
                    if(t->sub.size() != (unsigned int)argCount)
                        throw std::string("Wrong number of arguments");
                    for(int i = 0; i < argCount; i++)
                        substs.push_back({
                            argList[f->name->argIndex][i],
                            t->sub[i]});
                }
                Expr *e0 = compute(e->sub[0]);
                Expr *r = substitute(e0, substs);
                if(f->name->argIndex != -1)
                    r = compute(r);
                else
                    r->tupleSize = 1;
                return e->compute = r->compute = r;
            }
            case E(TOTAL):
            case E(PARTIAL):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *r = compute(derivative(e0, e->name));
                r->tupleSize = e0->tupleSize;
                return e->compute = r->compute = r;
            }
            case E(POW):
            {
                Expr *e0 = compute(e->sub[0]);
                Expr *e1 = compute(e->sub[1]);

                if((e0->type == E(NUMBER)) && (e1->type == E(NUMBER)))
                {
                    Expr *t = op(E(NUMBER), nullptr, nullptr, nullptr, pow(e0->number, e1->number));
                    t->tupleSize = 1;
                    return e->compute = t->compute = t;
                }
                
                if(e0->tupleSize > 1 || e1->tupleSize > 1)
                    throw std::string("Cannot exponentiate tuples");
                Expr *r = op(E(POW), e0, e1);
                r->tupleSize = e0->tupleSize;
                return e->compute = r->compute = r;
            }
            case E(TUPLE):
            {
                Expr *t = op(E(TUPLE));
                t->tupleSize = e->sub.size();
                for(int i = 0; i < t->tupleSize; i++)
                    t->sub.push_back(compute(e->sub[i]));
                return e->compute = t->compute = t;
            }
            default:
                throw std::string("Invalid expression type");
        }
    }

    #undef COMP

    #define FUN(x) op(E(FUNCTION), nullptr, nullptr, x)

    Expr *Compiler::derivative(Expr *e, Table *var)
    {
        switch(e->type)
        {
            case E(NUMBER):
                return op(E(NUMBER), nullptr, nullptr, nullptr, 0);
            case E(COMPONENT):
                return op(E(COMPONENT), derivative(e->sub[0], var), nullptr, nullptr, e->number);
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
                if(!var) return op(E(NUMBER), nullptr, nullptr, nullptr, 1);
                return op(E(NUMBER), nullptr, nullptr, nullptr, var == e->name ? 1 : 0);
            case E(FUNCTION):
            {
                if(e->name == sin) return FUN(cos);
                if(e->name == cos) return op(E(UMINUS), FUN(sin));
                if(e->name == tan)
                {
                    Expr *sec = op(E(UDIVIDE), FUN(cos));
                    return op(E(TIMES), sec, sec);
                }
                if(e->name == exp) return e;
                if(e->name == log) return op(E(UDIVIDE), FUN(id));
                if(e->name == sqrt)
                    return op(E(UDIVIDE), op(E(TIMES), 
                    op(E(NUMBER), nullptr, nullptr, nullptr, 2), e));
                if(e->name == id) return op(E(NUMBER), nullptr, nullptr, nullptr, 1);
            }
            case E(PLUS):
            case E(MINUS):
                return op(e->type, derivative(e->sub[0], var), derivative(e->sub[1], var));
            case E(TIMES):
            case E(JUX):
            {
                Expr *d0 = derivative(e->sub[0], var);
                Expr *d1 = derivative(e->sub[1], var);
                return op(E(PLUS), op(e->type, d0, e->sub[1]), op(E(TIMES), e->sub[0], d1));
            }
            case E(DIVIDE):
            {
                Expr *d0 = derivative(e->sub[0], var);
                Expr *d1 = derivative(e->sub[1], var);
                Expr *t = op(E(MINUS), op(e->type, d0, e->sub[1]), op(E(TIMES), e->sub[0], d1));
                return op(E(DIVIDE), t, op(E(POW), e->sub[1],
                op(E(NUMBER), nullptr, nullptr, nullptr, 2)));
            }
            case E(UPLUS):
            case E(UMINUS):
                return op(e->type, derivative(e->sub[0], var));
            case E(UTIMES):
                throw std::string("Invalid expression type");
            case E(UDIVIDE):
            {
                Expr *a = op(E(POW), e->sub[0], op(E(NUMBER), nullptr, nullptr, nullptr, -2));
                return op(E(UMINUS), op(E(UDIVIDE), a));
            }
            case E(APP):
            {
                Expr *d = derivative(e->sub[0], nullptr);
                Expr *d2 = derivative(e->sub[1], var);
                return op(E(TIMES), op(E(APP), d, e->sub[1]), d2);
            }
            case E(TOTAL):
            case E(PARTIAL):
                throw std::string("Derivatives should be gone");
            case E(POW):
            {
                if(e->sub[1]->type == E(NUMBER))
                {
                    float num = e->sub[1]->number;
                    Expr *b = op(E(TIMES), e->sub[1], derivative(e->sub[0], var));
                    return op(E(TIMES), b, op(E(POW), e->sub[0], op(E(NUMBER), nullptr, nullptr, nullptr, num-1)));
                }
                Expr *a = op(E(TIMES), derivative(e->sub[1], var), op(E(APP), FUN(log), e->sub[0]));
                Expr *b = op(E(TIMES), e->sub[1], op(E(DIVIDE), derivative(e->sub[0], var), e->sub[0]));
                return op(E(TIMES), op(E(PLUS), a, b), e);
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

    #undef FUN

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
            case E(POW):
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

    void Compiler::compile(Expr *e, std::stringstream &str, int &v)
    {
        switch(e->type)
        {
            case E(NUMBER):
                str << "float v" << ++v << "=(float)" << e->number << ";\n";
                break;
            case E(VARIABLE):
                str << "float v" << ++v << "=V" << e->name->getString() << ";\n";
                break;
            case E(CONSTANT):
                if(e->tupleSize == 1)
                    str << "float v" << ++v << "=C" << e->name->getString() << ";\n";
                else
                {
                    for(int i = 0; i < e->tupleSize; i++)
                        str << "float v" << ++v << "=C" << e->name->getString() << "_" << i+1 << ";\n";                    
                }
                break;
            case E(COMPONENT):
            {
                if(e->sub[0]->type != E(CONSTANT)) throw std::string("Invalid expression");
                Table *name = e->sub[0]->name;
                if(!name) throw std::string("Invalid expression");
                if(name->objIndex == -1) throw std::string("Invalid expression");
                Obj o = objects[name->objIndex];
                if(o.type != grid && o.type != param) throw std::string("Invalid expression");
                str << "float v" << ++v << "=C" << e->sub[0]->name->getString() << "_" << e->number << ";\n";
                break;
            }
            case E(PLUS):
            case E(MINUS):
            case E(TIMES):
            case E(DIVIDE):
            case E(JUX):
            {
                char symb;
                if(e->type == E(PLUS)) symb = '+';
                if(e->type == E(MINUS)) symb = '-';
                if(e->type == E(TIMES)) symb = '*';
                if(e->type == E(DIVIDE)) symb = '/';
                if(e->type == E(JUX)) symb = '*';
                compile(e->sub[0], str, v);
                int a = v;
                compile(e->sub[1], str, v);
                int b = v;
                str << "float v" << ++v << "=v"
                    << a << symb << "v" << b << ";\n";
                break;
            }
            case E(POW):
            {
                compile(e->sub[0], str, v);
                int a = v;
                if(e->sub[1]->type == E(NUMBER))
                {
                    float num = e->sub[1]->number;
                    if(num == (int)num && num >= -10 && num <= 10)
                    {
                        char symb = '*';
                        if(num < 0) symb = '/';

                        str << "float v" << ++v << "=1";
                        for(int i = 0; i < std::abs(num); i++)
                            str << symb << "v" << a;
                        str << ";\n";
                        break;   
                    }
                }
                compile(e->sub[1], str, v);
                int b = v;
                str << "float v" << ++v << "=pow(v"
                    << a << ",v" << b << ");\n";
                break;
            }
            case E(APP):
            {
                compile(e->sub[1], str, v);
                int a = v;
                str << "float v" << ++v << "=" << e->sub[0]->name->getString() << "(v"
                    << a << ");\n";
                break;
            }
            case E(UPLUS):
            case E(UMINUS):
            {
                char symb;
                if(e->type == E(UPLUS)) symb = '+';
                if(e->type == E(UMINUS)) symb = '-';
                compile(e->sub[0], str, v);
                int a = v;
                str << "float v" << ++v << "=" << symb << "v" << a << ";\n";
                break;
            }
            case E(UTIMES):
            case E(FUNCTION):
                break;
            case E(UDIVIDE):
            {
                compile(e->sub[0], str, v);
                int k = v;
                str << "float v" << ++v << "=" << "1.0/v" << k << ";\n";
                break;
            }
            case E(TUPLE):
            {
                std::vector<int> indices;
                for(int i = 0; i < (int)e->sub.size(); i++)
                {
                    if(e->sub[i]->tupleSize != 1)
                        throw std::string("Invalid tuple");
                    compile(e->sub[i], str, v);
                    indices.push_back(v);
                }
                for(int i = 0; i < (int)e->sub.size(); i++)
                    str << "float v" << ++v << "=" << "v" << indices[i] << ";\n";
                break;
            }
            default:
                throw std::string("Invalid expression type");
        }
    }

    void Compiler::compile(std::stringstream &str, bool declareOnly)
    {
        for(Obj o : objects)
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
            if(o.type == define)
            {
                float c = calc(compute(o.sub[0]));
                printf("define = %f\n", c);
            }
            if(o.type == curve)
            {
                int args = argList[o.name->argIndex].size();
                if(args != 1)
                    throw std::string("Curves should have 1 parameter");

                Expr *c = o.sub[0];
                Expr *cc = compute(c);

                int N = cc->tupleSize;
                if(N != 2 && N != 3)
                    throw std::string("Curves should be in 2d or 3d space");

                Expr *ct = op(E(TOTAL), c);

                compileFunction(cc, o.name->argIndex, str, o.name->getString(), declareOnly);
                compileFunction(compute(ct), o.name->argIndex, str, o.name->getString() + "_t", declareOnly);                
            }
            if(o.type == surface)
            {
                int args = argList[o.name->argIndex].size();

                if(args != 2)
                    throw std::string("Surface should have 2 parameters");

                Expr *s = o.sub[0];
                Expr *cs = compute(s);

                int N = cs->tupleSize;
                if(N != 3)
                    throw std::string("Surfaces should be in 3d space");

                Expr *s_u = op(E(PARTIAL), s, nullptr, argList[o.name->argIndex][0]);
                Expr *s_v = op(E(PARTIAL), s, nullptr, argList[o.name->argIndex][1]);
                Expr *s_uu = op(E(PARTIAL), s_u, nullptr, argList[o.name->argIndex][0]);
                Expr *s_uv = op(E(PARTIAL), s_u, nullptr, argList[o.name->argIndex][1]);
                Expr *s_vv = op(E(PARTIAL), s_v, nullptr, argList[o.name->argIndex][1]);

                compileFunction(cs, o.name->argIndex, str, o.name->getString(), declareOnly);
                compileFunction(compute(s_u), o.name->argIndex, str, o.name->getString() + "_u", declareOnly);
                compileFunction(compute(s_v), o.name->argIndex, str, o.name->getString() + "_v", declareOnly);
                compileFunction(compute(s_uu), o.name->argIndex, str, o.name->getString() + "_uu", declareOnly);
                compileFunction(compute(s_uv), o.name->argIndex, str, o.name->getString() + "_uv", declareOnly);
                compileFunction(compute(s_vv), o.name->argIndex, str, o.name->getString() + "_vv", declareOnly);

            }
            if(o.type == function)
            {
                Expr *cf = compute(o.sub[0]);
                try
                {
                    int args = argList[o.name->argIndex].size();
                    if(args != 1 && args != 2)
                        throw std::string("Functions should have 1 or 2 parameters");
                    int N = cf->tupleSize;
                    if(N != 1) throw std::string("Function is not plottable");

                    std::stringstream buf;
                    compileFunction(cf, o.name->argIndex, buf, o.name->getString(), declareOnly);
                    str << buf.str();
                } catch(std::string &error){}
            }
            if(o.type == point)
            {
                Expr *cp = compute(o.sub[0]);
                int N = cp->tupleSize;
                if(N != 2 && N != 3) throw std::string("Points must be in 2d or 3d space");

                compileFunction(cp, -1, str, o.name->getString(), declareOnly);
            }

            if(o.type == vector)
            {
                Expr *cv = compute(o.sub[0]);
                Expr *cv2 = compute(o.sub[1]);
                int N = cv->tupleSize;
                if(N != 2 && N != 3) throw std::string("Vectors must be in 2d or 3d space");
                if(cv2->tupleSize != N) throw std::string("Inconsistent space dimensions");

                compileFunction(cv, -1, str, o.name->getString(), declareOnly);
                compileFunction(cv2, -1, str, o.name->getString()+"_org", declareOnly);
            }
        }
    }

    void Compiler::compileFunction(Expr *exp, int argIndex, std::stringstream &str, std::string name, bool declareOnly)
    {
        int N = exp->tupleSize;
        declareFunction(N, argIndex, str, name);

        if(declareOnly)
        {
            str << ";\n";
            return;
        }

        str << "\n{\n";
        int v = 0;
        compile(exp, str, v);

        if(N == 1)
            str << "return v" << v << ";\n";
        else if(N == 2)
            str << "return vec2(v" << v-1 << ", v" << v << ");\n";
        else if(N == 3)
            str << "return vec3(v" << v-2 << ", v" << v-1 << ", v" << v << ");\n";

        str << "}\n";
    }

    void Compiler::declareFunction(int N, int argIndex, std::stringstream &str, std::string name)
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
    }

    float Compiler::calc(Expr *e)
    {
        switch(e->type)
        {
            case E(NUMBER): return e->number;
            case E(CONSTANT):
            {
                if(e->name == this->e) return Parser::CE;
                if(e->name == this->pi) return Parser::CPI;
                if(e->name->objIndex == -1)
                    throw std::string("Invalid constant");
                Obj o = objects[e->name->objIndex];
                if(o.type != grid && o.type != param)
                    throw std::string("Invalid constant");
                if(o.intervals.size() != 1)
                    throw std::string("Invalid constant");
                return o.intervals[0].number;
            }
            case E(COMPONENT):
            {
                if(e->sub[0]->type != E(CONSTANT))
                    throw std::string("Invalid component");
                Table *name = e->sub[0]->name;
                if(!name) throw std::string("Invalid component");
                if(name->objIndex == -1) throw std::string("Invalid component");
                Obj o = objects[name->objIndex];
                if(o.type != grid && o.type != param)
                    throw std::string("Invalid component");
                return o.intervals[e->number].number;
            }
            case E(APP):
            {
                if(e->sub[0]->type != E(FUNCTION))
                    throw std::string("Invalid application");
                Table *name = e->sub[0]->name;
                if(name->argIndex != -1)
                    throw std::string("Invalid application");
                if(name == sin) return std::sin(calc(e->sub[1]));
                if(name == cos) return std::cos(calc(e->sub[1]));
                if(name == tan) return std::tan(calc(e->sub[1]));
                if(name == exp) return std::exp(calc(e->sub[1]));
                if(name == log) return std::log(calc(e->sub[1]));
                if(name == sqrt) return std::sqrt(calc(e->sub[1]));
                if(name == id) return (calc(e->sub[1]));
                throw std::string("Invalid function");
            }
            case E(PLUS): return calc(e->sub[0]) + calc(e->sub[1]);
            case E(MINUS): return calc(e->sub[0]) - calc(e->sub[1]);
            case E(TIMES):
            case E(JUX):
                return calc(e->sub[0]) * calc(e->sub[1]);
            case E(DIVIDE): 
                return calc(e->sub[0]) / calc(e->sub[1]);
            case E(UPLUS): return calc(e->sub[0]);
            case E(UMINUS): return -calc(e->sub[0]);
            case E(UDIVIDE): return 1.0/calc(e->sub[0]);
            case E(POW): return std::pow(calc(e->sub[0]), calc(e->sub[1]));
            default:
                throw std::string("Cannot calculate expression");
        }
    }

};

#undef E