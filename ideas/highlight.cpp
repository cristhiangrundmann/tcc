#include "parser.hpp"

#include <wx/wx.h>
#include <wx/stc/stc.h>

namespace tcc
{
    struct StyleText
    {
        enum class Style
        {
            DEFAULT = 0, COMMENT = 4, DECLARE, FUNCTION, CONSTANT, 
            VARIABLE, NUMBER, UNDEFINED, SYMBOL, ERROR
        };

        int start = 0;
        int length = 0;
        int line = 1;
        Style style = Style::DEFAULT;
    };

    struct Highlight : public Parser
    {
        std::vector<StyleText> styles;

        void actAdvance()
        {
            StyleText s;
            s.start = lexer.lexeme - lexer.source;
            s.length = lexer.length;
            s.line = lexer.lineno;

            if((int)lexer.type < 256) s.style = StyleText::Style::SYMBOL;
            else switch(lexer.type)
            {
                case TokenType::UNDEFINED: s.style = StyleText::Style::UNDEFINED; break;
                case TokenType::EOI: s.style = StyleText::Style::SYMBOL; break;
                case TokenType::DECLARE: s.style = StyleText::Style::DECLARE; break;
                case TokenType::CONSTANT: s.style = StyleText::Style::CONSTANT; break;
                case TokenType::VARIABLE: s.style = StyleText::Style::VARIABLE; break;
                case TokenType::NUMBER: s.style = StyleText::Style::NUMBER; break;
                case TokenType::FUNCTION: s.style = StyleText::Style::FUNCTION; break;
                case TokenType::COMMENT: s.style = StyleText::Style::COMMENT; break;
            }
            styles.push_back(s);
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

        #define STYLE(x,y,z) ctrl->StyleSet##x((int)StyleText::Style::y, z)

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

        Highlight hl;

        try
        {
            hl.parseProgram(text.c_str());
            printf("Correct\n");
        } catch(const char *msg)
        {
            printf("Error: %s\n", msg);
        }

        for(StyleText &s : hl.styles)
        {
            ctrl->StartStyling(s.start);
            ctrl->SetStyling(s.length, (int)s.style);
            if(s.style == StyleText::Style::ERROR)
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

}

IMPLEMENT_APP(tcc::MyApp)