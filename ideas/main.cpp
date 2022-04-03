#include <stdio.h>
#include <stdlib.h>

//ascii tokens: ( ) [ ] : ; = @ , . + - * / _ ^ '
#define EOI         256
#define NUMBER      257
#define ID          258
#define VAR_ID      259
#define FUNC_ID     260
#define COMMAND_ID  261
#define CONST       262
#define EPS         263

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
    Node *children[62];
    char c;
    int type;
    int length;

    Node()
    {
        parent = nullptr;
        for(int i = 0; i < 62; i++) children[i] = nullptr;
        c = 0;
        type = ID;
        length = 0;
    }

    ~Node()
    {
        for(int i = 0; i < 62; i++) if(children[i] != nullptr) children[i]->~Node(); 
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
    void *data;
    Node *table;
    Node *id;
    double number;
    int length;
    int type;

    Token()
    {
        str = nullptr;
        data = nullptr;
        table = nullptr;
        id = nullptr;
        number = 0;
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
                    printf("Syntax error: (%d)\n", *str);
                    exit(1);
                }
        }
    }
};

enum Predef
{
    U=0, V, T, E, PI, PARAM, CURVE, SURFACE, DEFINE, FUNCTION, GRID, POINT, VECTOR,
    SIN, COS, TAN, LOG, SQRT, EXP, LEN
};

#define PDECL(x,y,z) predef[x] = table.insert(y); predef[x]->type = z;

struct Program
{
    Token token;
    Node table;
    Node *predef[31];

    const char *code; 

    #define MATCH(x) {if(token.type != x) {printf("Syntax error: %s @ %d\n", __FILE__, __LINE__); exit(1);}}
    #define MATCHA(x) {MATCH(x); token.advance(MAX_MATCH);}

    Program(const char *src)
    {
        code = src;
        token.str = code;
        token.table = &table;

        PDECL(U, "u", VAR_ID);
        PDECL(V, "v", VAR_ID);
        PDECL(T, "t", VAR_ID);
        PDECL(E, "e", CONST);
        PDECL(PI, "pi", CONST);
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

        token.advance(MAX_MATCH);

        parse_prog();

        MATCHA(EOI);
    }

    bool cmp(int type)
    {
        return token.type == type;
    }

    void parse_prog()
    {
        while(true)
        {
            if(cmp(COMMAND_ID)) parse_decl();
            else break;
        }
    }

    void parse_args()
    {
        parse_arg();

        while(true)
        {
            if(cmp(','))
            {
                token.advance(INSERT);
                parse_arg();
            }
            else break;
        }
    }

    void parse_arg()
    {
        Token t = token;
        MATCHA(ID);
        t.id->type = VAR_ID;
    }

    void delete_args()
    {
        delete_arg();

        while(true)
        {
            if(cmp(','))
            {
                token.advance(MAX_MATCH);
                delete_arg();
            }
            else break;
        }
    }

    void delete_arg()
    {
        Token t = token;
        MATCHA(VAR_ID);                
        t.id->type = ID;
    }

    void parse_decl()
    {
        MATCH(COMMAND_ID);

        if(token.id == predef[PARAM]) parse_param();
        if(token.id == predef[CURVE]) parse_curve();
        if(token.id == predef[SURFACE]) parse_surface();
        if(token.id == predef[DEFINE]) parse_define();
        if(token.id == predef[FUNCTION]) parse_function();
        if(token.id == predef[GRID]) parse_grid();
        if(token.id == predef[POINT]) parse_point();
        if(token.id == predef[VECTOR]) parse_vector();
    }

    void parse_param()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA(':');
        parse_int();
        MATCHA(';');
        t.id->type = VAR_ID;
    }

    void parse_curve()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        parse_add();
        MATCHA(',');
        MATCHA(VAR_ID);
        MATCHA(':');
        parse_int();
        MATCHA(';');
        t.id->type = FUNC_ID;
    }

    void parse_surface()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        parse_add();
        MATCHA(',');
        MATCHA(VAR_ID);
        MATCHA(':');
        parse_int();
        MATCHA(',');
        MATCHA(VAR_ID);
        MATCHA(':');
        parse_int();
        MATCHA(';');
        t.id->type = FUNC_ID;
    }

    void parse_define()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
    }

    void parse_function()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        Token t2 = token;
        MATCH('(');
        token.advance(INSERT);
        parse_args();
        MATCHA(')');
        MATCHA('=');
        parse_add();
        MATCHA(';');
        t.id->type = FUNC_ID;
        Token cur = token;
        token = t2;
        MATCHA('(');
        delete_args();
        token = cur;
    }

    void parse_grid()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA(':');
        parse_int();
        MATCHA(',');
        parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
    }

    void parse_point()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
    }

    void parse_vector()
    {
        token.advance(INSERT);
        Token t = token;
        MATCHA(ID);
        MATCHA('=');
        parse_add();
        MATCHA('@');
        parse_add();
        MATCHA(';');
        t.id->type = VAR_ID;
    }

    void parse_int()
    {
        MATCHA('[');
        parse_add();
        MATCHA(',');
        parse_add();
        MATCHA(']');
    }

    void parse_add()
    {
        parse_mult();

        while(true)
        {
            if(cmp('+'))
            {
                token.advance(MAX_MATCH);
                parse_mult();
            }
            else if(cmp('-'))
            {
                token.advance(MAX_MATCH);
                parse_mult();
            }
            else break;
        }
    }

    void parse_mult()
    {
        parse_unary();

        while(true)
        {
            if(cmp('*'))
            {
                token.advance(MAX_MATCH);
                parse_unary();
            }
            else if(cmp('/'))
            {
                token.advance(MAX_MATCH);
                parse_unary();
            }
            else if(cmp(FUNC_ID) || cmp(CONST) || cmp(NUMBER) || cmp(VAR_ID) || cmp('('))
            {
                parse_app();
            }
            else break;
        }
    }

    void parse_unary()
    {
        if(cmp('+'))
        {
            token.advance(MAX_MATCH);
            parse_unary();
        }
        else if(cmp('-'))
        {
            token.advance(MAX_MATCH);
            parse_unary();
        }
        else if(cmp('*'))
        {
            token.advance(MAX_MATCH);
            parse_unary();
        }
        else if(cmp('/'))
        {
            token.advance(MAX_MATCH);
            parse_unary();
        }
        else parse_app();
    }

    void parse_app()
    {
        if(cmp(FUNC_ID))
        {
            parse_func();
            parse_unary();
        }
        else parse_exp();
    }

    void parse_func()
    {
        MATCHA(FUNC_ID);

        while(true)
        {
            if(cmp('\''))
            {
                token.advance(MAX_MATCH);
            }
            else if(cmp('^'))
            {
                token.advance(MAX_MATCH);
                parse_unary();
            }
            else if(cmp('_'))
            {
                token.advance(MAX);
                if(token.length > 0) token.length = 1; //WRONG
                token.advance(MAX_MATCH);
            }
            else break;
        }
    }

    void parse_exp()
    {
        parse_comp();
        if(cmp('^'))
        {
            token.advance(MAX_MATCH);
            parse_unary();
        }
        else ;
    }

    void parse_comp()
    {
        parse_fact();

        if(cmp('.'))
        {
            token.advance(MAX_MATCH);
            MATCHA(NUMBER);
        }
        else ;
    }

    void parse_fact()
    {
        if(cmp(CONST))
        {
            token.advance(MAX_MATCH);
        }
        else if(cmp(NUMBER))
        {
            token.advance(MAX_MATCH);
        }
        else if(cmp(VAR_ID))
        {
            token.advance(MAX_MATCH);
        }
        else parse_tuple();
    }

    void parse_tuple()
    {
        MATCHA('(');
        parse_list();
        MATCHA(')');
    }

    void parse_list()
    {
        parse_add();
        if(cmp(','))
        {
            token.advance(MAX_MATCH);
            parse_list();
        }
        else ;
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

    printf("Correct\n");
    return 0;
}