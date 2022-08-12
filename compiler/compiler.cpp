#include "compiler.hpp"

using std::make_unique;
using std::unique_ptr;

namespace tcc
{

    void Compiler::actInt(ExprType type)
    {
        Expr *e[3]{};
        e[0] = expStack.top(); expStack.pop();
        e[1] = expStack.top(); expStack.pop();
        if(type == ExprType::GRID)
        {
            e[2] = expStack.top();
            expStack.pop();
        }
        Interval i = {type, tag, wrap, {e[0], e[1], e[2]}};
        intStack.push(i);
    }

    void Compiler::actOp(ExprType type)
    {
        Expr exp;
        exp.type = type;
        switch(type)
        {
            case ExprType::PARTIAL:
                exp.sub[0] = expStack.top();
                expStack.pop();
                exp.name = lexer.node;
                break;
            case ExprType::COMPONENT:
                exp.sub[0] = expStack.top();
                expStack.pop();
                exp.number = lexer.number;
                if((int)exp.number != exp.number || (int)exp.number < 1)
                    throw std::string("Component must be a positive integer");
                break;
            case ExprType::PLUS:
            case ExprType::MINUS:
            case ExprType::TIMES:
            case ExprType::DIVIDE:
            case ExprType::JUX:
            case ExprType::APP:
            case ExprType::FUNCEXP:
            case ExprType::EXP:
                exp.sub[0] = expStack.top();
                expStack.pop();
                exp.sub[1] = expStack.top();
                expStack.pop();
                break;
            case ExprType::TUPLE:
            {
                Expr *right = nullptr;
                Expr *exp = expStack.top();
                expStack.pop();
                while(--tupleSize)
                {
                    Expr e;
                    e.type = ExprType::TUPLE;
                    e.sub[0] = exp;
                    e.sub[1] = right;
                    right = newExpr(e);
                    exp = expStack.top();
                    expStack.pop();
                }
                expStack.push(exp);
                return;
            }
            case ExprType::UPLUS:
            case ExprType::UMINUS:
            case ExprType::UTIMES:
            case ExprType::UDIVIDE:
            case ExprType::TOTAL:
                exp.sub[0] = expStack.top();
                expStack.pop();
                break;
            case ExprType::NUMBER:
                exp.number = lexer.number;
                break;
            case ExprType::CONSTANT:
            case ExprType::VARIABLE:
            case ExprType::FUNCTION:
                exp.name = lexer.node;
                break;
            default:
                throw std::string("Invalid expression type");
        }
        expStack.push(newExpr(exp));
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
                obj.intervals.push_back(intStack.top());
                intStack.pop();
            }
        }
        else if(objType == define)
        {
            obj.sub[0] = expStack.top();
            expStack.pop();
        }
        else if(objType == curve || objType == surface || objType == function)
        {
            obj.sub[0] = expStack.top();
            expStack.pop();
            if(objType != function) while(!intStack.empty())
            {
                obj.intervals.push_back(intStack.top());
                intStack.pop();
            }
        }
        else if(objType == point || objType == vector)
        {
            obj.sub[0] = expStack.top();
            expStack.pop();
            if(objType == vector)
            {
                obj.sub[1] = expStack.top();
                expStack.pop();
            }
        }
        else throw std::string("Invalid declaration type");

        objects.push_back(obj);
    }

    Expr *Compiler::newExpr(Expr &e)
    {
        expressions.push_back(std::make_unique<Expr>(e));
        return expressions.back().get();
    }

    Expr *Compiler::op(ExprType type, Expr *a, Expr *b, Table *name, double number)
    {
        Expr e;
        e.type = type;
        e.sub[0] = a;
        e.sub[1] = b;
        e.name = name;
        e.number = number;
        return newExpr(e);
    }
    
    Expr *Compiler::derivative(Expr *e, Table *var)
    {
        
    }

};