#include <memory>
#include <vector>
#include <stdio.h>


#include <wx/wx.h>
#include <wx/stc/stc.h>


enum class Style
{
    DEFAULT = 0, COMMENT = 4, DECLARE, FUNCTION, CONSTANT, 
    VARIABLE, NUMBER, UNDEFINED, SYMBOL, ERROR
};

struct StyleText
{
    int start = 0;
    int length = 0;
    int line = 1;
    Style style = Style::DEFAULT; 
};

std::vector<StyleText> styles;

enum class Type : int
{
    EOI = 256,
    UNDEFINED,
    DECLARE,
    CONSTANT,
    VARIABLE,
    NUMBER,
    FUNCTION
};

int alphIndex(char c)
{
    if(c >= 'a' && c <= 'z') return c - 'a' + 0;
    if(c >= 'A' && c <= 'Z') return c - 'A' + 26;
    if(c >= '0' && c <= '9') return c - '0' + 52;
    return -1;
}

struct Trie
{
    Trie *parent = nullptr;
    std::unique_ptr<Trie> children[62];
    int data = -1;
    char c = 0;
    int length = 0;
    Type type = Type::UNDEFINED;

    enum class Mode
    {
        INSERT, MAX, MATCH
    };

    Trie *next(char _c, bool expand)
    {
        int index = alphIndex(_c);
        if(!children[index].get())
        {
            if(!expand) return nullptr;
            children[index] = std::make_unique<Trie>();
            Trie *t = children[index].get();
            t->parent = this;
            t->c = _c;
            t->length = length+1;
        }

        return children[index].get();
    }

    Trie *getString(const char *text, Mode mode)
    {
        int index = alphIndex(*text);
        if(index == -1) return this;
        Trie *a = next(*text, mode == Mode::INSERT);
        if(!a) return this;
        Trie *b = a->getString(text+1, mode);
        if(mode != Mode::MATCH) return b;
        if(b->type != Type::UNDEFINED) return b;
        return this;
    }

    Trie *setString(const char *text, Type type)
    {
        Trie *t = getString(text, Mode::INSERT);
        t->type = type;
        return t;
    }
};

struct Token
{
    const char *src = nullptr;
    const char *text = nullptr;
    int length = 0;
    int line = 1;
    Type type;
    double number;
    Trie *string = nullptr;
    Trie *table = nullptr;

    void advance(Trie::Mode mode = Trie::Mode::MATCH)
    {
        number = 0;
        string = nullptr;
        text += length;
        
        while(true)
        {
            StyleText style;
            while(*text == ' ' || *text == '\n' || *text == '\t')
            {
                if(*text == '\n') line++;
                text++;
            }
            if(*text != '#') break;
            style.style = Style::COMMENT;
            style.start = text-src;
            while(*text != '\n' && *text != 0) text++;
            style.length = text - src - style.start;
            style.line = line;
            styles.push_back(style);
        }

        if(*text == 0)
        {
            length = 0;
            type = Type::EOI;
        }
        else
        {
            if((*text >= '0' && *text <= '9') || *text == '.')
            {
                type = Type::NUMBER;
                int k = sscanf(text, "%lf%n\n", &number, &length);
                if(k != 1) throw 0;
                return;
            }

            int index = alphIndex(*text);
            if(index == -1)
            {
                length = 1;
                type = (Type)*text;
                return;
            }

            string = table->getString(text, mode);
            length = string->length;
            type = string->type;

            if(mode == Trie::Mode::MATCH && type == Type::UNDEFINED)
            {
                string = table->getString(text, Trie::Mode::INSERT);
                length = string->length;
                type = string->type;
            }
        }
    }
};

struct Object
{
    std::vector<Trie*> list;
};

struct Highlight
{
    Token token;
    Trie table;
    #define INIT(x, y) Trie *x = table.setString(#x, Type::y);
        INIT(param,     DECLARE)
        INIT(grid,      DECLARE)
        INIT(define,      DECLARE)
        INIT(curve,     DECLARE)
        INIT(surface,   DECLARE)
        INIT(function,  DECLARE)
        INIT(point,     DECLARE)
        INIT(vector,    DECLARE)
        INIT(e,         CONSTANT)
        INIT(pi,        CONSTANT)
        INIT(sin,       FUNCTION)
        INIT(cos,       FUNCTION)
        INIT(tan,       FUNCTION)
        INIT(exp,       FUNCTION)
        INIT(log,       FUNCTION)
        INIT(sqrt,      FUNCTION)
    #undef INIT

    std::vector<Object> objs;

    Highlight(const char *text)
    {
        token.src = text;
        token.text = text;
        token.table = &table;
        token.length = 0;
        token.line = 0;

        styles.clear();

        try
        {
            advance();
            parseProgram();
        } catch (int i)
        {

        }
    }

    const char *getTypeString(Type t)
    {
        static char c[4] = {'(', 0, ')', 0};
        if((int)t < 256)
        {
            c[1] = (char)t;
            return &c[0];
        }

        switch(t)
        {
            case Type::EOI: return "EOI";
            case Type::UNDEFINED: return "UNDEFINED";
            case Type::DECLARE: return "DECLARE";
            case Type::CONSTANT: return "CONSTANT";
            case Type::VARIABLE: return "VARIABLE";
            case Type::NUMBER: return "NUMBER";
            case Type::FUNCTION: return "FUNCTION";
            default: return "??";
        }
    }

    void setError()
    {
        StyleText s;
        s.start = token.text - token.src;
        s.length = token.length;
        s.style = Style::ERROR;
        s.line = token.line;
        styles.push_back(s);
        throw 0;
    }

    void require(Type type)
    {
        if(token.type != type) setError();
    }

    bool compare(Type type)
    {
        return token.type == type;
    }

    void color()
    {
        StyleText s;
        s.start = token.text - token.src;
        s.length = token.length;
        s.line = token.line;

        if((int)token.type < 256) s.style = Style::SYMBOL;
        else switch(token.type)
        {
            case Type::UNDEFINED: s.style = Style::UNDEFINED; break;
            case Type::EOI: s.style = Style::SYMBOL; break;
            case Type::DECLARE: s.style = Style::DECLARE; break;
            case Type::CONSTANT: s.style = Style::CONSTANT; break;
            case Type::VARIABLE: s.style = Style::VARIABLE; break;
            case Type::NUMBER: s.style = Style::NUMBER; break;
            case Type::FUNCTION: s.style = Style::FUNCTION; break;
        }
        styles.push_back(s);
    }

    void advance(Trie::Mode mode = Trie::Mode::MATCH)
    {
        color();        
        token.advance(mode);
    }

    void parseProgram()
    {
        while(!compare(Type::EOI)) parseDecl();
    }

    void parseInt()
    {
        require((Type)'[');
        advance();
        parseExpr();
        require((Type)',');
        advance();
        parseExpr();
        require((Type)']');
        advance();
    }

    void parseIGrid()
    {
        require((Type)'[');
        advance();
        parseExpr();
        require((Type)',');
        advance();
        parseExpr();
        require((Type)',');
        advance();
        parseExpr();
        require((Type)']');
        advance();
    }

    void parseIGrids()
    {
        parseIGrid();
        if(compare((Type)','))
        {
            advance();
            parseIGrid();
        }
    }

    void parseTint()
    {
        require(Type::VARIABLE);
        advance();
        require((Type)':');
        advance();
        if(compare((Type)'+') || compare((Type)'-')) advance();
        parseInt();
    }

    void parseTints()
    {
        parseTint();
        if(compare((Type)','))
        {
            advance();
            parseTint();
        }
    }

    void parseInts()
    {
        parseInt();
        if(compare((Type)','))
        {
            advance();
            parseInt();
        }
    }

    void removeArgs(Object &obj)
    {
        for(Trie *t : obj.list) t->type = Type::UNDEFINED;
    }

    void parseFdecl(Object &obj)
    {
        require(Type::UNDEFINED);
        Trie *string = token.string;
        token.type = Type::FUNCTION;
        advance();
        require((Type)'(');
        advance(Trie::Mode::INSERT);

        while(true)
        {
            require(Type::UNDEFINED);
            Trie *arg = token.string;
            arg->type = Type::VARIABLE;
            obj.list.push_back(arg);
            advance();

            if(!compare((Type)',')) break;
            advance(Trie::Mode::INSERT);
        }

        require((Type)')');
        advance();

        require((Type)'=');
        advance();

        parseExpr();

        string->type = Type::FUNCTION;
        string->data = (int)objs.size();
    }

    void parseParam()
    {
        advance(Trie::Mode::INSERT);
        require(Type::UNDEFINED);
        Trie *string = token.string;
        advance();
        require((Type)'=');
        advance();
        parseInts();
        require((Type)';');
        advance();
        string->type = Type::VARIABLE;
    }

    void parseGrid()
    {
        advance(Trie::Mode::INSERT);
        require(Type::UNDEFINED);
        Trie *string = token.string;
        advance();
        require((Type)'=');
        advance();
        parseIGrids();
        require((Type)';');
        advance();
        string->type = Type::VARIABLE;
    }

    void parseDefine()
    {
        advance(Trie::Mode::INSERT);
        require(Type::UNDEFINED);
        Trie *string = token.string;
        advance();
        require((Type)'=');
        advance();
        parseExpr();
        require((Type)';');
        advance();   
        string->type = Type::VARIABLE;     
    }

    void parseCurve()
    {
        Object obj;
        advance(Trie::Mode::INSERT);
        parseFdecl(obj);
        require((Type)',');
        advance();
        parseTints();
        require((Type)';');
        advance();
        removeArgs(obj);
        objs.push_back(obj);
    }

    void parseSurface()
    {
        Object obj;
        advance(Trie::Mode::INSERT);
        parseFdecl(obj);
        require((Type)',');
        advance();
        parseTints();
        require((Type)';');
        advance();
        removeArgs(obj);
        objs.push_back(obj);
    }

    void parseFunction()
    {
        Object obj;
        advance(Trie::Mode::INSERT);
        parseFdecl(obj);
        require((Type)';');
        advance();
        removeArgs(obj);
        objs.push_back(obj);
    }

    void parsePoint()
    {
        advance(Trie::Mode::INSERT);
        require(Type::UNDEFINED);
        Trie *string = token.string;
        advance();
        require((Type)'=');
        advance();
        parseExpr();
        require((Type)';');
        advance();   
        string->type = Type::VARIABLE;     
    }

    void parseVector()
    {
        advance(Trie::Mode::INSERT);
        require(Type::UNDEFINED);
        Trie *string = token.string;
        advance();
        require((Type)'=');
        advance();
        parseExpr();
        require((Type)'@');
        advance();
        parseExpr();
        require((Type)';');
        advance();   
        string->type = Type::VARIABLE;     
    }

    void parseDecl()
    {
        require(Type::DECLARE);
        
        if(token.string == param)           parseParam();
        else if(token.string == grid)       parseGrid();
        else if(token.string == define)     parseDefine();
        else if(token.string == curve)      parseCurve();
        else if(token.string == surface)    parseSurface();
        else if(token.string == function)   parseFunction();
        else if(token.string == point)      parsePoint();
        else if(token.string == vector)     parseVector();
    }


    void parseExpr()
    {
        parseAdd();
    }

    void parseAdd()
    {
        parseJux();

        while(compare((Type)'+') || compare((Type)'-'))
        {
            advance();
            parseJux();
        }
    }

    void parseJux()
    {
        parseMult(true);

        while(compare(Type::FUNCTION) || compare(Type::CONSTANT)
        || compare(Type::NUMBER) || compare(Type::VARIABLE) || compare((Type)'('))
        {
            parseMult(false);
        }
    }

    void parseMult(bool unary = true)
    {
        if(unary) parseUnary();
        else parseApp();

        while(compare((Type)'*') || compare((Type)'/'))
        {
            advance();
            parseUnary();
        }
    }

    void parseUnary()
    {
        while(compare((Type)'*') || compare((Type)'/') || 
        compare((Type)'+') || compare((Type)'-'))
        {
            advance();
        }
        parseApp();
    }

    void parseApp()
    {
        if(compare(Type::FUNCTION))
        {
            parseFunc();
            parseUnary();
        }
        else parsePow();
    }

    void parseFunc()
    {
        require(Type::FUNCTION);
        Trie *string = token.string;
        advance();

        while(true)
        {
            if(compare((Type)'\''))
            {
                advance();
            }
            else if(compare((Type)'_'))
            {
                advance(Trie::Mode::MAX);
                bool ok = false;

                if(string->data != -1 && token.string)
                {
                    Object &obj = objs[string->data];
                    while(true)
                    {
                        for(Trie *arg : obj.list)
                        {
                            if(arg == token.string)
                            {
                                ok = true;
                                break;
                            }
                        }
                        if(ok) break;

                        if(token.string->parent)
                        {
                            token.string = token.string->parent;
                            if(token.length) token.length--;
                        }
                        else break;
                    }

                }
                if(!ok)
                {
                    token.length = 1;
                    setError();
                }
                token.type = Type::FUNCTION;
                color();
                advance();
            }
            else break;
        }

        if(compare((Type)'^'))
        {
            advance();
            parseUnary();
        }
    }

    void parsePow()
    {
        parseComp();

        if(compare((Type)'^'))
        {
            advance();
            parseUnary();
        }
    }

    void parseComp()
    {
        parseFact();

        while(compare((Type)'_'))
        {
            advance();
            require(Type::NUMBER);
            advance();
        }
    }

    void parseFact()
    {
        if(compare(Type::CONSTANT)) advance();
        else if(compare(Type::NUMBER)) advance();
        else if(compare(Type::VARIABLE)) advance();
        else parseTuple();
    }

    void parseTuple()
    {
        require((Type)'(');
        advance();
        parseAdd();
        while(compare((Type)','))
        {
            advance();
            parseAdd();
        }
        require((Type)')');
        advance();
    }
};

class Simple : public wxFrame
{
public:
    Simple(const wxString& title);
    wxStyledTextCtrl *ctrl;

    void OnStyleNeeded(wxStyledTextEvent& event);
};

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};


IMPLEMENT_APP(MyApp)

Simple::Simple(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(512, 512))
{
    Centre();
    ctrl = new wxStyledTextCtrl(this);
    ctrl->SetLexer(wxSTC_LEX_CONTAINER);
    ctrl->Bind(wxEVT_STC_STYLENEEDED, &Simple::OnStyleNeeded, this);

    wxFont italic;
    italic.SetFaceName("C059");
    italic.SetStyle(wxFONTSTYLE_ITALIC);

    wxFont bold;
    bold.SetFaceName("FreeMono");
    bold.SetWeight(wxFONTWEIGHT_BOLD);

    wxFont bolditalic;
    bolditalic.SetFaceName("FreeMono");
    bolditalic.SetWeight(wxFONTWEIGHT_BOLD);
    bolditalic.SetStyle(wxFONTSTYLE_ITALIC);

    wxFont normal;
    normal.SetFaceName("FreeMono");

    #define STYLE(x,y,z) ctrl->StyleSet##x((int)Style::y, z)

        STYLE(Foreground, DEFAULT,  wxColor(0, 0, 0));
        STYLE(Foreground, SYMBOL,   wxColor(0, 0, 0));
        STYLE(Foreground, COMMENT,  wxColor(0, 100, 0));
        STYLE(Foreground, DECLARE,  wxColor(50, 50, 200));
        STYLE(Foreground, FUNCTION, wxColor(50, 50, 50));
        STYLE(Foreground, CONSTANT, wxColor(100, 120, 150));
        STYLE(Foreground, VARIABLE, wxColor(242, 150, 11));
        STYLE(Foreground, NUMBER,   wxColor(100, 100, 255));
        STYLE(Foreground, UNDEFINED,wxColor(242, 150, 11));
        STYLE(Foreground, ERROR,    wxColor(230, 0, 0));

        STYLE(Font, DEFAULT, normal);
        STYLE(Font, SYMBOL, bold);
        STYLE(Font, COMMENT, bolditalic);
        STYLE(Font, DECLARE, bold);
        STYLE(Font, FUNCTION, italic);
        STYLE(Font, CONSTANT, italic);
        STYLE(Font, VARIABLE, italic);
        STYLE(Font, UNDEFINED, italic);
        STYLE(Font, ERROR, bold);

    #undef STYLE

    wxColor col = wxColor(255, 0, 0);
    ctrl->MarkerDefine(1, wxSTC_MARK_CIRCLE, wxNullColour, col);
}


void Simple::OnStyleNeeded(wxStyledTextEvent& event)
{
    wxString text = ctrl->GetText();
    size_t length = text.length();

    ctrl->MarkerDeleteAll(1);
    ctrl->StartStyling(0);
    ctrl->SetStyling(length, 0);

    Highlight hl(text.c_str());

    for(StyleText &s : styles)
    {
        ctrl->StartStyling(s.start);
        ctrl->SetStyling(s.length, (int)s.style);
        if(s.style == Style::ERROR)
        {
            ctrl->MarkerAdd(s.line, 1);
        }
    }
}

bool MyApp::OnInit()
{
    Simple *simple = new Simple(wxT("Simple"));
    simple->Show(true);
    return true;
}