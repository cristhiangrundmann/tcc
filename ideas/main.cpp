#include <stdio.h>
#include <stdlib.h>
#include <vector>

#define WUN __attribute__((warn_unused_result))

#define ERROR(x) {printf("%s: %s @ %d\n", x, __FILE__, __LINE__); exit(1);}
#define MATCH(x) {if(token.type != x) ERROR("Syntax error");}
#define MATCHA(x) {MATCH(x); token.advance(MAX_MATCH);}

#define PDECL(x,y,z) predef[x] = table.insert(y); predef[x]->type = z;

//ascii tokens: ( ) [ ] : ; = @ , . + - * / _ ^ '
#define EOI         256
#define NUMBER      257
#define ID          258
#define VAR_ID      259
#define FUNC_ID     260
#define COMMAND_ID  261
#define CONST_ID    262
#define EPS         263

using std::vector;

int next_address(char c)
{
    if(c >= 'a' && c <= 'z') return c-'a' + 0;
    if(c >= 'A' && c <= 'Z') return c-'A' + 26;
    if(c >= '0' && c <= '9') return c-'0' + 52;
    return -1;
}

struct Node
{
    Node *parent;
    void *data;
    Node *children[62];
    char c;
    int type;
    int length;

    Node()
    {
        parent = nullptr;
        data = nullptr;
        for(Node *&node : children) node = nullptr;
        c = 0;
        type = ID;
        length = 0;
    }

    ~Node()
    {
        for(int i = 0; i < 62; i++) if(children[i]) delete children[i];
    }

    Node *next(char c, bool expand)
    {
        int a = next_address(c);
        Node *child = children[a];
        if(child == nullptr)
        {
            if(!expand) return nullptr;
            child = new Node;
            children[a] = child;
            child->parent = this;
            child->c = c;
            child->type = ID;
            child->length = length+1;
        }
        return child;
    }

    Node *insert(const char *str)
    {
        if(next_address(*str) == -1) return this;
        return next(*str, true)->insert(str+1);
    }

    Node *max(const char *str)
    {
        if(next_address(*str) == -1) return this;
        Node *n = next(*str, false);
        if(n == nullptr) return this;
        return n->max(str+1);
    }

    Node *max_match(const char *str)
    {
        if(next_address(*str) == -1) return this;
        Node *n = next(*str, false);
        if(n == nullptr) return this;
        Node *lm = n->max_match(str+1);
        if(lm->type != ID) return lm;
        return this;
    }
};

enum TableMode
{
    INSERT, MAX, MAX_MATCH
};

struct Token
{
    const char *str;
    Node *table;
    union
    {
        Node *id;
        double number;
    };
    int length;
    int type;

    Token()
    {
        str = nullptr;
        table = nullptr;
        id = nullptr;
        length = 0;
        type = EOI;
    }

    void advance(TableMode mode)
    {
        str += length;

        skipwhitespace:;
        while(*str == ' ' || *str == '\t' || *str == '\n') str++;

        if(*str == '#')
        {
            while(*str != '\n' && *str != 0) str++;
            goto skipwhitespace;
        }

        length = 1;

        id = nullptr;

        switch(*str)
        {
            case 0:
                length = 0;
                type = EOI;
                return;
            case '(': type = *str; return;
            case ')': type = *str; return;
            case '[': type = *str; return;
            case ']': type = *str; return;
            case ':': type = *str; return;
            case ';': type = *str; return;
            case '=': type = *str; return;
            case '@': type = *str; return;
            case ',': type = *str; return;
            case '+': type = *str; return;
            case '-': type = *str; return;
            case '.': type = *str; return;
            case '*': type = *str; return;
            case '/': type = *str; return;
            case '_': type = *str; return;
            case '^': type = *str; return;
            case '\'': type = *str; return;
            default:
                if(*str >= '0' && *str <= '9')
                {
                    int k = sscanf(str, "%lf%n", &number, &length);
                    if(k != 1) exit(3);
                    type = NUMBER;
                }
                else if(next_address(*str) != -1)
                {
                    if(mode == INSERT)         id = table->insert(str);
                    else if(mode == MAX)       id = table->max(str);
                    else if(mode == MAX_MATCH) id = table->max_match(str);
                    else exit(2);
                    type = id->type;
                    length = id->length;
                    if(length == 0) type = EPS;
                }
                else
                {
                    printf("%c: ", *str);
                    ERROR("invalid character");
                }
        }
    }
};

enum Operator
{
    PLUS, MINUS, TIMES, DIVIDE, DOT, POW, FUNC_POW, PRIME, PARTIAL, 
    UPLUS, UMINUS, UTIMES, UDIVIDE, APP, COMP, VARCONST, NUM, TUPLE, FUNC
};

struct Expr
{
    Operator op;

    union
    {
        Expr *binary[2];
        Expr *unary;
        Node *func;
        struct
        {
            Expr *partial_func;
            Node *partial_id;
        };
        double number;
        Node *varconst;
    };
    vector<Expr*> tuple;
};

struct Interval
{
    Expr *a, *b;
};

struct Param
{
    Node *id;
    Interval i;
};  

struct Define
{
    Node *id;
    Expr *e;
};

struct Function
{
    Node *id;
    Expr *e;
    Interval t, u, v;
    vector<Node*> args;
};

struct Grid
{
    Node *id;
    Interval i;
    Expr *e;
};

struct Point
{
    Node *id;
    Expr *e;
};

struct Vector
{
    Node *id;
    Expr *v, *p;
};

enum Predef
{
    U=0, V, T, E, PI, PARAM, CURVE, SURFACE, DEFINE, FUNCTION, GRID, POINT, VECTOR,
    SIN, COS, TAN, LOG, SQRT, EXP, LEN
};

struct Program
{
    Token token;
    Node table;
    Node *predef[31];
    vector<Node*> prog;
    vector<Node*> curve_args;
    vector<Node*> surface_args;

    const char *code; 

    Program(const char *src)
    {
        code = src;
        token.str = code;
        token.table = &table;
        
        PDECL(U, "u", VAR_ID);
        PDECL(V, "v", VAR_ID);
        PDECL(T, "t", VAR_ID);
        PDECL(E, "e", CONST_ID);
        PDECL(PI, "pi", CONST_ID);
        PDECL(PARAM, "param", COMMAND_ID);
        PDECL(CURVE, "curve", COMMAND_ID);
        PDECL(SURFACE, "surface", COMMAND_ID);
        PDECL(DEFINE, "define", COMMAND_ID);
        PDECL(FUNCTION, "function", COMMAND_ID);
        PDECL(GRID, "grid", COMMAND_ID);
        PDECL(POINT, "point", COMMAND_ID);
        PDECL(VECTOR, "vector", COMMAND_ID);
        PDECL(SIN, "sin", FUNC_ID);
        PDECL(COS, "cos", FUNC_ID);
        PDECL(TAN, "tan", FUNC_ID);
        PDECL(LOG, "log", FUNC_ID);
        PDECL(SQRT, "sqrt", FUNC_ID);
        PDECL(EXP, "exp", FUNC_ID);
        PDECL(LEN, "len", FUNC_ID);

        curve_args.push_back(predef[T]);
        surface_args.push_back(predef[U]);
        surface_args.push_back(predef[V]);

        token.advance(MAX_MATCH);

        prog = parse_prog();

        MATCHA(EOI);

        printf("Correct\n");
    }

    bool cmp(int type)
    {
        return token.type == type;
    }

    WUN vector<Node*> parse_prog()
    {
        vector<Node*> vec;
        while(true)
        {
            if(cmp(COMMAND_ID)) vec.push_back(parse_decl());
            else break;
        }
        return vec;
    }

    WUN vector<Node*> parse_args()
    {
        vector<Node*> vec;
        vec.push_back(parse_arg());

        while(true)
        {
            if(cmp(','))
            {
                token.advance(INSERT);
                vec.push_back(parse_arg());
            }
            else break;
        }

        return vec;
    }

    WUN Node* parse_arg()
    {
        Token t = token;
        MATCHA(ID);
        t.id->type = VAR_ID;
        return t.id;
    }

    WUN Node* parse_decl()
    {
        MATCH(COMMAND_ID);

        if(token.id == predef[PARAM]) return parse_param();
        if(token.id == predef[CURVE]) return parse_curve();
        if(token.id == predef[SURFACE]) return parse_surface();
        if(token.id == predef[DEFINE]) return parse_define();
        if(token.id == predef[FUNCTION]) return parse_function();
        if(token.id == predef[GRID]) return parse_grid();
        if(token.id == predef[POINT]) return parse_point();
        if(token.id == predef[VECTOR]) return parse_vector();

        return nullptr;
    }

    WUN Node* parse_param()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA(':');
        Interval i = parse_int();
        MATCHA(';');
        t.id->type = VAR_ID;
        Param *p = new Param;
        p->i = i;
        p->id = t.id;
        t.id->data = p;
        return t.id;
    }

    WUN Node* parse_curve()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        Expr *e = parse_add();
        MATCHA(',');
        MATCH(VAR_ID);
        if(predef[T] != token.id) ERROR("Parameter name should be t\n");
        token.advance(MAX_MATCH);
        MATCHA(':');
        Interval i = parse_int();
        MATCHA(';');
        t.id->type = FUNC_ID;
        Function *c = new Function;
        c->e = e;
        c->t = i;
        c->args = curve_args;
        c->id = t.id;
        t.id->data = c;
        return t.id;
    }

    WUN Node* parse_surface()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        Expr *e = parse_add();
        MATCHA(',');
        MATCH(VAR_ID);
        if(predef[U] != token.id) ERROR("Parameter name should be u");
        token.advance(MAX_MATCH);
        MATCHA(':');
        Interval u = parse_int();
        MATCHA(',');
        MATCH(VAR_ID);
        if(predef[V] != token.id) ERROR("Parameter name should be v");
        token.advance(MAX_MATCH);
        MATCHA(':');
        Interval v = parse_int();
        MATCHA(';');
        t.id->type = FUNC_ID;
        Function *s = new Function;
        s->e = e;
        s->u = u;
        s->v = v;
        s->args = surface_args;
        s->id = t.id;
        t.id->data = s;
        return t.id;
    }

    WUN Node* parse_define()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        Expr *e = parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
        Define *d = new Define;
        d->e = e;
        d->id = t.id;
        t.id->data = e;
        return t.id;
    }

    WUN Node* parse_function()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        Token t2 = token;
        MATCH('(');
        token.advance(INSERT);
        vector<Node*> args = parse_args();
        MATCHA(')');
        MATCHA('=');
        Expr *e = parse_add();
        MATCHA(';');
        t.id->type = FUNC_ID;
        Token cur = token;
        token = t2;
        MATCHA('(');
        token = cur;
        Function *f = new Function;
        f->e = e;
        f->args = args;
        f->id = t.id;
        t.id->data = f;
        for(Node *arg : args) arg->type = ID;
        return t.id;
    }

    WUN Node* parse_grid()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA(':');
        Interval i = parse_int();
        MATCHA(',');
        Expr *e = parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
        Grid *g = new Grid;
        g->i = i;
        g->e = e;
        g->id = t.id;
        t.id->data = g;
        return t.id;
    }

    WUN Node* parse_point()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        Expr *e = parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
        Point *p = new Point;
        p->e = e;
        p->id = t.id;
        t.id->data = p;
        return t.id;
    }

    WUN Node* parse_vector()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        Expr *v = parse_add();
        MATCHA('@');
        Expr *p = parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
        Vector *vec = new Vector;
        vec->v = v;
        vec->p = p;
        vec->id = t.id;
        t.id->data = vec;
        return t.id;
    }

    WUN Interval parse_int()
    {
        Interval i;
        MATCHA('[');
        i.a = parse_add();
        MATCHA(',');
        i.b = parse_add();
        MATCHA(']');
        return i;
    }

    WUN Expr *parse_add()
    {
        Expr *sum = parse_mult();

        while(true)
        {
            Expr *e;
            Expr *b;
            if(cmp('+'))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                b = parse_mult();
                e->op = PLUS;
            }
            else if(cmp('-'))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                b = parse_mult();
                e->op = MINUS;
            }
            else break;
            e->binary[0] = sum;
            e->binary[1] = b;
            sum = e;
        }

        return sum;
    }

    WUN Expr *parse_mult()
    {
        Expr *prod = parse_unary();

        while(true)
        {
            Expr *e;
            Expr *b;
            if(cmp('*'))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                b = parse_unary();
                e->op = TIMES;
            }
            else if(cmp('/'))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                b = parse_unary();
                e->op = DIVIDE;
            }
            else if(cmp(FUNC_ID) || cmp(CONST_ID) || cmp(NUMBER) || cmp(VAR_ID) || cmp('('))
            {
                e = new Expr;
                b = parse_app();
                e->op = DOT;
            }
            else break;
            e->binary[0] = prod;
            e->binary[1] = b;
            prod = e;
        }

        return prod;
    }

    WUN Expr *parse_unary()
    {
        Expr *u;
        if(cmp('+'))
        {
            u = new Expr;
            token.advance(MAX_MATCH);
            u->unary = parse_unary();
            u->op = UPLUS;
        }
        else if(cmp('-'))
        {
            u = new Expr;
            token.advance(MAX_MATCH);
            u->unary = parse_unary();
            u->op = UMINUS;
        }
        else if(cmp('*'))
        {
            u = new Expr;
            token.advance(MAX_MATCH);
            u->unary = parse_unary();
            u->op = UTIMES;
        }
        else if(cmp('/'))
        {
            u = new Expr;
            token.advance(MAX_MATCH);
            u->unary = parse_unary();
            u->op = UDIVIDE;
        }
        else u = parse_app();

        return u;
    }

    WUN Expr *parse_app()
    {
        Expr *app;
        if(cmp(FUNC_ID))
        {
            app = new Expr;
            Expr *f = parse_func();
            Expr *u = parse_unary();
            app->op = APP;
            app->binary[0] = f;
            app->binary[1] = u;
        }
        else app = parse_pow();

        return app;
    }

    WUN Expr *parse_func()
    {
        Expr *func = new Expr;

        Token t = token;
        MATCHA(FUNC_ID);

        func->op = FUNC;
        func->func = t.id;
        Node *org_func = t.id;

        while(true)
        {
            Expr *e;
            if(cmp('\''))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                e->op = PRIME;
                e->unary = func;
                func = e;
            }
            else if(cmp('^'))
            {
                e = new Expr;
                token.advance(MAX_MATCH);
                Expr *b = parse_unary();
                e->op = FUNC_POW;
                e->binary[0] = func;
                e->binary[1] = b;
                func = e;
            }
            else if(cmp('_'))
            {
                e = new Expr;
                token.advance(MAX);
    
                if(token.id == nullptr) ERROR("Syntax error");

                Function *fp = (Function*)(org_func->data);

                while(true)
                {
                    bool found = false;
                    for(Node *arg : fp->args) if(arg == token.id)
                    {
                        found = true;
                        break;
                    }
                    if(found) break;

                    if(token.id == token.table) ERROR("Syntax error");

                    token.id = token.id->parent;
                }

                token.length = token.id->length;

                Token t = token;
                token.advance(MAX_MATCH);

                e->op = PARTIAL;
                e->partial_func = func;
                e->partial_id = t.id;
                func = e;
            }
            else break;
        }

        return func;
    }

    WUN Expr *parse_pow()
    {
        Expr *pow = parse_comp();

        if(cmp('^'))
        {
            token.advance(MAX_MATCH);
            Expr *b = parse_unary();
            Expr *e = new Expr;
            e->op = POW;
            e->binary[0] = pow;
            e->binary[1] = b;
            pow = e;
        }
        
        return pow;
    }

    WUN Expr *parse_comp()
    {
        Expr *comp = parse_fact();

        if(cmp('.'))
        {
            token.advance(MAX_MATCH);
            Token t = token;
            MATCHA(NUMBER);

            Expr *e = new Expr;
            Expr *n = new Expr;
            n->op = NUM;
            n->number = t.number;
            e->op = COMP;
            e->binary[0] = comp;
            e->binary[1] = n;
            comp = e;
        }

        return comp;
    }

    WUN Expr *parse_fact()
    {
        Expr *fact;
        Token t = token;

        if(cmp(CONST_ID))
        {
            fact = new Expr;
            fact->op = VARCONST;
            fact->varconst = t.id;
            token.advance(MAX_MATCH);
        }
        else if(cmp(NUMBER))
        {
            fact = new Expr;
            fact->op = NUM;
            fact->number = t.number;
            token.advance(MAX_MATCH);
        }
        else if(cmp(VAR_ID))
        {
            fact = new Expr;
            fact->op = VARCONST;
            fact->varconst = t.id;
            token.advance(MAX_MATCH);
        }
        else fact = parse_tuple();

        return fact;
    }

    WUN Expr *parse_tuple()
    {
        MATCHA('(');
        
        Expr *tuple = new Expr;
        tuple->op = TUPLE;
        Expr *a = parse_add();
        tuple->tuple.push_back(a);

        while(true)
        {
            if(cmp(','))
            {
                token.advance(MAX_MATCH);
                a = parse_add();
                tuple->tuple.push_back(a);
            }
            else break;
        }

        MATCHA(')');

        return tuple;
    }

};

int main()
{
    FILE *fp = fopen("input", "rb");
    if(!fp) return 1;

    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(size == 0) return 3;

    char *src = new char[size+1];
    int k = fread(src, size, 1, fp);
    if(k != 1) return 2;

    src[size] = 0;

    fclose(fp);

    Program p(src);

    return 0;
}